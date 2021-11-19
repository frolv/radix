/*
 * arch/i386/bios.c
 * Copyright (C) 2017 Alexei Frolov
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

#include <radix/asm/bios.h>
#include <radix/kernel.h>
#include <radix/mm.h>

#include <rlibc/string.h>

#define EBDA_BASE_LOCATION_PHYS 0x0000040E
#define BIOS_REGION_PHYS_START  0x000E0000
#define BIOS_REGION_PHYS_END    0x00100000

#define EBDA_BASE_LOCATION phys_to_virt(EBDA_BASE_LOCATION_PHYS)
#define BIOS_REGION_START  phys_to_virt(BIOS_REGION_PHYS_START)
#define BIOS_REGION_END    phys_to_virt(BIOS_REGION_PHYS_END)

static addr_t ebda_base = 0;

/*
 * __find_sig_area:
 * Attempt to find signature `sig` in the memory region from start to end.
 */
static void *__find_sig_area(
    const char *sig, size_t size, size_t align, addr_t start, addr_t end)
{
    void *s;

    for (s = (void *)start; s < (void *)end; s += align) {
        if (memcmp(s, sig, size) == 0)
            return s;
    }

    return NULL;
}

/*
 * bios_find_signature:
 * Search for the signature `sig` in the BIOS data areas and return
 * a pointer to its location if found.
 * `sig_size` specifies the size of the signature.
 * `sig_align` specifies the signature's alignment.
 */
void *bios_find_signature(const char *sig, size_t sig_size, size_t sig_align)
{
    void *ret;

    if (!ebda_base)
        ebda_base = phys_to_virt((*(uint16_t *)EBDA_BASE_LOCATION) << 4);

    ret = __find_sig_area(
        sig, sig_size, sig_align, ebda_base, ebda_base + KIB(1));
    if (!ret)
        ret = __find_sig_area(
            sig, sig_size, sig_align, BIOS_REGION_START, BIOS_REGION_END);

    return ret;
}
