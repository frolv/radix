/*
 * arch/i386/mm/paging.c
 * Copyright (C) 2021 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "paging.h"

#include <radix/asm/msr.h>
#include <radix/config.h>
#include <radix/cpu.h>
#include <radix/kernel.h>
#include <radix/klog.h>
#include <radix/limits.h>
#include <radix/mm.h>
#include <radix/slab.h>
#include <radix/smp.h>
#include <radix/vmm.h>

#include <rlibc/string.h>

// The page directory containing the kernel's page mappings.
extern pde_t kernel_pgdir[PTRS_PER_PGDIR];

// Allocates a new page directory for a process and copies entries from the
// kernel's page directory into it, from index `start` to `end`.
//
// The page directory is returned mapped into the current address space to allow
// it to be further modified prior to starting the process. After all
// modifications are complete, the directory must be unmapped by calling
// unmap_cloned_pgdir().
static pde_t *clone_kernel_pgdir(size_t start, size_t end)
{
    assert(start < end);
    assert(end < PTRS_PER_PGDIR);

    struct page *p = alloc_page(PA_PAGETABLE);
    if (IS_ERR(p)) {
        return ERR_PTR(ERR_VAL(p));
    }

    pde_t *pgdir = vmalloc(PAGE_SIZE);
    if (!pgdir) {
        return ERR_PTR(ENOMEM);
    }

    const paddr_t phys = page_to_phys(p);

    int err =
        map_page_kernel((addr_t)pgdir, phys, PROT_WRITE, PAGE_CP_UNCACHEABLE);
    if (err) {
        return ERR_PTR(err);
    }

    memset(pgdir, 0, start * sizeof *pgdir);
    memcpy(pgdir + start, kernel_pgdir + start, (end - start) * sizeof *pgdir);
    memset(pgdir + end, 0, PAGE_SIZE - (end * sizeof *pgdir));

    return pgdir;
}

// Unmaps a page directory page from clone_kernel_pgdir() from the current
// address space.
//
// This does not release the physical memory allocated for the directory --
// only its current virtual address, which is not needed beyond initial setup.
static void unmap_cloned_pgdir(pde_t *pgdir)
{
    i386_unmap_pages((addr_t)pgdir, 1);
    vfree(pgdir);
}

// Releases the physical pages of the mapped page tables in the page directory
// located at address `phys` between entries `start` and `end`. This does not
// free the pages mapped within those page tables; they are managed by the VMM
// subsystem.
static int free_page_directory(paddr_t phys, size_t start, size_t end)
{
    pde_t *pgdir = vmalloc(PAGE_SIZE);
    if (!pgdir) {
        return ENOMEM;
    }

    int err =
        map_page_kernel((addr_t)pgdir, phys, PROT_READ, PAGE_CP_UNCACHEABLE);
    if (err) {
        return err;
    }

    for (size_t i = start; i < end; ++i) {
        pdeval_t value = PDE(pgdir[i]);
        if (value & PAGE_PRESENT) {
            free_pages(phys_to_page(value & PAGE_MASK));
        }
    }

    i386_unmap_pages((addr_t)pgdir, 1);
    vfree(pgdir);

    return 0;
}

static int ___map_page(pde_t *pgdir,
                       pte_t *pgtbl,
                       size_t pdi,
                       size_t pti,
                       paddr_t phys,
                       pteval_t flags)
{
    struct page *new;

    if (PDE(pgdir[pdi]) & PAGE_PRESENT) {
        /* page is already mapped */
        if (PTE(pgtbl[pti]) & PAGE_PRESENT)
            return EBUSY;
    } else {
        /* allocate a new page table */
        new = alloc_page(PA_PAGETABLE);
        if (IS_ERR(new))
            return ERR_VAL(new);

        pgdir[pdi] = make_pde(page_to_phys(new) | PAGE_RW | PAGE_PRESENT);
        memset(pgtbl, 0, PGTBL_SIZE);
    }
    pgtbl[pti] = make_pte(phys | flags | PAGE_PRESENT);

    return 0;
}

// Unmaps `n` pages starting at address `virt` from the specified page table,
// which has index `pdi` in the given page directory.
int __unmap_pages(pde_t *pgdir, size_t pdi, pte_t *pgtbl, addr_t virt, size_t n)
{
    size_t initial_pti, curr_pti;
    paddr_t phys;
    int unmapped;

    initial_pti = PGTBL_INDEX(virt);

    // Check if any previous pages in the current table are mapped.
    for (curr_pti = 0; curr_pti < initial_pti; ++curr_pti) {
        if (PTE(pgtbl[curr_pti]) & PAGE_PRESENT) {
            break;
        }
    }

    if (curr_pti == initial_pti) {
        initial_pti = 0;
    } else {
        curr_pti = initial_pti;
    }

    unmapped = 0;
    while (n) {
        pgtbl[curr_pti] = make_pte(0);
        tlb_flush_page_lazy(virt);

        --n;
        ++unmapped;
        virt += PAGE_SIZE;
        if (++curr_pti == PTRS_PER_PGTBL) {
            if (initial_pti == 0) {
                // All pages in the table are unmapped.
                phys = PDE(pgdir[pdi]) & PAGE_MASK;
                free_pages(phys_to_page(phys));
                pgdir[pdi] = make_pde(0);
                tlb_flush_page_lazy((addr_t)pgtbl);
            }
            break;
        }
    }

    return unmapped;
}

// Loads a page table from the specified index of a page directory and maps it
// to the address of the `pgtbl` pointer in the current address space. If no
// page table entry for the index is present, allocates a new one.
static int load_and_map_page_table(pde_t *pgdir, size_t pdi, pte_t *pgtbl)
{
    bool allocated = false;

    if (!(PDE(pgdir[pdi]) & PAGE_PRESENT)) {
        struct page *p = alloc_page(PA_PAGETABLE);
        if (IS_ERR(p)) {
            return ERR_VAL(p);
        }

        pgdir[pdi] =
            make_pde(page_to_phys(p) | PAGE_USER | PAGE_RW | PAGE_PRESENT);
        allocated = true;
    }

    paddr_t phys = PDE(pgdir[pdi]) & PAGE_MASK;

    int err =
        map_page_kernel((addr_t)pgtbl, phys, PROT_WRITE, PAGE_CP_UNCACHEABLE);
    if (err) {
        return err;
    }

    if (allocated) {
        // A newly-allocated page table should be zeroed.
        memset(pgtbl, 0, PAGE_SIZE);
    }

    return 0;
}

#if CONFIG(X86_PAE)

#define pgdir_base(n)          (addr_t)(0xFF800000 + (n)*MIB(2))
#define get_page_table(ind, n) (pte_t *)(pgdir_base(ind) + (n)*PAGE_SIZE)
#define get_page_dir(pdpti)    (pde_t *)(0xFFFFC000 + (pdpti)*PAGE_SIZE)

extern struct pdpt kernel_pdpt;

// PDPTs are small (32 bytes). Instead of wasting an entire page for each one,
// allocate them from a cache.
static struct slab_cache *pdpt_cache;

#if CONFIG(X86_NX)
static pteval_t page_nx_flags = 0;
#endif  // CONFIG(X86_NX)

static __always_inline void get_paging_indices(addr_t virt,
                                               size_t *pdpti,
                                               size_t *pdi,
                                               size_t *pti)
{
    *pdpti = PDPT_INDEX(virt);
    *pdi = PGDIR_INDEX(virt);
    *pti = PGTBL_INDEX(virt);
}

static pdpte_t *get_pdpt(void)
{
    struct task *curr = current_task();
    struct pdpt *pdpt =
        curr && curr->vmm ? curr->vmm->paging_ctx : &kernel_pdpt;
    return pdpt->entries;
}

/*
 * pgtbl_entry:
 * Return a pointer to the page table entry representing the specified address.
 */
static pte_t *pgtbl_entry(addr_t virt)
{
    size_t pdpti, pdi, pti;
    pde_t *pgdir;
    pte_t *pgtbl;

    get_paging_indices(virt, &pdpti, &pdi, &pti);

    pdpte_t *pdpt = get_pdpt();
    if (!(PDPTE(pdpt[pdpti]) & PAGE_PRESENT)) {
        return NULL;
    }

    pgdir = get_page_dir(pdpti);

    if (PDE(pgdir[pdi]) & PAGE_PRESENT) {
        pgtbl = get_page_table(pdpti, pdi);
        return pgtbl + pti;
    }

    return NULL;
}

static int add_page_directory(pdpte_t *pdpt, size_t pdpti)
{
    struct page *p = alloc_page(PA_PAGETABLE);
    if (IS_ERR(p)) {
        return ERR_VAL(p);
    }

    pdpt[pdpti] = make_pdpte(page_to_phys(p) | PAGE_PRESENT);
    memset(get_page_dir(pdpti), 0, PAGE_SIZE);

    return 0;
}

void i386_set_pde(addr_t virt, pde_t pde)
{
    pde_t *pgdir = get_page_dir(PDPT_INDEX(virt));
    pgdir[PGDIR_INDEX(virt)] = pde;
}

static int __map_page(addr_t virt, paddr_t phys, pteval_t flags)
{
    size_t pdpti, pdi, pti;
    pde_t *pgdir;
    pte_t *pgtbl;

    // Addresses must be page-aligned.
    if (!ALIGNED(virt, PAGE_SIZE) || !ALIGNED(phys, PAGE_SIZE)) {
        return EINVAL;
    }

    get_paging_indices(virt, &pdpti, &pdi, &pti);

    pdpte_t *pdpt = get_pdpt();
    if (!(PDPTE(pdpt[pdpti]) & PAGE_PRESENT)) {
        int err = add_page_directory(pdpt, pdpti);
        if (err) {
            return err;
        }
    }

    pgdir = get_page_dir(pdpti);
    pgtbl = get_page_table(pdpti, pdi);
    return ___map_page(pgdir, pgtbl, pdi, pti, phys, flags);
}

static int __map_pages_vmm(const struct vmm_space *vmm,
                           addr_t virt,
                           paddr_t phys,
                           size_t num_pages,
                           pteval_t flags)
{
    int status = 0;
    pdpte_t *pdpt = ((struct pdpt *)vmm->paging_ctx)->entries;

    // At any point in time, only one page directory and page table is accessed.
    // Allocate virtual addresses for these up front, and remap them to physical
    // addresses as needed.
    pde_t *pgdir = vmalloc(PAGE_SIZE);
    if (!pgdir) {
        return ENOMEM;
    }
    pte_t *pgtbl = vmalloc(PAGE_SIZE);
    if (!pgtbl) {
        vfree(pgdir);
        return ENOMEM;
    }

    // Indices of the previously accessed PDPT entry and PD entry, used to track
    // when mapping into a new paging structure.
    size_t prev_pdpti = UINT_MAX;
    size_t prev_pdi = UINT_MAX;

    // Map the address space's kernel page directory into the current space so
    // it can be updated with recursive mappings if new page directories are
    // allocated.
    const paddr_t kernel_pd_phys = PDPTE(pdpt[PDPT_ENTRY_C0]) & PAGE_MASK;
    pde_t *kernel_pd = vmalloc(PAGE_SIZE);
    if (!kernel_pd) {
        vfree(pgtbl);
        vfree(pgdir);
        return ENOMEM;
    }

    status = map_page_kernel(
        (addr_t)kernel_pd, kernel_pd_phys, PROT_WRITE, PAGE_CP_UNCACHEABLE);
    if (status) {
        vfree(kernel_pd);
        vfree(pgtbl);
        vfree(pgdir);
        return status;
    }

    for (size_t i = 0; i < num_pages;
         ++i, virt += PAGE_SIZE, phys += PAGE_SIZE) {
        size_t pdpti, pdi, pti;
        get_paging_indices(virt, &pdpti, &pdi, &pti);

        // Check if advancing to a new page directory; if so, map it into the
        // kernel address space.
        if (pdpti != prev_pdpti) {
            // Unmap the previous page directory.
            unmap_pages((addr_t)pgdir, 1);

            bool allocated_pgdir = false;

            if (!(PDPTE(pdpt[pdpti]) & PAGE_PRESENT)) {
                // Allocate a new page directory for the PDPT.
                struct page *p = alloc_page(PA_PAGETABLE);
                if (IS_ERR(p)) {
                    status = ERR_VAL(p);
                    break;
                }

                const addr_t pgdir_phys = page_to_phys(p);
                pdpt[pdpti] = make_pdpte(pgdir_phys | PAGE_PRESENT);

                // Recursively map the newly-allocated directory into the
                // address space.
                const size_t recursive_index = (PTRS_PER_PGDIR - 4) + pdpti;
                kernel_pd[recursive_index] =
                    make_pde(pgdir_phys | PAGE_RW | PAGE_PRESENT);

                allocated_pgdir = true;
            }

            const paddr_t directory = PDPTE(pdpt[pdpti]) & PAGE_MASK;
            if ((status = map_page_kernel((addr_t)pgdir,
                                          directory,
                                          PROT_WRITE,
                                          PAGE_CP_UNCACHEABLE))) {
                break;
            }

            if (allocated_pgdir) {
                // A newly-allocated page directory should be zeroed.
                memset(pgdir, 0, PAGE_SIZE);
            }
        }

        // Check if advancing to a new page table; if so, map it.
        if (prev_pdi != pdi) {
            unmap_pages((addr_t)pgtbl, 1);
            if ((status = load_and_map_page_table(pgdir, pdi, pgtbl))) {
                break;
            }
        }

        pgtbl[pti] = make_pte(phys | flags | PAGE_PRESENT);

        prev_pdpti = pdpti;
        prev_pdi = pdi;
    }

    unmap_pages((addr_t)pgtbl, 1);
    vfree(pgtbl);

    unmap_pages((addr_t)pgdir, 1);
    vfree(pgdir);

    unmap_pages((addr_t)kernel_pd, 1);
    vfree(kernel_pd);

    return status;
}

/*
 * i386_unmap_pages:
 * Unmap `n` pages, starting from address `virt`.
 */
int i386_unmap_pages(addr_t virt, size_t n)
{
    pde_t *pgdir;
    pte_t *pgtbl;
    size_t pdpti, pdi;
    int unmapped;

    if (!ALIGNED(virt, PAGE_SIZE))
        return EINVAL;

    pdpti = PDPT_INDEX(virt);
    pdi = PGDIR_INDEX(virt);
    pgdir = get_page_dir(pdpti);

    pdpte_t *pdpt = get_pdpt();
    if (!(PDPTE(pdpt[pdpti]) & PAGE_PRESENT)) {
        return EINVAL;
    }

    if (!(PDE(pgdir[pdi]) & PAGE_PRESENT)) {
        return EINVAL;
    }

    pgtbl = get_page_table(pdpti, pdi);
    while (n) {
        unmapped = __unmap_pages(pgdir, pdi, pgtbl, virt, n);
        n -= unmapped;
        virt += unmapped * PAGE_SIZE;

        // Advance to the next page directory.
        if (++pdi == PTRS_PER_PGDIR) {
            if (++pdpti == PTRS_PER_PDPT) {
                break;
            }
            if (!(PDPTE(pdpt[pdpti]) & PAGE_PRESENT)) {
                break;
            }
            pgdir = get_page_dir(pdpti);
            pdi = 0;
        }
        if (!(PDE(pgdir[pdi]) & PAGE_PRESENT)) {
            break;
        }
        pgtbl = get_page_table(pdpti, pdi);
    }

    return 0;
}

int arch_vmm_setup(struct vmm_space *vmm)
{
    struct pdpt *p = alloc_cache(pdpt_cache);
    if (IS_ERR(p)) {
        return ERR_VAL(p);
    }

    // Clone the kernel's page directory for the process, excluding the final
    // four entries, which are the recursively mapped page directories.
    pde_t *kernel_pd = clone_kernel_pgdir(0, PTRS_PER_PGDIR - 4);
    if (IS_ERR(kernel_pd)) {
        return ERR_VAL(kernel_pd);
    }

    const paddr_t phys = virt_to_phys(kernel_pd);

    p->entries[PDPT_ENTRY_C0] = make_pdpte(phys | PAGE_PRESENT);

    // Recursively map the cloned directory.
    kernel_pd[PTRS_PER_PGDIR - 1] = make_pde(phys | PAGE_RW | PAGE_PRESENT);

    unmap_cloned_pgdir(kernel_pd);

    vmm->paging_base = virt_to_phys(p);
    vmm->paging_ctx = p;

    return 0;
}

void arch_vmm_release(struct vmm_space *vmm)
{
    struct pdpt *pdpt = vmm->paging_ctx;

    for (size_t i = 0; i < PTRS_PER_PDPT; ++i) {
        pdpteval_t value = PDPTE(pdpt->entries[i]);
        if (!(value & PAGE_PRESENT)) {
            continue;
        }

        const addr_t phys = value & PAGE_MASK;

        if (i != PDPT_ENTRY_C0) {
            // Free all allocated page tables from the non-kernel directories.
            free_page_directory(phys, 0, PTRS_PER_PGDIR);
        }

        free_pages(phys_to_page(phys));
    }

    free_cache(pdpt_cache, pdpt);
}

#else  // CONFIG(X86_PAE)

#define get_page_table(n) (pte_t *)(PAGING_BASE + ((n)*PAGE_SIZE))

/* The page directory of a legacy 2-level x86 paging setup. */
pde_t *const pgdir = (pde_t *)PAGING_VADDR;

static __always_inline void get_paging_indices(addr_t virt,
                                               size_t *pdi,
                                               size_t *pti)
{
    *pdi = PGDIR_INDEX(virt);
    *pti = PGTBL_INDEX(virt);
}

/*
 * pgtbl_entry:
 * Return a pointer to the page table entry representing the specified address.
 */
static pte_t *pgtbl_entry(addr_t virt)
{
    size_t pdi, pti;
    pte_t *pgtbl;

    get_paging_indices(virt, &pdi, &pti);
    if (PDE(pgdir[pdi]) & PAGE_PRESENT) {
        pgtbl = get_page_table(pdi);
        return pgtbl + pti;
    }

    return NULL;
}

void i386_set_pde(addr_t virt, pde_t pde) { pgdir[PGDIR_INDEX(virt)] = pde; }

static int __map_page(addr_t virt, paddr_t phys, pteval_t flags)
{
    size_t pdi, pti;
    pte_t *pgtbl;

    /* addresses must be page-aligned */
    if (!ALIGNED(virt, PAGE_SIZE) || !ALIGNED(phys, PAGE_SIZE))
        return EINVAL;

    get_paging_indices(virt, &pdi, &pti);
    pgtbl = get_page_table(pdi);
    return ___map_page(pgdir, pgtbl, pdi, pti, phys, flags);
}

static int __map_pages_vmm(const struct vmm_space *vmm,
                           addr_t virt,
                           paddr_t phys,
                           size_t num_pages,
                           pteval_t flags)
{
    int status = 0;

    // At any point in time, only one page directory and page table is accessed.
    // Allocate virtual addresses for these up front, and remap them to physical
    // addresses as needed.
    pde_t *pgdir = vmalloc(PAGE_SIZE);
    if (!pgdir) {
        return ENOMEM;
    }
    if ((status = map_page_kernel((addr_t)pgdir,
                                  vmm->paging_base,
                                  PROT_WRITE,
                                  PAGE_CP_UNCACHEABLE))) {
        vfree(pgdir);
        return status;
    }

    pte_t *pgtbl = vmalloc(PAGE_SIZE);
    if (!pgtbl) {
        unmap_pages((addr_t)pgdir, 1);
        vfree(pgdir);
        return ENOMEM;
    }

    // Index of the previously access page directory entry, used to track when
    // mapping into a new page table.
    size_t prev_pdi = UINT_MAX;

    for (size_t i = 0; i < num_pages;
         ++i, virt += PAGE_SIZE, phys += PAGE_SIZE) {
        size_t pdi, pti;
        get_paging_indices(virt, &pdi, &pti);

        // Check if advancing to a new page table; if so, map it.
        if (prev_pdi != pdi) {
            unmap_pages((addr_t)pgtbl, 1);
            if ((status = load_and_map_page_table(pgdir, pdi, pgtbl))) {
                break;
            }
        }

        pgtbl[pti] = make_pte(phys | flags | PAGE_PRESENT);
        prev_pdi = pdi;
    }

    unmap_pages((addr_t)pgtbl, 1);
    vfree(pgtbl);

    unmap_pages((addr_t)pgdir, 1);
    vfree(pgdir);

    return status;
}

/*
 * i386_unmap_pages:
 * Unmap `n` pages, starting from address `virt`.
 */
int i386_unmap_pages(addr_t virt, size_t n)
{
    pte_t *pgtbl;
    size_t pdi;
    int unmapped;

    if (!ALIGNED(virt, PAGE_SIZE))
        return EINVAL;

    pdi = PGDIR_INDEX(virt);
    if (!(PDE(pgdir[pdi]) & PAGE_PRESENT))
        return EINVAL;

    pgtbl = get_page_table(pdi);
    while (n) {
        unmapped = __unmap_pages(pgdir, pdi, pgtbl, virt, n);
        n -= unmapped;
        virt += unmapped * PAGE_SIZE;

        if (++pdi == PTRS_PER_PGDIR)
            break;
        if (!(PDE(pgdir[pdi]) & PAGE_PRESENT))
            break;
        pgtbl = get_page_table(pdi);
    }

    return 0;
}

int arch_vmm_setup(struct vmm_space *vmm)
{
    pde_t *kernel_pd = clone_kernel_pgdir(PGDIR_INDEX(KERNEL_VIRTUAL_BASE),
                                          PTRS_PER_PGDIR - 1);

    // Recursively map the cloned directory.
    const paddr_t phys = virt_to_phys(kernel_pd);
    kernel_pd[PTRS_PER_PGDIR - 1] = make_pde(phys | PAGE_RW | PAGE_PRESENT);

    unmap_cloned_pgdir(kernel_pd);

    vmm->paging_base = phys;
    vmm->paging_ctx = NULL;

    return 0;
}

void arch_vmm_release(struct vmm_space *vmm)
{
    free_page_directory(vmm->paging_base, 0, PGDIR_INDEX(KERNEL_VIRTUAL_BASE));
    free_pages(phys_to_page(vmm->paging_base));
}

#endif  // CONFIG(X86_PAE)

/*
 * i386_virt_to_phys:
 * Return the physical address to which the specified virtual address
 * is mapped.
 */
paddr_t i386_virt_to_phys(addr_t addr)
{
    pte_t *pte;

    pte = pgtbl_entry(addr);
    if (!pte || !(PTE(*pte) & PAGE_PRESENT))
        return ~0;

    return (PTE(*pte) & PAGE_MASK) + (addr & ~PAGE_MASK);
}

/*
 * i386_addr_mapped:
 * Return 1 if address `virt` has been mapped to a physical address.
 */
int i386_addr_mapped(addr_t virt)
{
    pte_t *pte;

    pte = pgtbl_entry(virt);
    return pte ? PTE(*pte) & PAGE_PRESENT : 0;
}

static int mp_args_to_flags(pteval_t *flags, int prot, enum cache_policy cp);

/*
 * i386_map_page_kernel:
 * Map a page with base virtual address `virt` to physical address `phys`
 * for kernel use.
 */
int i386_map_page_kernel(addr_t virt,
                         paddr_t phys,
                         int prot,
                         enum cache_policy cp)
{
    pteval_t flags;
    int err;

    if ((err = mp_args_to_flags(&flags, prot, cp)) != 0)
        return err;

    if ((err = __map_page(virt, phys, flags | PAGE_GLOBAL))) {
        return err;
    }

    tlb_flush_page_lazy(virt);
    return 0;
}

/*
 * i386_map_page_user:
 * Map a page with base virtual address `virt` to physical address `phys`
 * for userspace.
 */
int i386_map_page_user(addr_t virt,
                       paddr_t phys,
                       int prot,
                       enum cache_policy cp)
{
    pteval_t flags;
    int err;

    if ((err = mp_args_to_flags(&flags, prot, cp)) != 0)
        return err;

    return __map_page(virt, phys, flags | PAGE_USER);
}

int i386_map_pages(addr_t virt,
                   paddr_t phys,
                   size_t num_pages,
                   int prot,
                   enum cache_policy cp,
                   bool user)
{
    pteval_t flags;
    int err;

    if ((err = mp_args_to_flags(&flags, prot, cp)) != 0) {
        return err;
    }

    flags |= user ? PAGE_USER : PAGE_GLOBAL;

    for (size_t i = 0; i < num_pages;
         ++i, virt += PAGE_SIZE, phys += PAGE_SIZE) {
        if ((err = __map_page(virt, phys, flags)) != 0) {
            return err;
        }
    }

    return 0;
}

int i386_map_pages_vmm(const struct vmm_space *vmm,
                       addr_t virt,
                       paddr_t phys,
                       size_t num_pages,
                       unsigned long prot,
                       enum cache_policy cp)
{
    pteval_t flags;
    int err = mp_args_to_flags(&flags, prot, cp);
    if (err != 0) {
        return err;
    }

    flags |= PAGE_USER;

    return __map_pages_vmm(vmm, virt, phys, num_pages, flags);
}

/*
 * cp_to_flags:
 * Convert a cache policy to x86 page flags.
 * `flags` must already be initialized.
 */
static int cp_to_flags(pteval_t *flags, enum cache_policy cp)
{
    switch (cp) {
    case PAGE_CP_DEFAULT:
    case PAGE_CP_WRITE_BACK:
        *flags &= ~(PAGE_PAT | PAGE_PCD | PAGE_PWT);
        break;
    case PAGE_CP_WRITE_THROUGH:
        *flags |= PAGE_PWT;
        *flags &= ~(PAGE_PCD | PAGE_PAT);
        break;
    case PAGE_CP_UNCACHED:
        *flags |= PAGE_PCD;
        *flags &= ~(PAGE_PWT | PAGE_PAT);
        break;
    case PAGE_CP_UNCACHEABLE:
        *flags |= PAGE_PCD | PAGE_PWT;
        *flags &= ~PAGE_PAT;
        break;
    case PAGE_CP_WRITE_COMBINING:
        *flags |= PAGE_PAT;
        *flags &= ~(PAGE_PCD | PAGE_PWT);
        break;
    case PAGE_CP_WRITE_PROTECTED:
        *flags |= PAGE_PAT | PAGE_PWT;
        *flags &= ~PAGE_PCD;
        break;
    default:
        return EINVAL;
    }

    return 0;
}

static int mp_args_to_flags(pteval_t *flags, int prot, enum cache_policy cp)
{
    *flags = 0;

    if (prot & PROT_WRITE) {
        *flags |= PAGE_RW;
    }

#if CONFIG(X86_NX)
    if (!(prot & PROT_EXEC)) {
        *flags |= page_nx_flags;
    }
#endif  // CONFIG(X86_NX)

    return cp_to_flags(flags, cp);
}

/*
 * i386_set_cache_policy:
 * Set the CPU caching policy for a single virtual page.
 */
int i386_set_cache_policy(addr_t virt, enum cache_policy policy)
{
    pte_t *pte;
    pteval_t pteval;
    int err;

    pte = pgtbl_entry(virt);
    if (!pte || !(PTE(*pte) & PAGE_PRESENT))
        return EINVAL;

    /* WC and WP cache policies are only available through PAT. */
    if (!cpu_supports(CPUID_PAT) && (policy == PAGE_CP_WRITE_COMBINING ||
                                     policy == PAGE_CP_WRITE_PROTECTED))
        policy = PAGE_CP_WRITE_BACK;

    pteval = PTE(*pte);
    if ((err = cp_to_flags(&pteval, policy)) != 0)
        return err;

    *pte = make_pte(pteval);
    tlb_flush_page_lazy(virt);

    return 0;
}

void i386_switch_address_space(struct vmm_space *vmm)
{
    if (vmm) {
        cpu_write_cr3(vmm->paging_base);
    }
}

void arch_vmm_init(struct vmm_space *kernel_vmm_space)
{
    kernel_vmm_space->paging_base = cpu_read_cr3();

#if CONFIG(X86_PAE)
    kernel_vmm_space->paging_ctx = &kernel_pdpt;

    pdpt_cache = create_cache("pdpt_cache",
                              sizeof(struct pdpt),
                              sizeof(struct pdpt),
                              SLAB_PANIC,
                              NULL);
#endif  // CONFIG(X86_PAE)
}

int cpu_paging_init(bool is_bootstrap_processor)
{
#if CONFIG(X86_NX)
    // Check to see whether the early boot code enabled NX protections. In a
    // kernel compiled with CONFIG_X86_NX, this will happen if the CPU supports
    // it.
    bool nx_enabled = false;

    if (cpu_supports_extended(CPUID_EXT_NXE)) {
        uint32_t eax, edx;
        rdmsr(IA32_EFER, &eax, &edx);
        nx_enabled = (eax & IA32_EFER_NXE) != 0;
    }

    if (is_bootstrap_processor) {
        page_nx_flags = nx_enabled ? PAGE_NX : 0;
    } else if (page_nx_flags == PAGE_NX && !nx_enabled) {
        int cpu = processor_id();
        klog(KLOG_ERROR,
             "CPU0 activated NX memory protection, but CPU%d cannot.",
             cpu);
        klog(KLOG_ERROR, "Such a system configuration is not supported.");
        klog(KLOG_ERROR, "Shutting down CPU%d.", cpu);
        klog(KLOG_ERROR,
             "Run a kernel compiled without CONFIG_X86_NX to use all "
             "processors on this system.",
             cpu);
        return 1;
    }
#endif  // CONFIG(X86_NX)

    return 0;
}
