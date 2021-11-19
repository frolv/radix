/*
 * arch/i386/mm/pagetable.c
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

#include <radix/config.h>
#include <radix/cpu.h>
#include <radix/kernel.h>
#include <radix/mm.h>
#include <radix/vmm.h>

#include <rlibc/string.h>

static int ___map_page(pde_t *pgdir,
                       pte_t *pgtbl,
                       size_t pdi,
                       size_t pti,
                       addr_t virt,
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

        pgdir[pdi] =
            make_pde(page_to_phys(new) | PAGE_GLOBAL | PAGE_RW | PAGE_PRESENT);
        tlb_flush_page_lazy((addr_t)pgtbl);
        memset(pgtbl, 0, PGTBL_SIZE);
    }
    pgtbl[pti] = make_pte(phys | flags | PAGE_PRESENT);
    tlb_flush_page_lazy(virt);

    return 0;
}

/*
 * __unmap_pages:
 * Unmap `n` pages starting at address `virt` from the specified page table,
 * which has index `pdi` in the given page directory.
 */
int __unmap_pages(pde_t *pgdir, size_t pdi, pte_t *pgtbl, addr_t virt, size_t n)
{
    size_t initial_pti, curr_pti;
    paddr_t phys;
    int unmapped;

    initial_pti = PGTBL_INDEX(virt);
    /* check if any previous pages in the current table are mapped */
    for (curr_pti = 0; curr_pti < initial_pti; ++curr_pti) {
        if (PTE(pgtbl[curr_pti]) & PAGE_PRESENT)
            break;
    }
    if (curr_pti == initial_pti)
        initial_pti = 0;
    else
        curr_pti = initial_pti;

    unmapped = 0;
    while (n) {
        pgtbl[curr_pti] = make_pte(0);
        tlb_flush_page_lazy(virt);

        --n;
        ++unmapped;
        virt += PAGE_SIZE;
        if (++curr_pti == PTRS_PER_PGTBL) {
            if (initial_pti == 0) {
                /* all pages in the table are unmapped */
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

#if CONFIG(X86_PAE)

#define pgdir_base(n)          (addr_t)(0xFF800000 + (n)*MIB(2))
#define get_page_table(ind, n) (pte_t *)(pgdir_base(ind) + (n)*PAGE_SIZE)
#define get_page_dir(pdpti)    (pde_t *)(0xFFFFC000 + (pdpti)*PAGE_SIZE)

static __always_inline void get_paging_indices(addr_t virt,
                                               size_t *pdpti,
                                               size_t *pdi,
                                               size_t *pti)
{
    *pdpti = PDPT_INDEX(virt);
    *pdi = PGDIR_INDEX(virt);
    *pti = PGTBL_INDEX(virt);
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
    pgdir = get_page_dir(pdpti);

    if (PDE(pgdir[pdi]) & PAGE_PRESENT) {
        pgtbl = get_page_table(pdpti, pdi);
        return pgtbl + pti;
    }

    return NULL;
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

    /* addresses must be page-aligned */
    if (!ALIGNED(virt, PAGE_SIZE) || !ALIGNED(phys, PAGE_SIZE))
        return EINVAL;

    get_paging_indices(virt, &pdpti, &pdi, &pti);
    pgdir = get_page_dir(pdpti);
    pgtbl = get_page_table(pdpti, pdi);
    return ___map_page(pgdir, pgtbl, pdi, pti, virt, phys, flags);
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

    if (!(PDE(pgdir[pdi]) & PAGE_PRESENT))
        return EINVAL;

    pgtbl = get_page_table(pdpti, pdi);
    while (n) {
        unmapped = __unmap_pages(pgdir, pdi, pgtbl, virt, n);
        n -= unmapped;
        virt += unmapped * PAGE_SIZE;

        /* advance to the next page directory */
        if (++pdi == PTRS_PER_PGDIR) {
            if (++pdpti == PTRS_PER_PDPT)
                break;
            pgdir = get_page_dir(pdpti);
            pdi = 0;
        }
        if (!(PDE(pgdir[pdi]) & PAGE_PRESENT))
            break;
        pgtbl = get_page_table(pdpti, pdi);
    }

    return 0;
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
    return ___map_page(pgdir, pgtbl, pdi, pti, virt, phys, flags);
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

static int mp_args_to_flags(pteval_t *flags, int prot, int cp);

/*
 * i386_map_page_kernel:
 * Map a page with base virtual address `virt` to physical address `phys`
 * for kernel use.
 */
int i386_map_page_kernel(addr_t virt, paddr_t phys, int prot, int cp)
{
    pteval_t flags;
    int err;

    if ((err = mp_args_to_flags(&flags, prot, cp)) != 0)
        return err;

    return __map_page(virt, phys, flags | PAGE_GLOBAL);
}

/*
 * i386_map_page_user:
 * Map a page with base virtual address `virt` to physical address `phys`
 * for userspace.
 */
int i386_map_page_user(addr_t virt, paddr_t phys, int prot, int cp)
{
    pteval_t flags;
    int err;

    if ((err = mp_args_to_flags(&flags, prot, cp)) != 0)
        return err;

    return __map_page(virt, phys, flags | PAGE_USER);
}

int i386_map_pages(
    addr_t virt, paddr_t phys, int prot, int cp, int user, size_t n)
{
    pteval_t flags;
    int err;

    if ((err = mp_args_to_flags(&flags, prot, cp)) != 0)
        return err;

    flags |= user ? PAGE_USER : PAGE_GLOBAL;

    for (; n; --n, virt += PAGE_SIZE, phys += PAGE_SIZE) {
        if ((err = __map_page(virt, phys, flags)) != 0)
            return err;
    }

    return 0;
}

/*
 * cp_to_flags:
 * Convert a cache policy to x86 page flags.
 * `flags` must already be initialized.
 */
static int cp_to_flags(pteval_t *flags, int cp)
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

static int mp_args_to_flags(pteval_t *flags, int prot, int cp)
{
    *flags = 0;

    if (prot == PROT_WRITE)
        *flags = PAGE_RW;
    else if (prot != PROT_READ)
        return EINVAL;

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
    if (!vmm)
        return;

    cpu_write_cr3(vmm->paging_base);
}
