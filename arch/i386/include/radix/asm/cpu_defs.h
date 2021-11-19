/*
 * arch/i386/include/radix/asm/cpu_defs.h
 * Copyright (C) 2016-2017 Alexei Frolov
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

#ifndef ARCH_I386_RADIX_CPU_DEFS_H
#define ARCH_I386_RADIX_CPU_DEFS_H

#define CPU_VENDOR_AMD       "AuthenticAMD"
#define CPU_VENDOR_CENTAUR   "CentaurHauls"
#define CPU_VENDOR_CYRIX     "CyrixInstead"
#define CPU_VENDOR_INTEL     "GenuineIntel"
#define CPU_VENDOR_TRANSMETA "GenuineTMx86"
#define CPU_VENDOR_NEXGEN    "NexGenDriven"
#define CPU_VENDOR_RISE      "RiseRiseRise"
#define CPU_VENDOR_SIS       "SiS SiS SiS "
#define CPU_VENDOR_UMC       "UMC UMC UMC "
#define CPU_VENDOR_VIA       "VIA VIA VIA "
#define CPU_VENDOR_VORTEX    "Vortex86 SoC"

#define CPU_VENDOR_KVM       "KVMKVMKVM"
#define CPU_VENDOR_HYPERV    "Microsoft Hv"
#define CPU_VENDOR_VMWARE    "VMwareWMware"
#define CPU_VENDOR_PARALLELS " lrpepyh vr"
#define CPU_VENDOR_XENHVM    "XenVMMXenVMM"

/* cpuid 0x1 EDX bits */
#define CPUID_FPU   (1ULL << 0)
#define CPUID_VME   (1ULL << 1)
#define CPUID_DE    (1ULL << 2)
#define CPUID_PSE   (1ULL << 3)
#define CPUID_TSC   (1ULL << 4)
#define CPUID_MSR   (1ULL << 5)
#define CPUID_PAE   (1ULL << 6)
#define CPUID_MCE   (1ULL << 7)
#define CPUID_CX8   (1ULL << 8)
#define CPUID_APIC  (1ULL << 9)
#define CPUID_SEP   (1ULL << 11)
#define CPUID_MTRR  (1ULL << 12)
#define CPUID_PGE   (1ULL << 13)
#define CPUID_MCA   (1ULL << 14)
#define CPUID_CMOV  (1ULL << 15)
#define CPUID_PAT   (1ULL << 16)
#define CPUID_PSE36 (1ULL << 17)
#define CPUID_PSN   (1ULL << 18)
#define CPUID_CLFSH (1ULL << 19)
#define CPUID_DS    (1ULL << 21)
#define CPUID_ACPI  (1ULL << 22)
#define CPUID_MMX   (1ULL << 23)
#define CPUID_FXSR  (1ULL << 24)
#define CPUID_SSE   (1ULL << 25)
#define CPUID_SSE2  (1ULL << 26)
#define CPUID_SS    (1ULL << 27)
#define CPUID_HTT   (1ULL << 28)
#define CPUID_TM    (1ULL << 29)
#define CPUID_IA64  (1ULL << 30)
#define CPUID_PBE   (1ULL << 31)

/* cpuid 0x1 ECX bits */
#define CPUID_SSE3      (1ULL << 32)
#define CPUID_PCLMULQDQ (1ULL << 33)
#define CPUID_DTES64    (1ULL << 34)
#define CPUID_MONITOR   (1ULL << 35)
#define CPUID_DS_CPL    (1ULL << 36)
#define CPUID_VMX       (1ULL << 37)
#define CPUID_SMX       (1ULL << 38)
#define CPUID_EST       (1ULL << 39)
#define CPUID_TM2       (1ULL << 40)
#define CPUID_SSSE3     (1ULL << 41)
#define CPUID_CNXTID    (1ULL << 42)
#define CPUID_SDBG      (1ULL << 43)
#define CPUID_FMA       (1ULL << 44)
#define CPUID_CX16      (1ULL << 45)
#define CPUID_XTPR      (1ULL << 46)
#define CPUID_PDCM      (1ULL << 47)
#define CPUID_PCID      (1ULL << 49)
#define CPUID_DCA       (1ULL << 50)
#define CPUID_SSE4_1    (1ULL << 51)
#define CPUID_SSE4_2    (1ULL << 52)
#define CPUID_X2APIC    (1ULL << 53)
#define CPUID_MOVBE     (1ULL << 54)
#define CPUID_POPCNT    (1ULL << 55)
#define CPUID_TSC_DL    (1ULL << 56)
#define CPUID_AES       (1ULL << 57)
#define CPUID_XSAVE     (1ULL << 58)
#define CPUID_OSXSAVE   (1ULL << 59)
#define CPUID_AVX       (1ULL << 60)
#define CPUID_F16C      (1ULL << 61)
#define CPUID_RDRND     (1ULL << 62)
#define CPUID_HYPER     (1ULL << 63)

/* EFLAGS register bits */
#define EFLAGS_CF   (1 << 0)
#define EFLAGS_PF   (1 << 2)
#define EFLAGS_AF   (1 << 4)
#define EFLAGS_ZF   (1 << 6)
#define EFLAGS_SF   (1 << 7)
#define EFLAGS_TF   (1 << 8)
#define EFLAGS_IF   (1 << 9)
#define EFLAGS_DF   (1 << 10)
#define EFLAGS_OF   (1 << 11)
#define EFLAGS_IOPL ((1 << 12) | (1 << 13))
#define EFLAGS_NT   (1 << 14)
#define EFLAGS_RF   (1 << 16)
#define EFLAGS_VM   (1 << 17)
#define EFLAGS_AC   (1 << 18)
#define EFLAGS_VIF  (1 << 19)
#define EFLAGS_VIP  (1 << 20)
#define EFLAGS_ID   (1 << 21)

/* CR0 bits */
#define CR0_PE (1 << 0)
#define CR0_MP (1 << 1)
#define CR0_EM (1 << 2)
#define CR0_TS (1 << 3)
#define CR0_ET (1 << 4)
#define CR0_NE (1 << 5)
#define CR0_WP (1 << 16)
#define CR0_AM (1 << 18)
#define CR0_NW (1 << 29)
#define CR0_CD (1 << 30)
#define CR0_PG (1 << 31)

/* CR4 bits */
#define CR4_VME        (1 << 0)
#define CR4_PVI        (1 << 1)
#define CR4_TSD        (1 << 2)
#define CR4_DE         (1 << 3)
#define CR4_PSE        (1 << 4)
#define CR4_PAE        (1 << 5)
#define CR4_MCE        (1 << 6)
#define CR4_PGE        (1 << 7)
#define CR4_PCE        (1 << 8)
#define CR4_OSFXSR     (1 << 9)
#define CR4_OSXMMEXCPT (1 << 10)
#define CR4_VMXE       (1 << 13)
#define CR4_SMXE       (1 << 14)
#define CR4_FSGSBASE   (1 << 16)
#define CR4_PCIDE      (1 << 17)
#define CR4_OSXSAVE    (1 << 18)
#define CR4_SMEP       (1 << 20)
#define CR4_SMAP       (1 << 21)
#define CR4_PKE        (1 << 22)

#include <radix/compiler.h>

#ifdef __KERNEL__

static __always_inline unsigned long cpu_read_cr2(void)
{
    unsigned long ret;

    asm volatile("mov %%cr2, %0" : "=r"(ret));
    return ret;
}

static __always_inline void cpu_write_cr3(unsigned long val)
{
    asm volatile("mov %0, %%cr3" : : "r"(val));
}

#define cpuid(eax, a, b, c, d)               \
    asm volatile(                            \
        "xchg %%ebx, %1\n\t"                 \
        "cpuid\n\t"                          \
        "xchg %%ebx, %1"                     \
        : "=a"(a), "=r"(b), "=c"(c), "=d"(d) \
        : "0"(eax))

#define __modify_control_register(cr, clear, set) \
    asm volatile("movl %%" cr                     \
                 ", %%eax\n\t"                    \
                 "andl %0, %%eax\n\t"             \
                 "orl %1, %%eax\n\t"              \
                 "movl %%eax, %%" cr              \
                 :                                \
                 : "r"(~clear), "r"(set)          \
                 : "%eax", "%edx")

#define cpu_modify_cr0(clear, set) __modify_control_register("cr0", clear, set)

#define cpu_modify_cr4(clear, set) __modify_control_register("cr4", clear, set)

static __always_inline unsigned long cpu_read_flags(void)
{
    unsigned long ret;

    asm volatile(
        "pushf\n\t"
        "pop %0"
        : "=g"(ret));
    return ret;
}

static __always_inline void cpu_update_flags(unsigned long clear,
                                             unsigned long set)
{
    asm volatile(
        "pushf\n\t"
        "andl %0, (%%esp)\n\t"
        "orl %1, (%%esp)\n\t"
        "popf"
        :
        : "r"(~clear), "r"(set));
}

#define cpu_pause() asm volatile("pause")

#endif /* __KERNEL__ */

#endif /* ARCH_I386_RADIX_CPU_DEFS_H */
