/*
 * include/radix/elf.h
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

#ifndef RADIX_ELF_H
#define RADIX_ELF_H

#include <radix/vmm.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define EI_NIDENT 16

#define EI_MAG0       0
#define EI_MAG1       1
#define EI_MAG2       2
#define EI_MAG3       3
#define EI_CLASS      4
#define EI_DATA       5
#define EI_VERSION    6
#define EI_OSABI      7
#define EI_ABIVERSION 8
#define EI_PAD        9

// ELF magic.
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

// ELF class (e_ident[EI_CLASS]).
#define ELFCLASSNONE 0
#define ELFCLASS32   1
#define ELFCLASS64   2

// ELF data (e_ident[EI_DATA]).
#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

// ELF OS ABI (e_ident[EI_OSABI]).
#define ELFOSABI_NONE       0
#define ELFOSABI_HPUX       1
#define ELFOSABI_NETBSD     2
#define ELFOSABI_LINUX      3
#define ELFOSABI_SOLARIS    6
#define ELFOSABI_AIX        7
#define ELFOSABI_IRIX       8
#define ELFOSABI_FREEBSD    9
#define ELFOSABI_TRU64      10
#define ELFOSABI_MODESTO    11
#define ELFOSABI_OPENBSD    12
#define ELFOSABI_OPENVMS    13
#define ELFOSABI_NSK        14
#define ELFOSABI_ARM_AEABI  64
#define ELFOSABI_RADIX      69
#define ELFOSABI_ARM        97
#define ELFOSABI_STANDALONE 255

// ELF type (e_type field).
#define ET_NONE   0
#define ET_REL    1
#define ET_EXEC   2
#define ET_DYN    3
#define ET_CORE   4
#define ET_LOOS   0xfe00
#define ET_HIOS   0xfeff
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

// ELF processor (e_machine field).
#define EM_NONE          0
#define EM_M32           1
#define EM_SPARC         2
#define EM_386           3
#define EM_68K           4
#define EM_88K           5
#define EM_860           7
#define EM_MIPS          8
#define EM_S370          9
#define EM_MIPS_RS3_LE   10
#define EM_PARISC        15
#define EM_VPP500        17
#define EM_SPARC32PLUS   18
#define EM_960           19
#define EM_PPC           20
#define EM_PPC64         21
#define EM_S390          22
#define EM_V800          36
#define EM_FR20          37
#define EM_RH32          38
#define EM_RCE           39
#define EM_ARM           40
#define EM_ALPHA         41
#define EM_SH            42
#define EM_SPARCV9       43
#define EM_TRICORE       44
#define EM_ARC           45
#define EM_H8_300        46
#define EM_H8_300H       47
#define EM_H8S           48
#define EM_H8_500        49
#define EM_IA_64         50
#define EM_MIPS_X        51
#define EM_COLDFIRE      52
#define EM_68HC12        53
#define EM_MMA           54
#define EM_PCP           55
#define EM_NCPU          56
#define EM_NDR1          57
#define EM_STARCORE      58
#define EM_ME16          59
#define EM_ST100         60
#define EM_TINYJ         61
#define EM_X86_64        62
#define EM_PDSP          63
#define EM_PDP10         64
#define EM_PDP11         65
#define EM_FX66          66
#define EM_ST9PLUS       67
#define EM_ST7           68
#define EM_68HC16        69
#define EM_68HC11        70
#define EM_68HC08        71
#define EM_68HC05        72
#define EM_SVX           73
#define EM_ST19          74
#define EM_VAX           75
#define EM_CRIS          76
#define EM_JAVELIN       77
#define EM_FIREPATH      78
#define EM_ZSP           79
#define EM_MMIX          80
#define EM_HUANY         81
#define EM_PRISM         82
#define EM_AVR           83
#define EM_FR30          84
#define EM_D10V          85
#define EM_D30V          86
#define EM_V850          87
#define EM_M32R          88
#define EM_MN10300       89
#define EM_MN10200       90
#define EM_PJ            91
#define EM_OPENRISC      92
#define EM_ARC_A5        93
#define EM_XTENSA        94
#define EM_VIDEOCORE     95
#define EM_TMM_GPP       96
#define EM_NS32K         97
#define EM_TPC           98
#define EM_SNP1K         99
#define EM_ST200         100
#define EM_IP2K          101
#define EM_MAX           102
#define EM_CR            103
#define EM_F2MC16        104
#define EM_MSP430        105
#define EM_BLACKFIN      106
#define EM_SE_C33        107
#define EM_SEP           108
#define EM_ARCA          109
#define EM_UNICORE       110
#define EM_EXCESS        111
#define EM_DXP           112
#define EM_ALTERA_NIOS2  113
#define EM_CRX           114
#define EM_XGATE         115
#define EM_C166          116
#define EM_M16C          117
#define EM_DSPIC30F      118
#define EM_CE            119
#define EM_M32C          120
#define EM_TSK3000       131
#define EM_RS08          132
#define EM_SHARC         133
#define EM_ECOG2         134
#define EM_SCORE7        135
#define EM_DSP24         136
#define EM_VIDEOCORE3    137
#define EM_LATTICEMICO32 138
#define EM_SE_C17        139
#define EM_TI_C6000      140
#define EM_TI_C2000      141
#define EM_TI_C5500      142
#define EM_TI_ARP32      143
#define EM_TI_PRU        144
#define EM_MMDSP_PLUS    160
#define EM_CYPRESS_M8C   161
#define EM_R32C          162
#define EM_TRIMEDIA      163
#define EM_QDSP6         164
#define EM_8051          165
#define EM_STXP7X        166
#define EM_NDS32         167
#define EM_ECOG1X        168
#define EM_MAXQ30        169
#define EM_XIMO16        170
#define EM_MANIK         171
#define EM_CRAYNV2       172
#define EM_RX            173
#define EM_METAG         174
#define EM_MCST_ELBRUS   175
#define EM_ECOG16        176
#define EM_CR16          177
#define EM_ETPU          178
#define EM_SLE9X         179
#define EM_L10M          180
#define EM_K10M          181
#define EM_AARCH64       183
#define EM_AVR32         185
#define EM_STM8          186
#define EM_TILE64        187
#define EM_TILEPRO       188
#define EM_MICROBLAZE    189
#define EM_CUDA          190
#define EM_TILEGX        191
#define EM_CLOUDSHIELD   192
#define EM_COREA_1ST     193
#define EM_COREA_2ND     194
#define EM_ARC_COMPACT2  195
#define EM_OPEN8         196
#define EM_RL78          197
#define EM_VIDEOCORE5    198
#define EM_78KOR         199
#define EM_56800EX       200
#define EM_BA1           201
#define EM_BA2           202
#define EM_XCORE         203
#define EM_MCHP_PIC      204
#define EM_KM32          210
#define EM_KMX32         211
#define EM_EMX16         212
#define EM_EMX8          213
#define EM_KVARC         214
#define EM_CDP           215
#define EM_COGE          216
#define EM_COOL          217
#define EM_NORC          218
#define EM_CSR_KALIMBA   219
#define EM_Z80           220
#define EM_VISIUM        221
#define EM_FT32          222
#define EM_MOXIE         223
#define EM_AMDGPU        224
#define EM_RISCV         243
#define EM_BPF           247
#define EM_CSKY          252

// ELF version (e_version field).
#define EV_NONE    0
#define EV_CURRENT 1

typedef uint32_t elf32_addr;
typedef uint16_t elf32_half;
typedef uint32_t elf32_off;
typedef uint32_t elf32_word;
typedef int32_t elf32_sword;

struct elf32_hdr {
    unsigned char e_ident[EI_NIDENT];
    elf32_half e_type;
    elf32_half e_machine;
    elf32_word e_version;
    elf32_addr e_entry;
    elf32_off e_phoff;
    elf32_off e_shoff;
    elf32_word e_flags;
    elf32_half e_ehsize;
    elf32_half e_phentsize;
    elf32_half e_phnum;
    elf32_half e_shentsize;
    elf32_half e_shnum;
    elf32_half e_shstrndx;
};

struct elf64_hdr {
    unsigned char e_ident[EI_NIDENT];
    // TODO(frolv): 64-bit support.
};

// Special section indices.
#define SHN_UNDEF     0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC    0xff00
#define SHN_HIPROC    0xff1f
#define SHN_LOOS      0xff20
#define SHN_HIOS      0xff3f
#define SHN_ABS       0xfff1
#define SHN_COMMON    0xfff2
#define SHN_HIRESERVE 0xffff

// Section types.
#define SHT_NULL          0
#define SHT_PROGBITS      1
#define SHT_SYMTAB        2
#define SHT_STRTAB        3
#define SHT_RELA          4
#define SHT_HASH          5
#define SHT_DYNAMIC       6
#define SHT_NOTE          7
#define SHT_NOBITS        8
#define SHT_REL           9
#define SHT_SHLIB         10
#define SHT_DYNSYM        11
#define SHT_INIT_ARRAY    14
#define SHT_FINI_ARRAY    15
#define SHT_PREINIT_ARRAY 16
#define SHT_GROUP         17
#define SHT_SYMTAB_SHNDX  18

// Section flags.
#define SHF_WRITE            (1 << 0)
#define SHF_ALLOC            (1 << 1)
#define SHF_EXECINSTR        (1 << 2)
#define SHF_MERGE            (1 << 4)
#define SHF_STRINGS          (1 << 5)
#define SHF_INFO_LINK        (1 << 6)
#define SHF_LINK_ORDER       (1 << 7)
#define SHF_OS_NONCONFORMING (1 << 8)
#define SHF_GROUP            (1 << 9)
#define SHF_TLS              (1 << 10)
#define SHF_COMPRESSED       (1 << 11)

struct elf32_shdr {
    elf32_word sh_name;
    elf32_word sh_type;
    elf32_word sh_flags;
    elf32_addr sh_addr;
    elf32_off sh_offset;
    elf32_word sh_size;
    elf32_word sh_link;
    elf32_word sh_info;
    elf32_word sh_addralign;
    elf32_word sh_entsize;
};

// Program header types.
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7

// Program header flags.
#define PF_X (1 << 0)
#define PF_W (1 << 1)
#define PF_R (1 << 2)

struct elf32_phdr {
    elf32_word p_type;
    elf32_off p_offset;
    elf32_addr p_vaddr;
    elf32_addr p_paddr;
    elf32_word p_filesz;
    elf32_word p_memsz;
    elf32_word p_flags;
    elf32_word p_align;
};

struct elf_context {
    addr_t entry;
};

#include <radix/asm/elf.h>

#ifdef __cplusplus
extern "C" {
#endif

// Loads and maps loadable segments from an ELF file into an address space.
// The provided context struct is populated with relevant information about the
// file on success.
int elf_load(struct vmm_space *vmm,
             const void *ptr,
             size_t len,
             struct elf_context *context);

// Returns true if the provided ELF machine is compatible with the current
// processor architecture.
static inline bool elf_machine_is_supported(elf32_half machine)
{
    return __arch_elf_machine_is_supported(machine);
}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // RADIX_ELF_H
