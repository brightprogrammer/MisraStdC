#include <Misra.h>
#include <stdio.h>

typedef enum {
    ELF_CLASS_NONE = 0,
    ELF_CLASS_32   = 1,
    ELF_CLASS_64   = 2,
    ELF_CLASS_NUM  = 3,
} ElfClass;

typedef enum {
    ELF_ENCODING_NONE = 0,
    ELF_ENCODING_LSB  = 1,
    ELF_ENCODING_MSB  = 2,
    ELF_ENCODING_NUM  = 3,
} ElfEncoding;

typedef enum {
    ELF_VERSION_NONE    = 0,
    ELF_VERSION_CURRENT = 1,
    ELF_VERSION_NUM     = 2,
} ElfVersion;

typedef enum {
    ELF_OS_ABI_NONE       = 0,              /// UNIX System V ABI/
    ELF_OS_ABI_SYSV       = 0,              /// Alias.
    ELF_OS_ABI_HPUX       = 1,              /// HP-UX
    ELF_OS_ABI_NETBSD     = 2,              /// NetBSD.
    ELF_OS_ABI_GNU        = 3,              /// Object uses GNU ELF extensions.
    ELF_OS_ABI_LINUX      = ELF_OS_ABI_GNU, /// Compatibility alias.
    ELF_OS_ABI_SOLARIS    = 6,              /// Sun Solaris.
    ELF_OS_ABI_AIX        = 7,              /// IBM AIX.
    ELF_OS_ABI_IRIX       = 8,              /// SGI Irix.
    ELF_OS_ABI_FREEBSD    = 9,              /// FreeBSD.
    ELF_OS_ABI_TRU64      = 10,             /// Compaq TRU64 UNIX.
    ELF_OS_ABI_MODESTO    = 11,             /// Novell Modesto.
    ELF_OS_ABI_OPENBSD    = 12,             /// OpenBSD.
    ELF_OS_ABI_ARM_AEABI  = 64,             /// ARM EABI
    ELF_OS_ABI_ARM        = 97,             /// ARM
    ELF_OS_ABI_STANDALONE = 255,            /// Standalone (embedded) application
} ElfOsAbi;

typedef struct {
    ElfClass    class;
    ElfEncoding encoding;
    ElfVersion  version;
    ElfOsAbi    os_abi;
    ElfVersion  abi_version;
} ElfMeta;

#define FMT_ELF_META                                                                                                   \
    "\x7f"                                                                                                             \
    "ELF"                                                                                                              \
    "{1r}" /* class */                                                                                                 \
    "{1r}" /* encoding */                                                                                              \
    "{1r}" /* version */                                                                                               \
    "{1r}" /* os_abi */                                                                                                \
    "{1r}" /* abi_version */

/// Legal values for e_machine (architecture).
typedef enum {
    ELF_MACHINE_NONE        = 0,  /// No machine
    ELF_MACHINE_M32         = 1,  /// AT&T WE 32100
    ELF_MACHINE_SPARC       = 2,  /// SUN SPARC
    ELF_MACHINE_386         = 3,  /// Intel 80386
    ELF_MACHINE_68K         = 4,  /// Motorola m68k family
    ELF_MACHINE_88K         = 5,  /// Motorola m88k family
    ELF_MACHINE_IAMCU       = 6,  /// Intel MCU
    ELF_MACHINE_860         = 7,  /// Intel 80860
    ELF_MACHINE_MIPS        = 8,  /// MIPS R3000 big-endian
    ELF_MACHINE_S370        = 9,  /// IBM System/370
    ELF_MACHINE_MIPS_RS3_LE = 10, /// MIPS R3000 little-endian
    /* reserved 11-14 */
    ELF_MACHINE_PARISC = 15, /// HPPA
    /* reserved 16 */
    ELF_MACHINE_VPP500      = 17, /// Fujitsu VPP500
    ELF_MACHINE_SPARC32PLUS = 18, /// Sun's "v8plus"
    ELF_MACHINE_960         = 19, /// Intel 80960
    ELF_MACHINE_PPC         = 20, /// PowerPC
    ELF_MACHINE_PPC64       = 21, /// PowerPC 64-bit
    ELF_MACHINE_S390        = 22, /// IBM S390
    ELF_MACHINE_SPU         = 23, /// IBM SPU/SPC
    /* reserved 24-35 */
    ELF_MACHINE_V800         = 36,  /// NEC V800 series
    ELF_MACHINE_FR20         = 37,  /// Fujitsu FR20
    ELF_MACHINE_RH32         = 38,  /// TRW RH-32
    ELF_MACHINE_RCE          = 39,  /// Motorola RCE
    ELF_MACHINE_ARM          = 40,  /// ARM
    ELF_MACHINE_FAKE_ALPHA   = 41,  /// Digital Alpha
    ELF_MACHINE_SH           = 42,  /// Hitachi SH
    ELF_MACHINE_SPARCV9      = 43,  /// SPARC v9 64-bit
    ELF_MACHINE_TRICORE      = 44,  /// Siemens Tricore
    ELF_MACHINE_ARC          = 45,  /// Argonaut RISC Core
    ELF_MACHINE_H8_300       = 46,  /// Hitachi H8/300
    ELF_MACHINE_H8_300H      = 47,  /// Hitachi H8/300H
    ELF_MACHINE_H8S          = 48,  /// Hitachi H8S
    ELF_MACHINE_H8_500       = 49,  /// Hitachi H8/500
    ELF_MACHINE_IA_64        = 50,  /// Intel Merced
    ELF_MACHINE_MIPS_X       = 51,  /// Stanford MIPS-X
    ELF_MACHINE_COLDFIRE     = 52,  /// Motorola Coldfire
    ELF_MACHINE_68HC12       = 53,  /// Motorola M68HC12
    ELF_MACHINE_MMA          = 54,  /// Fujitsu MMA Multimedia Accelerator
    ELF_MACHINE_PCP          = 55,  /// Siemens PCP
    ELF_MACHINE_NCPU         = 56,  /// Sony nCPU embedded RISC
    ELF_MACHINE_NDR1         = 57,  /// Denso NDR1 microprocessor
    ELF_MACHINE_STARCORE     = 58,  /// Motorola Start*Core processor
    ELF_MACHINE_ME16         = 59,  /// Toyota ME16 processor
    ELF_MACHINE_ST100        = 60,  /// STMicroelectronic ST100 processor
    ELF_MACHINE_TINYJ        = 61,  /// Advanced Logic Corp. Tinyj emb.fam
    ELF_MACHINE_X86_64       = 62,  /// AMD x86-64 architecture
    ELF_MACHINE_PDSP         = 63,  /// Sony DSP Processor
    ELF_MACHINE_PDP10        = 64,  /// Digital PDP-10
    ELF_MACHINE_PDP11        = 65,  /// Digital PDP-11
    ELF_MACHINE_FX66         = 66,  /// Siemens FX66 microcontroller
    ELF_MACHINE_ST9PLUS      = 67,  /// STMicroelectronics ST9+ 8/16 mc
    ELF_MACHINE_ST7          = 68,  /// STmicroelectronics ST7 8 bit mc
    ELF_MACHINE_68HC16       = 69,  /// Motorola MC68HC16 microcontroller
    ELF_MACHINE_68HC11       = 70,  /// Motorola MC68HC11 microcontroller
    ELF_MACHINE_68HC08       = 71,  /// Motorola MC68HC08 microcontroller
    ELF_MACHINE_68HC05       = 72,  /// Motorola MC68HC05 microcontroller
    ELF_MACHINE_SVX          = 73,  /// Silicon Graphics SVx
    ELF_MACHINE_ST19         = 74,  /// STMicroelectronics ST19 8 bit mc
    ELF_MACHINE_VAX          = 75,  /// Digital VAX
    ELF_MACHINE_CRIS         = 76,  /// Axis Communications 32-bit emb.proc
    ELF_MACHINE_JAVELIN      = 77,  /// Infineon Technologies 32-bit emb.proc
    ELF_MACHINE_FIREPATH     = 78,  /// Element 14 64-bit DSP Processor
    ELF_MACHINE_ZSP          = 79,  /// LSI Logic 16-bit DSP Processor
    ELF_MACHINE_MMIX         = 80,  /// Donald Knuth's educational 64-bit proc
    ELF_MACHINE_HUANY        = 81,  /// Harvard University machine-independent object files
    ELF_MACHINE_PRISM        = 82,  /// SiTera Prism
    ELF_MACHINE_AVR          = 83,  /// Atmel AVR 8-bit microcontroller
    ELF_MACHINE_FR30         = 84,  /// Fujitsu FR30
    ELF_MACHINE_D10V         = 85,  /// Mitsubishi D10V
    ELF_MACHINE_D30V         = 86,  /// Mitsubishi D30V
    ELF_MACHINE_V850         = 87,  /// NEC v850
    ELF_MACHINE_M32R         = 88,  /// Mitsubishi M32R
    ELF_MACHINE_MN10300      = 89,  /// Matsushita MN10300
    ELF_MACHINE_MN10200      = 90,  /// Matsushita MN10200
    ELF_MACHINE_PJ           = 91,  /// picoJava
    ELF_MACHINE_OPENRISC     = 92,  /// OpenRISC 32-bit embedded processor
    ELF_MACHINE_ARC_COMPACT  = 93,  /// ARC International ARCompact
    ELF_MACHINE_XTENSA       = 94,  /// Tensilica Xtensa Architecture
    ELF_MACHINE_VIDEOCORE    = 95,  /// Alphamosaic VideoCore
    ELF_MACHINE_TMM_GPP      = 96,  /// Thompson Multimedia General Purpose Proc
    ELF_MACHINE_NS32K        = 97,  /// National Semi. 32000
    ELF_MACHINE_TPC          = 98,  /// Tenor Network TPC
    ELF_MACHINE_SNP1K        = 99,  /// Trebia SNP 1000
    ELF_MACHINE_ST200        = 100, /// STMicroelectronics ST200
    ELF_MACHINE_IP2K         = 101, /// Ubicom IP2xxx
    ELF_MACHINE_MAX          = 102, /// MAX processor
    ELF_MACHINE_CR           = 103, /// National Semi. CompactRISC
    ELF_MACHINE_F2MC16       = 104, /// Fujitsu F2MC16
    ELF_MACHINE_MSP430       = 105, /// Texas Instruments msp430
    ELF_MACHINE_BLACKFIN     = 106, /// Analog Devices Blackfin DSP
    ELF_MACHINE_SE_C33       = 107, /// Seiko Epson S1C33 family
    ELF_MACHINE_SEP          = 108, /// Sharp embedded microprocessor
    ELF_MACHINE_ARCA         = 109, /// Arca RISC
    ELF_MACHINE_UNICORE      = 110, /// PKU-Unity & MPRC Peking Uni. mc series
    ELF_MACHINE_EXCESS       = 111, /// eXcess configurable cpu
    ELF_MACHINE_DXP          = 112, /// Icera Semi. Deep Execution Processor
    ELF_MACHINE_ALTERA_NIOS2 = 113, /// Altera Nios II
    ELF_MACHINE_CRX          = 114, /// National Semi. CompactRISC CRX
    ELF_MACHINE_XGATE        = 115, /// Motorola XGATE
    ELF_MACHINE_C166         = 116, /// Infineon C16x/XC16x
    ELF_MACHINE_M16C         = 117, /// Renesas M16C
    ELF_MACHINE_DSPIC30F     = 118, /// Microchip Technology dsPIC30F
    ELF_MACHINE_CE           = 119, /// Freescale Communication Engine RISC
    ELF_MACHINE_M32C         = 120, /// Renesas M32C
    /* reserved 121-130 */
    ELF_MACHINE_TSK3000       = 131, /// Altium TSK3000
    ELF_MACHINE_RS08          = 132, /// Freescale RS08
    ELF_MACHINE_SHARC         = 133, /// Analog Devices SHARC family
    ELF_MACHINE_ECOG2         = 134, /// Cyan Technology eCOG2
    ELF_MACHINE_SCORE7        = 135, /// Sunplus S+core7 RISC
    ELF_MACHINE_DSP24         = 136, /// New Japan Radio (NJR) 24-bit DSP
    ELF_MACHINE_VIDEOCORE3    = 137, /// Broadcom VideoCore III
    ELF_MACHINE_LATTICEMICO32 = 138, /// RISC for Lattice FPGA
    ELF_MACHINE_SE_C17        = 139, /// Seiko Epson C17
    ELF_MACHINE_TI_C6000      = 140, /// Texas Instruments TMS320C6000 DSP
    ELF_MACHINE_TI_C2000      = 141, /// Texas Instruments TMS320C2000 DSP
    ELF_MACHINE_TI_C5500      = 142, /// Texas Instruments TMS320C55x DSP
    ELF_MACHINE_TI_ARP32      = 143, /// Texas Instruments App. Specific RISC
    ELF_MACHINE_TI_PRUi       = 144, /// Texas Instruments Prog. Realtime Unit
    /* reserved 145-159 */
    ELF_MACHINE_MMDSP_PLUS  = 160, /// STMicroelectronics 64bit VLIW DSP
    ELF_MACHINE_CYPRESS_M8C = 161, /// Cypress M8C
    ELF_MACHINE_R32C        = 162, /// Renesas R32C
    ELF_MACHINE_TRIMEDIA    = 163, /// NXP Semi. TriMedia
    ELF_MACHINE_QDSP6       = 164, /// QUALCOMM DSP6
    ELF_MACHINE_8051        = 165, /// Intel 8051 and variants
    ELF_MACHINE_STXP7X      = 166, /// STMicroelectronics STxP7x
    ELF_MACHINE_NDS32       = 167, /// Andes Tech. compact code emb. RISC
    ELF_MACHINE_ECOG1X      = 168, /// Cyan Technology eCOG1X
    ELF_MACHINE_MAXQ30      = 169, /// Dallas Semi. MAXQ30 mc
    ELF_MACHINE_XIMO16      = 170, /// New Japan Radio (NJR) 16-bit DSP
    ELF_MACHINE_MANIK       = 171, /// M2000 Reconfigurable RISC
    ELF_MACHINE_CRAYNV2     = 172, /// Cray NV2 vector architecture
    ELF_MACHINE_RX          = 173, /// Renesas RX
    ELF_MACHINE_METAG       = 174, /// Imagination Tech. META
    ELF_MACHINE_MCST_ELBRUS = 175, /// MCST Elbrus
    ELF_MACHINE_ECOG16      = 176, /// Cyan Technology eCOG16
    ELF_MACHINE_CR16        = 177, /// National Semi. CompactRISC CR16
    ELF_MACHINE_ETPU        = 178, /// Freescale Extended Time Processing Unit
    ELF_MACHINE_SLE9X       = 179, /// Infineon Tech. SLE9X
    ELF_MACHINE_L10M        = 180, /// Intel L10M
    ELF_MACHINE_K10M        = 181, /// Intel K10M
    /* reserved 182 */
    ELF_MACHINE_AARCH64 = 183, /// ARM AARCH64
    /* reserved 184 */
    ELF_MACHINE_AVR32       = 185, /// Amtel 32-bit microprocessor
    ELF_MACHINE_STM8        = 186, /// STMicroelectronics STM8
    ELF_MACHINE_TILE64      = 187, /// Tilera TILE64
    ELF_MACHINE_TILEPRO     = 188, /// Tilera TILEPro
    ELF_MACHINE_MICROBLAZE  = 189, /// Xilinx MicroBlaze
    ELF_MACHINE_CUDA        = 190, /// NVIDIA CUDA
    ELF_MACHINE_TILEGX      = 191, /// Tilera TILE-Gx
    ELF_MACHINE_CLOUDSHIELD = 192, /// CloudShield
    ELF_MACHINE_COREA_1ST   = 193, /// KIPO-KAIST Core-A 1st gen.
    ELF_MACHINE_COREA_2ND   = 194, /// KIPO-KAIST Core-A 2nd gen.
    ELF_MACHINE_ARCV2       = 195, /// Synopsys ARCv2 ISA.
    ELF_MACHINE_OPEN8       = 196, /// Open8 RISC
    ELF_MACHINE_RL78        = 197, /// Renesas RL78
    ELF_MACHINE_VIDEOCORE5  = 198, /// Broadcom VideoCore V
    ELF_MACHINE_78KOR       = 199, /// Renesas 78KOR
    ELF_MACHINE_56800EX     = 200, /// Freescale 56800EX DSC
    ELF_MACHINE_BA1         = 201, /// Beyond BA1
    ELF_MACHINE_BA2         = 202, /// Beyond BA2
    ELF_MACHINE_XCORE       = 203, /// XMOS xCORE
    ELF_MACHINE_MCHP_PIC    = 204, /// Microchip 8-bit PIC(r)
    ELF_MACHINE_INTELGT     = 205, /// Intel Graphics Technology
    /* reserved 206-209 */
    ELF_MACHINE_KM32        = 210, /// KM211 KM32
    ELF_MACHINE_KMX32       = 211, /// KM211 KMX32
    ELF_MACHINE_EMX16       = 212, /// KM211 KMX16
    ELF_MACHINE_EMX8        = 213, /// KM211 KMX8
    ELF_MACHINE_KVARC       = 214, /// KM211 KVARC
    ELF_MACHINE_CDP         = 215, /// Paneve CDP
    ELF_MACHINE_COGE        = 216, /// Cognitive Smart Memory Processor
    ELF_MACHINE_COOL        = 217, /// Bluechip CoolEngine
    ELF_MACHINE_NORC        = 218, /// Nanoradio Optimized RISC
    ELF_MACHINE_CSR_KALIMBA = 219, /// CSR Kalimba
    ELF_MACHINE_Z80         = 220, /// Zilog Z80
    ELF_MACHINE_VISIUM      = 221, /// Controls and Data Services VISIUMcore
    ELF_MACHINE_FT32        = 222, /// FTDI Chip FT32
    ELF_MACHINE_MOXIE       = 223, /// Moxie processor
    ELF_MACHINE_AMDGPU      = 224, /// AMD GPU
    /* reserved 225-242 */
    ELF_MACHINE_RISCV     = 243, /// RISC-V
    ELF_MACHINE_BPF       = 247, /// Linux BPF -- in-kernel virtual machine
    ELF_MACHINE_CSKY      = 252, /// C-SKY
    ELF_MACHINE_LOONGARCH = 258, /// LoongArch

    ELF_MACHINE_NUM = 259
} ElfMachine;

typedef enum {
    ELF_TYPE_NONE = 0,         /// No file type
    ELF_TYPE_REL  = 1,         /// Relocatable file
    ELF_TYPE_EXEC = 2,         /// Executable file
    ELF_TYPE_DYN  = 3,         /// Shared object file
    ELF_TYPE_CORE = 4,         /// Core file
    ELF_TYPE_NUM  = 5,         /// Number of defined types
} ElfType;

#define ELF_TYPE_LOOS   0xfe00 /* OS-specific range start */
#define ELF_TYPE_HIOS   0xfeff /* OS-specific range end */
#define ELF_TYPE_LOPROC 0xff00 /* Processor-specific range start */
#define ELF_TYPE_HIPROC 0xffff /* Processor-specific range end */

typedef struct {
    ElfMeta    meta;
    u16        type;
    ElfMachine machine;
    u32        version;
    u32        entry;
    u32        program_header_table_offset;
    u32        section_header_table_offset;
    u32        flags;
    u16        elf_header_size;
    u16        program_header_entry_size;
    u16        program_header_count;
    u16        section_header_entry_size;
    u16        section_header_count;
    u16        string_table_index;
} ElfHeader32;

typedef struct {
    ElfMeta    meta;
    u16        type;
    ElfMachine machine;
    u32        version;
    u64        entry;
    u64        program_header_table_offset;
    u64        section_header_table_offset;
    u32        flags;
    u16        elf_header_size;
    u16        program_header_entry_size;
    u16        program_header_count;
    u16        section_header_entry_size;
    u16        section_header_count;
    u16        string_table_index;
} ElfHeader64;

#define FMT_ELF_HEADER_64_LE                                                                                           \
    "{<2r}" /* type */                                                                                                 \
    "{<2r}" /* machine */                                                                                              \
    "{<4r}" /* version */                                                                                              \
    "{<8r}" /* entry */                                                                                                \
    "{<8r}" /* phoff */                                                                                                \
    "{<8r}" /* shoff */                                                                                                \
    "{<4r}" /* flags */                                                                                                \
    "{<2r}" /* ehsize */                                                                                               \
    "{<2r}" /* phentsize */                                                                                            \
    "{<2r}" /* phnum */                                                                                                \
    "{<2r}" /* shentsize */                                                                                            \
    "{<2r}" /* shnum */                                                                                                \
    "{<2r}" /* shstrndx */

#define FMT_ELF_HEADER_64_BE                                                                                           \
    "{>2r}" /* type */                                                                                                 \
    "{>2r}" /* machine */                                                                                              \
    "{>4r}" /* version */                                                                                              \
    "{>8r}" /* entry */                                                                                                \
    "{>8r}" /* phoff */                                                                                                \
    "{>8r}" /* shoff */                                                                                                \
    "{>4r}" /* flags */                                                                                                \
    "{>2r}" /* ehsize */                                                                                               \
    "{>2r}" /* phentsize */                                                                                            \
    "{>2r}" /* phnum */                                                                                                \
    "{>2r}" /* shentsize */                                                                                            \
    "{>2r}" /* shnum */                                                                                                \
    "{>2r}" /* shstrndx */

int main(int argc, char** argv) {
    if (argc < 2) {
        LOG_FATAL("USAGE: {} {}", FMT(argv[0]), FMT(argv[1]));
    }

    FILE* elf = fopen(argv[1], "rb");
    if (!elf) {
        LOG_ERROR("Failed to open file for reading.");
        return 1;
    }

    ElfHeader64 eh = {0};
    FReadFmt(
        elf,
        FMT_ELF_META,
        FMT(eh.meta.class),
        FMT(eh.meta.encoding),
        FMT(eh.meta.version),
        FMT(eh.meta.os_abi),
        FMT(eh.meta.abi_version)
    );

    // technically padding here but we can skip it
    fseek(elf, 7, SEEK_CUR);

    // XXX: For now we only support x86_64 binaries
    // this will also indirectly decide if elf magic is valid
    // for an invalid elf magic, any subsequent fields will be zero, and hence will be invalid
    if (eh.meta.class != ELF_CLASS_64) {
        LOG_FATAL("Only 64-bit binaries supported for now. Class for provided binary is = {}", FMT(eh.meta.class));
    }

    FReadFmt(
        elf,
        eh.meta.encoding == ELF_ENCODING_LSB ? FMT_ELF_HEADER_64_LE : FMT_ELF_HEADER_64_BE,
        FMT(eh.type),
        FMT(eh.machine),
        FMT(eh.version),
        FMT(eh.entry),
        FMT(eh.program_header_table_offset),
        FMT(eh.section_header_table_offset),
        FMT(eh.flags),
        FMT(eh.elf_header_size),
        FMT(eh.program_header_entry_size),
        FMT(eh.program_header_count),
        FMT(eh.section_header_entry_size),
        FMT(eh.section_header_count),
        FMT(eh.string_table_index)
    );

    WriteFmtLn(
        "ElfHeader64 {{\n"
        "  meta: {{class: {}, encoding: {}, version: {}, os_abi: {}}}\n"
        "  type: {x}\n"
        "  machine: {x}\n"
        "  version: {}\n"
        "  entry: {x}\n"
        "  program_header_table_offset: {x}\n"
        "  section_header_table_offset: {x}\n"
        "  flags: {b}\n"
        "  elf_header_size: {x}\n"
        "  program_header_entry_size: {x}\n"
        "  program_header_count: {}\n"
        "  section_header_entry_size: {x}\n"
        "  section_header_count: {}\n"
        "  string_table_index: {}\n"
        "}}",

        FMT(eh.meta.class),
        FMT(eh.meta.encoding),
        FMT(eh.meta.version),
        FMT(eh.meta.os_abi),

        FMT(eh.type),
        FMT(eh.machine),
        FMT(eh.version),
        FMT(eh.entry),
        FMT(eh.program_header_table_offset),
        FMT(eh.section_header_table_offset),
        FMT(eh.flags),
        FMT(eh.elf_header_size),
        FMT(eh.program_header_entry_size),
        FMT(eh.program_header_count),
        FMT(eh.section_header_entry_size),
        FMT(eh.section_header_count),
        FMT(eh.string_table_index)
    );

    return 0;
}
