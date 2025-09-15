#pragma once
// MIT License
// Copyright (c) 2021 Jan-Willem Krans (janwillem32 <at> hotmail <dot> com)
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <cinttypes>
#include <intrin.h>

// disable warning messages for this file only:
// C4201: nonstandard extension used: nameless struct/union
#pragma warning(push)
#pragma warning(disable: 4201)

union FeatureBitsCPUId{
	struct{uint32_t eax_1, ebx_1, ecx_1, edx_1, ebx_7s0, ecx_7s0, edx_7s0, eax_7s1, ebx_0x80000001, ecx_0x80000001, edx_0x80000001;};
	struct{
		// eax_1
		uint32_t SteppingBits : 4;           // 0 to 3, CPU Stepping ID
		uint32_t BaseModelBits : 4;          // 4 to 7, CPU model
		uint32_t BaseFamilyBits : 4;         // 8 to 11, CPU family ID
		uint32_t ProcessorTypeBits : 2;      // 12 to 13, Processor Type
		uint32_t eax_1_14_to_15 : 2;         // 14 to 15 are reserved
		uint32_t ExtModelBits : 4;           // 16 to 19, Extended Model ID
		uint32_t ExtFamilyBits : 8;          // 20 to 27, Extended Family ID
		uint32_t eax_1_28_to_31 : 4;         // 28 to 31 are reserved
		// ebx_1
		uint8_t E8BitBrandIdBits;            // 0 to 7, Brand Index
		uint8_t CLFlushSizeInQuadwordsBits;  // 8 to 15, requires the CLFLUSH feature flag, the cache line flush size for explicit CLFLUSH instructions
		uint8_t LogicalProcessorCountBits;   // 16 to 23, requires the HTT flag, this only indicates the logical processor count in multi-core aware processors and only shows the core count for the tested processor, an additonal amount of cores will be present on multi-processor systems
		uint8_t LocalApicIdBits;             // 24 to 31, Local APIC ID
		// ecx_1
		uint32_t SSE3 : 1;                   // 0, since 2004, Streaming SIMD Extensions 3, 11 instructions
		uint32_t PCLMULDQ : 1;               // 1, since 2010, Carry-less Multiplication, 5 instructions
		uint32_t DTES64 : 1;                 // 2, since 2003, kernel mode only, 64-bit Debug Store, only available in the long mode
		uint32_t MONITOR : 1;                // 3, since 2004, kernel mode only, MONITOR and MWAIT instruction pair, _mm_monitor() and _mm_mwait()
		uint32_t DS_CPL : 1;                 // 4, since 2000, kernel mode only, CPL Qualified Debug Store, extensions of the debug store for branch message storage in level from kernel mode (CPL == 0) to user mode (CPL == 4)
		uint32_t VMX : 1;                    // 5, since 2005, kernel mode only, Virtual Machine eXtensions
		uint32_t SMX : 1;                    // 6, since 2006, kernel mode only, Safer Mode eXtensions, hardware control on OS trusted environment
		uint32_t EIST : 1;                   // 7, since 2004, kernel mode only, Enhanced Intel SpeedsTep
		uint32_t TM2 : 1;                    // 8, since 2004, Thermal Monitor 2, indicates that a thermal monitor automatically limits the CPU temperature by lowering the CPU clock multiplier, no new instructions
		uint32_t SSSE3 : 1;                  // 9, since 2006, Supplemental Streaming SIMD Extensions 3, 32 instructions
		uint32_t CNXT_ID : 1;                // 10, since 2010, kernel mode only, Context ID, process scheduler L1 data cache management in level from kernel mode (CPL == 0) to user mode (CPL == 4)
		uint32_t SDBG : 1;                   // 11, since 2013, Silicon Debug interface
		uint32_t FMA3 : 1;                   // 12, since 2011, 3-operand Fused Multiply-Add, 18 instructions
		uint32_t CX16 : 1;                   // 13, since 2003, CMPXCHG16B instruction, only available in the long mode (acts on 128-bit rdx:rax), _InterlockedCompareExchange128()
		uint32_t XTPR : 1;                   // 14, since 2004, kernel mode only, xTPR update control, TPR register chipset update control messenger APIC extension
		uint32_t PDCM : 1;                   // 15, since 2002, kernel mode only, Performance Debug Capability MSR
		uint32_t ecx_1_16 : 1;               // 16 is reserved
		uint32_t PCID : 1;                   // 17, since 2011, kernel mode only, Process Context IDentifiers, INVPCID instruction, _invpcid()
		uint32_t DCA : 1;                    // 18, since 2007, kernel mode only, Direct Cache Access for DMA writes
		uint32_t SSE4_1 : 1;                 // 19, since 2007, Streaming SIMD Extensions 4.1, 47 instructions
		uint32_t SSE4_2 : 1;                 // 20, since 2008, Streaming SIMD Extensions 4.2, 6 instructions
		uint32_t X2APIC : 1;                 // 21, since 2009, kernel mode only, x2APIC architecture, controlled with the RDMSR and WRMSR instruction pair
		uint32_t MOVBE : 1;                  // 22, since 2008, MOVBE instruction, first implemented on Intel Atom processors, added in 2013 to desktop processors
		uint32_t POPCNT : 1;                 // 23, since 2008, POPCNT instruction, _mm_popcnt_u32(), _mm_popcnt_u64()
		uint32_t TSC_DEADLINE : 1;           // 24, since 2006, kernel mode only, TSC-Deadline timer, one-shot timer mode extension of the Local APIC
		uint32_t AES : 1;                    // 25, since 2010, Advanced Encryption Standard extensions, 6 instructions
		uint32_t XSAVE : 1;                  // 26, since 2008, kernel mode only, XSAVE, XRSTOR, XSETBV and XGETBV instructions, extensions of the FXSR flag
		uint32_t OSXSAVE : 1;                // 27, since 2008, kernel mode only, OS provides processor extended state management, this flag is set by the host OS, implies that the previous bit is set
		uint32_t AVX : 1;                    // 28, since 2011, Advanced Vector eXtensions, 12 actually new instructions, many legacy SSE instructions are extended and duplicated in its VEX encoding scheme, all current operating systems allow usage of the AVX instructions
		uint32_t F16C : 1;                   // 29, since 2011, F16C instruction set, half-precision floating-point conversion extensions, VCVTPH2PS and VCVTPS2PH instructions
		uint32_t RDRAND : 1;                 // 30, since 2012, RDRAND instruction
		uint32_t HYPERVISOR : 1;             // 31, since 2005, by definition 0 on all physical processors, indicates that the current processor is a hypervisor
		// edx_1
		uint32_t FPU : 1;                    // 0, legacy check, since 1993, onboard x87 floating-point unit, 83 instructions, the x87 coprocessors (since 1980) were still optional on early 486 processors that first supported cpuid, hence this flag
		uint32_t VME : 1;                    // 1, since 2005, kernel mode only, Virtual Mode Extensions, hardware virtualization
		uint32_t DE : 1;                     // 2, legacy check, since 1995, Debugging Extension, support for I/O breakpoints
		uint32_t PSE : 1;                    // 3, legacy check, since 1995, kernel mode only, Page Size Extensions, adds an optional large page size setting of 4 M (2 M in PAE mode) for page addressing next to the standard 4 k pages
		uint32_t TSC : 1;                    // 4, legacy check, since 1993, RDTSC instruction, this instruction can be priviledged at kernel level only by the time stamp disable (TSD) flag in control register CR4, but no operating systems actually set that flag, __rdtsc()
		uint32_t MSR : 1;                    // 5, legacy check, since 1993, kernel mode only, Model-Specific Registers, RDMSR and WRMSR instruction pair
		uint32_t PAE : 1;                    // 6, legacy check, since 1995, kernel mode only, Physical Address Extension, allows usage of more than 32-bit physical addressing space in 32-bit mode
		uint32_t MCE : 1;                    // 7, legacy check, since 1995, kernel mode only, Machine Check Exception, hardware error reporting system
		uint32_t CX8 : 1;                    // 8, legacy check, since 1995, CMPXCHG8B instruction, rarely used in the long mode (acts on 64-bit edx:eax), _InterlockedCompareExchange64()
		uint32_t APIC : 1;                   // 9, since 1993, kernel mode only, Advanced Programmable Interrupt Controller, indicates presence of a software-accessible APIC for multiprocessor systems
		uint32_t edx_1_10 : 1;               // 10 is reserved
		uint32_t SEP : 1;                    // 11, legacy check, since 1993, SYSENTER and SYSEXIT instructions, implementations of fast system call in user mode to kernel mode and back, some parts of this extension are kernel mode only
		uint32_t MTRR : 1;                   // 12, legacy check, since 1995, Memory Type Range Registers, kernel mode only, registers that control caching on specific memory ranges
		uint32_t PGE : 1;                    // 13, legacy check, since 1993, Page Global Enable, kernel mode only, allows address translations in multiple processes to be globally tracked, indicates that cache for shared pages does not need to be flushed on context switches
		uint32_t MCA : 1;                    // 14, legacy check, since 2000, kernel mode only, Machine Check Architecture, hardware error reporting system
		uint32_t CMOV : 1;                   // 15, legacy check, since 1995, Conditional Move extensions, 42 instructions
		uint32_t PAT : 1;                    // 16, legacy check, since 1999, kernel mode only, Page Attribute Table, allows setting per-page attributes on caching
		uint32_t PSE_36 : 1;                 // 17, legacy check, since 1997, kernel mode only, 36-bit Page Size Extension, allows usage of more than 32-bit physical addressing space in 32-bit mode, alternative of PAE
		uint32_t PSN : 1;                    // 18, since 1999, Processor Serial Number, only found on Pentium 3 processors and has to be enabled in the BIOS to work
		uint32_t CLFSH : 1;                  // 19, legacy check, since 2000, CLFLUSH instruction, paired with the data from CLFlushSizeInQuadwordsBits, _mm_clflush()
		uint32_t edx_1_20 : 1;               // 20 is reserved
		uint32_t DS : 1;                     // 21, legacy check, since 2000, Debug Store, store branch (Branch Trace Store, or BTS) and PEBS records in a memory buffer referred to as the Debug Store save area
		uint32_t ACPI : 1;                   // 22, legacy check, since 1996, kernel mode only, Advanced Configuration and Power Interface, thermal and power controls
		uint32_t MMX : 1;                    // 23, legacy check, since 1996, MMX instruction set, 57 instructions
		uint32_t FXSR : 1;                   // 24, legacy check, since 1996, kernel mode only, FXSAVE and FXRSTOR instruction pair, context switch save and restore registers
		uint32_t SSE : 1;                    // 25, legacy check, since 1999, Streaming SIMD Extensions, 70 instructions
		uint32_t SSE2 : 1;                   // 26, legacy check, since 2000, Streaming SIMD Extensions 2, 144 instructions
		uint32_t SS : 1;                     // 27, since 2000, indicates that the processor cache supports Self-Snoop to manage memory transactions
		uint32_t HTT : 1;                    // 28, since 2002, Hyper-Threading Technology, indicates that the processor is Hyper-Threading or multi-core aware, implementation of either is not a requirement, paired with the data from LogicalProcessorCountBits()
		uint32_t TM : 1;                     // 29, since 2000, Thermal Monitor, indicates that a thermal monitor automatically limits the CPU temperature by inserting idle cycles, no new instructions
		uint32_t IA64 : 1;                   // 30, since 2001, indicates x86 emulation by an IA-64 processor
		uint32_t PBE : 1;                    // 31, since 2002, Pending Break Enable wakeup support
		// ebx_7s0
		uint32_t FSGSBASE : 1;               // 0, since 2013, Access to the base of the fs and gs registers, RDFSBASE and RDGSBASE instructions
		uint32_t IA32_TSC_ADJUST : 1;        // 1, since 2015, Time-Stamp Counter Adjustment, no new instructions
		uint32_t SGX : 1;                    // 2, since 2015, kernel mode only, Software Guard Extensions, enables the use of enclaves, private regions of memory for user-mode processes, 18 instructions
		uint32_t BMI1 : 1;                   // 3, since 2013, Bit Manipulation Instruction Set 1, 6 instructions
		uint32_t HLE : 1;                    // 4, since 2013, Transactional Synchronization Extensions, XACQUIRE and XRELEASE instruction prefixes, XTEST instruction
		uint32_t AVX2 : 1;                   // 5, since 2013, Advanced Vector Extensions 2, 30 actually new instructions, many legacy integer SSE instructions are extended and duplicated in the newer VEX encoding scheme
		uint32_t FDP_EXCPTN_ONLY : 1;        // 6, since 2016, kernel mode only, x87 FPU Data Pointer (FDP) updated only on x87 exceptions if enabled
		uint32_t SMEP : 1;                   // 7, since 2013, kernel mode only, Supervisor-Mode Execution Prevention
		uint32_t BMI2 : 1;                   // 8, since 2015, Bit Manipulation Instruction Set 2, 8 instructions
		uint32_t ERMS : 1;                   // 9, since 2015, Enhanced REP MOVSB and REP STOSB, no new instructions
		uint32_t INVPCID : 1;                // 10, since 2013, kernel mode only, INVPCID instruction
		uint32_t TSX : 1;                    // 11, since 2013, Transactional Synchronization Extensions, XBEGIN, XEND and XABORT instructions
		uint32_t PQM : 1;                    // 12, since 2015, kernel mode only, Platform Quality of Service Monitoring
		uint32_t DFPUCSGS: 1;                // 13, since 2015, deprecates FPU CS and FPU DS segment registers
		uint32_t MPX : 1;                    // 14, since 2015, kernel mode only, Memory Protection Extensions
		uint32_t PQE : 1;                    // 15, since 2015, kernel mode only, Platform Quality of Service Monitoring
		uint32_t AVX512_F : 1;               // 16, since 2015, AVX-512 Foundation instructions, 152 actually new instructions, many legacy SSE and AVX instructions are extended and duplicated in its EVEX encoding scheme, all current operating systems allow usage of the AVX-512 instructions
		uint32_t AVX512_DQ : 1;              // 17, since 2015, AVX-512 Doubleword and Quadword Instructions, 68 instructions
		uint32_t RDSEED : 1;                 // 18, since 2013, RDSEED instruction
		uint32_t ADX : 1;                    // 19, since 2013, Multi-Precision Add-Carry Instruction Extensions, ADCX and ADOX instructions
		uint32_t SMAP : 1;                   // 20, since 2013, kernel mode only, Supervisor Mode Access Prevention
		uint32_t AVX512_IFMA : 1;            // 21, since 2015, AVX-512 Integer Fused Multiply-Add Instructions, VPMADD52HUQ and VPMADD52LUQ instructions
		uint32_t PCOMMIT : 1;                // 22, since 2014, kernel mode only, PCOMMIT instruction
		uint32_t CLFLUSHOPT : 1;             // 23, since 2014, kernel mode only, CLFLUSHOPT instruction
		uint32_t CLWB : 1;                   // 24, since 2014, kernel mode only, CLWB instruction
		uint32_t PT : 1;                     // 25, since 2014, kernel mode only, Processor Trace extensions
		uint32_t AVX512_PF : 1;              // 26, since 2015, AVX-512 Prefetch Instructions, 16 instructions
		uint32_t AVX512_ER : 1;              // 27, since 2015, AVX-512 Exponential and Reciprocal Instructions, 10 instructions
		uint32_t AVX512_CD : 1;              // 28, since 2015, AVX-512 Conflict Detection Instructions, 6 instructions
		uint32_t SHA : 1;                    // 29, since 2015, Secure Hash Algorithm extensions, 7 instructions
		uint32_t AVX512_BW : 1;              // 30, since 2015, AVX-512 Byte and Word Instructions, 112 instructions
		uint32_t AVX512_VL : 1;              // 31, since 2015, AVX-512 Vector Length Extensions, no actual new instructions, but allows many existing AVX-512 to be executed on xmm and ymm registers instead of just zmm registers
		// ecx_7s0
		uint32_t PREFETCHWT1 : 1;            // 0, since 2015, PREFETCHWT1 instruction, support for _mm_prefetch() with the _MM_HINT_ET1 hint
		uint32_t AVX512_VBMI : 1;            // 1, since 2015, AVX-512 Vector Bit Manipulation Instructions, VPERMB, VPERMT2B, VPERMI2B and VPMULTISHIFTQB instructions
		uint32_t UMIP : 1;                   // 2, since 2016, User-mode Instruction Prevention, no new instructions, no new instructions, affects the user-mode execution of SGDT, SIDT, SLDT, SMSW and STR instructions
		uint32_t PKU : 1;                    // 3, since 2015, kernel mode only, Protection Keys for User-mode pages, RDPKRU and WRPKRU instruction pair
		uint32_t OSPKE : 1;                  // 4, since 2015, OS has set CR4.PKE to enable protection keys, this flag implies PKU to be enabled
		uint32_t WAITPKG : 1;                // 5, since 2020, Timed pause and user-level monitor/wait, UMONITOR, UMWAIT and TPAUSE instructions
		uint32_t AVX512_VBMI2 : 1;           // 6, since 2019, AVX-512 Vector Bit Manipulation Instructions 2, 16 instructions
		uint32_t CET_SS : 1;                 // 7, since 2019, kernel mode only, Control flow enforcement (CET) shadow stack
		uint32_t GFNI : 1;                   // 8, since 2019, Galois Field instructions, VGF2P8AFFINEINVQB, VGF2P8AFFINEQB and VGF2P8MULB instructions
		uint32_t VAES : 1;                   // 9, since 2019, Vector AES instruction set (VEX-256/EVEX), VAESDEC, VAESDECLAST VAESENC and VAESENCLAST instructions
		uint32_t VPCLMULQDQ : 1;             // 10, since 2019, Vector CLMUL instruction set (VEX-256/EVEX), VPCLMULQDQ instruction
		uint32_t AVX512_VNNI : 1;            // 11, since 2019, AVX-512 Vector Neural Network Instructions, VPDPBUSD, VPDPBUSDS, VPDPWSSD and VPDPWSSDS instructions
		uint32_t AVX512_BITALG : 1;          // 12, since 2019, AVX-512 Bit Algorithms (BITALG) instructions, VPOPCNTB, VPOPCNTW and VPSHUFBITQMB instructions
		uint32_t ecx_7s0_13 : 1;             // 13 is reserved
		uint32_t AVX512_VPOPCNTDQ : 1;       // 14, since 2016, AVX-512 Vector Population Count D/Q, VPOPCNTD and VPOPCNTQ instructions
		uint32_t ecx_7s0_15 : 1;             // 15 is reserved
		uint32_t FIVE_LEVEL_PAGING : 1;      // 16, since 2019, kernel mode only, 5-level paging processor extension
		uint32_t MAWAU : 5;                  // 17 to 21, since 2013, kernel mode only, The value of userspace MPX Address-Width Adjust used by the BNDLDX and BNDSTX Intel MPX instructions in 64-bit mode
		uint32_t RDPID : 1;                  // 22, since 2016, Read Processor ID, RDPID instruction
		uint32_t ecx_7s0_23_to_24 : 2;       // 23 to 24 are reserved
		uint32_t CLDEMOTE : 1;               // 25, since 2021, Cache line demote, CLDEMOTE instruction
		uint32_t ecx_7s0_26 : 1;             // 26 is reserved
		uint32_t MOVDIRI : 1;                // 27, since 2021, direct-store instruction, MOVDIRI instruction, _directstoreu_u32(), _directstoreu_u64()
		uint32_t MOVDIR64B : 1;              // 28, since 2021, 64-byte direct-store instruction, MOVDIR64B instruction,  _movdir64b()
		uint32_t ENQCMD : 1;                 // 29, since 2021, kernel mode only, Enqueue Stores, NQCMD, ENQCMDS, VP2INTERSECTD and VP2INTERSECTQ instructions
		uint32_t SGX_LC : 1;                 // 30, since 2015, kernel mode only, SGX Launch Configuration for Software Guard Extensions
		uint32_t PKS : 1;                    // 31, since 2021, kernel mode only, Protection keys for supervisor-mode pages
		// edx_7s0
		uint32_t edx_7s0_0_to_1 : 2;         // 0 to 1 are reserved
		uint32_t AVX512_4VNNIW : 1;          // 2, since 2017, AVX-512 Neural Network Instructions, VP4DPWSSD and VP4DPWSSDS instructions
		uint32_t AVX512_4FMAPS : 1;          // 3, since 2017, AVX-512 Multiply Accumulation Single precision, V4FMADDPS, V4FMADDSS, V4FNMADDPS and V4FNMADDSS instructions
		uint32_t FSRM : 1;                   // 4, since 2019, Fast Short REP MOVSB flag, no new instructions
		uint32_t edx_7s0_5_to_7 : 3;         // 5 to 7 are reserved
		uint32_t AVX512_VP2INTERSECT : 1;    // 8, since 2020, AVX-512 Vector Pair Intersection to a Pair of Mask Registers, VP2INTERSECTD and VP2INTERSECTQ instructions
		uint32_t SRBDS_CTRL : 1;             // 9, since 2012 (backported through microcode), security flag for Special Register Buffer Data Sampling mitigations, no new instructions, affects RDRAND, RDSEED and EGETKEY instructions
		uint32_t MD_CLEAR : 1;               // 10, since 2008 (backported through microcode), kernel mode only, security flag for VERW instruction clears CPU buffers, no new instructions
		uint32_t edx_7s0_11_to_12 : 2;       // 11 to 12 are reserved
		uint32_t TSX_FORCE_ABORT : 1;        // 13, since 2015 (backported through microcode), security flag for TSX instruction mitigations, no new instructions
		uint32_t SERIALIZE : 1;              // 14, since 2021, Serialize instruction execution, SERIALIZE instruction
		uint32_t HYBRID : 1;                 // 15, since 2020, hybrid CPU cores are present in system (a set for energy efficient and a set for performance computing)
		uint32_t TSXLDTRK : 1;               // 16, since 2021, TSX suspend load address tracking, XSUSLDTRK and XRESLDTRK instructions
		uint32_t edx_7s0_17 : 1;             // 17 is reserved
		uint32_t PCONFIG : 1;                // 18, since 2019, kernel mode only, Platform configuration (Memory Encryption Technologies Instructions), PCONFIG instruction
		uint32_t LBR : 1;                    // 19, since 2021, kernel mode only, Architectural Last Branch Records
		uint32_t CET_IBT : 1;                // 20, since 2021, kernel mode only, Control flow enforcement (CET) indirect branch tracking
		uint32_t edx_7s0_21 : 1;             // 21 is reserved
		uint32_t AMX_BF16 : 1;               // 22, since 2021, Tile computation on bfloat16 numbers, Advanced Matrix Extension (AMX), TDPBF16PS instruction
		uint32_t edx_7s0_23 : 1;             // 23 is reserved
		uint32_t AMX_TILE : 1;               // 24, since 2021, Tile architecture, Advanced Matrix Extension (AMX), 7 instructions
		uint32_t AMX_INT8 : 1;               // 25, since 2021, Tile computation on 8-bit integers, Advanced Matrix Extension (AMX), TDPBSSD, TDPBSUD, TDPBUSD and TDPBUUD instructions
		uint32_t IBRS_IBPB_SPEC_CTRL : 1;    // 26, since 2018, kernel mode only, Speculation Control, part of Indirect Branch Control (IBC), no new instructions
		uint32_t STIBP : 1;                  // 27, since 2018, kernel mode only, Single Thread Indirect Branch Predictor, Indirect Branch Control (IBC), no new instructions
		uint32_t L1D_FLUSH : 1;              // 28, since 2018, kernel mode only, IA32_FLUSH_CMD MSR, no new instructions
		uint32_t IA32_ARCH_CAPABILITIES : 1; // 29, since 2018, kernel mode only, Speculative Side Channel Mitigations, no new instructions
		uint32_t IA32_CORE_CAPABILITIES : 1; // 30, since 2018, kernel mode only, Support for a MSR listing model-specific core capabilities, no new instructions
		uint32_t SSBD : 1;                   // 31, since 2018, kernel mode only, Speculative Store Bypass Disable, no new instructions
		// eax_7s1, note: these flags will all be zero if the processor does not support the 7 subleaf 1 CPUID input
		uint32_t eax_7s1_0_to_4 : 5;         // 0 to 4 are reserved
		uint32_t AVX512_BF16 : 1;            // 5, since 2020, AVX-512 BFloat16 (BF16) instruction set, VCVTNE2PS2BF16, VCVTNEPS2BF16 and VDPBF16PS instructions
		uint32_t eax_7s1_5_to_31 : 26;       // 6 to 31 are reserved
		// ebx_0x80000001, note: these flags will all be zero if the processor does not support the 0x80000001 CPUID input
		uint32_t ebx_0x80000001_0_to_31;     // 0 to 31 are reserved
		// ecx_0x80000001, note: these flags will all be zero if the processor does not support the 0x80000001 CPUID input
		uint32_t LAHF_LM : 1;                // 0, since 2005, LAHF and SAHF instructions in long mode
		uint32_t CMP_LEGACY : 1;             // 1, since 2003, old Hyperthreading flag, not valid
		uint32_t SVM : 1;                    // 2, since 2006, Secure Virtual Machine, processor virtualization extensions
		uint32_t EXTAPIC : 1;                // 3, since 2006, kernel mode only, Extended APIC space
		uint32_t CR8_LEGACY : 1;             // 4, since 2006, kernel mode only, CR8 in 32-bit mode
		uint32_t ABM : 1;                    // 5, since 2008, Advanced Bit Manipulation, POPCNT and LZCNT instructions
		uint32_t SSE4A : 1;                  // 6, since 2008, Streaming SIMD Extensions 4a, EXTRQ, INSERTQ, MOVNTSD and MOVNTSS instructions
		uint32_t MISALIGNSSE : 1;            // 7, since 2008, Misaligned SSE mode
		uint32_t A3DNOWPREFETCH : 1;         // 8, since 2003, PREFETCH and PREFETCHW instructions
		uint32_t OSVW : 1;                   // 9, since 2011, kernel mode only, OS Visible Workaround
		uint32_t IBS : 1;                    // 10, since 2007, kernel mode only, Instruction Based Sampling
		uint32_t XOP : 1;                    // 11, since 2011, eXtended Operations instruction set, 55 instructions
		uint32_t SKINIT : 1;                 // 12, since 2005, SKINIT and STGI instructions
		uint32_t WDT : 1;                    // 13, since 2010, kernel mode only, Watchdog timer
		uint32_t ecx_0x80000001_14 : 1;      // 14 is reserved
		uint32_t LWP : 1;                    // 15, since 2010, kernel mode only, Light Weight Profiling
		uint32_t FMA4 : 1;                   // 16, since 2011, 4-operand Fused Multiply-Add, 6 instructions
		uint32_t TCE : 1;                    // 17, since 2013, kernel mode only, Translation Cache Extension
		uint32_t ecx_0x80000001_18 : 1;      // 18 is reserved
		uint32_t NODEID_MSR : 1;             // 19, since 2008, kernel mode only, NodeID MSR
		uint32_t ecx_0x80000001_20 : 1;      // 20 is reserved
		uint32_t TBM : 1;                    // 21, since 2012, Trailing Bit Manipulation, 10 instructions
		uint32_t TOPOEXT : 1;                // 22, since 2012, Topology Extensions (physical attribute of the CPU core)
		uint32_t PERFCTR_CORE : 1;           // 23, since 2012, kernel mode only, Core performance counter extensions
		uint32_t PERFCTR_NB : 1;             // 24, since 2012, kernel mode only, NorthBridge performance counter extensions
		uint32_t ecx_0x80000001_25 : 1;      // 25 is reserved
		uint32_t DBX : 1;                    // 26, since 2013, kernel mode only, Data Breakpoint eXtensions
		uint32_t PERFTSC : 1;                // 27, since 2013, kernel mode only, Performance TSC
		uint32_t PCX_L2I : 1;                // 28, since 2013, kernel mode only, L2I Performance Counter eXtensions
		uint32_t ecx_0x80000001_29_to_31 : 3;// 29 to 31 are reserved
		// edx_0x80000001, note: these flags will all be zero if the processor does not support the 0x80000001 CPUID input
		uint32_t AFPU : 1;                   // 0, do not use, since 1993, onboard x87 floating-point unit, 83 instructions, the x87 coprocessors (since 1980) were still optional on early 486 processors that first supported cpuid, hence this flag
		uint32_t AVME : 1;                   // 1, do not use, since 2005, kernel mode only, Virtual Mode Extensions, hardware virtualization
		uint32_t ADE : 1;                    // 2, do not use, since 1995, Debugging Extension, support for I/O breakpoints
		uint32_t APSE : 1;                   // 3, do not use, since 1995, kernel mode only, Page Size Extensions, adds an optional large page size setting of 4 M (2 M in PAE mode) for page addressing next to the standard 4 k pages
		uint32_t ATSC : 1;                   // 4, do not use, since 1993, RDTSC instruction, this instruction can be priviledged at kernel level only by the time stamp disable (TSD) flag in control register CR4, but no operating systems actually set that flag, __rdtsc()
		uint32_t AMSR : 1;                   // 5, do not use, since 1993, kernel mode only, Model-Specific Registers, RDMSR and WRMSR instruction pair
		uint32_t APAE : 1;                   // 6, do not use, since 1995, kernel mode only, Physical Address Extension, allows usage of more than 32-bit physical addressing space in 32-bit mode
		uint32_t AMCE : 1;                   // 7, do not use, since 1995, kernel mode only, Machine Check Exception, hardware error reporting system
		uint32_t ACX8 : 1;                   // 8, do not use, since 1995, CMPXCHG8B instruction, rarely used in the long mode (acts on 64-bit edx:eax), _InterlockedCompareExchange64()
		uint32_t AAPIC : 1;                  // 9, since 1993, kernel mode only, Advanced Programmable Interrupt Controller, indicates presence of a software-accessible APIC for multiprocessor systems
		uint32_t edx_0x80000001_10 : 1;      // 10 is reserved
		uint32_t SYSCALL : 1;                // 11, since 1997, SYSCALL and SYSRET instructions, implementations of fast system call in user mode to kernel mode and back, some parts of this extension are kernel mode only, this flag is a requirement if long mode long is supported
		uint32_t AMTRR : 1;                  // 12, do not use, since 1995, Memory Type Range Registers, kernel mode only, registers that control caching on specific memory ranges
		uint32_t APGE : 1;                   // 13, do not use, since 1993, Page Global Enable, kernel mode only, allows address translations in multiple processes to be globally tracked, indicates that cache for shared pages does not need to be flushed on context switches
		uint32_t AMCA : 1;                   // 14, do not use, since 2000, kernel mode only, Machine Check Architecture, hardware error reporting system
		uint32_t ACMOV : 1;                  // 15, do not use, since 1995, Conditional Move extensions, 42 instructions
		uint32_t APAT : 1;                   // 16, do not use, since 1999, kernel mode only, Page Attribute Table, allows setting per-page attributes on caching
		uint32_t APSE_36 : 1;                // 17, do not use, since 1997, kernel mode only, 36-bit Page Size Extension, allows usage of more than 32-bit physical addressing space in 32-bit mode, alternative of PAE
		uint32_t edx_0x80000001_18 : 1;      // 18 is reserved
		uint32_t MP : 1;                     // 19, since 2003, Multiprocessor Capable
		uint32_t NX : 1;                     // 20, since 2003, No-eXecute bit extensions, allows the operating system to set up rules for memory for use by either storage of processor instructions, some parts of this extension are kernel mode only
		uint32_t edx_0x80000001_21 : 1;      // 21 is reserved
		uint32_t MMXEXT : 1;                 // 22, since 1998, Extended MMX of the 3DNow! instruction subset
		uint32_t AMMX : 1;                   // 23, do not use, since 1996, MMX instruction set, 57 instructions
		uint32_t AFXSR : 1;                  // 24, do not use, since 1996, kernel mode only, FXSAVE and FXRSTOR instruction pair, context switch save and restore registers
		uint32_t FXSR_OPT : 1;               // 25, since 1998, kernel mode only, FXSAVE and FXRSTOR optimizations
		uint32_t PDPE1GB : 1;                // 26, since 2003, support for Gigabyte-sized pages, some parts of this extension are kernel mode only
		uint32_t RDTSCP : 1;                 // 27, since 2007, RDTSCP instruction, this instruction can be priviledged at kernel level only by the time stamp disable (TSD) flag in control register CR4, but no operating systems actually set that flag, __rdtscp()
		uint32_t edx_0x80000001_28 : 1;      // 28 is reserved
		uint32_t LM : 1;                     // 29, since 2003, Long Mode (x64), some parts of this extension are kernel mode only
		uint32_t A3DNOWEXT: 1;               // 30, since 1999, 3DNow! extensions, 5 instructions
		uint32_t A3DNOW: 1;                  // 31, since 1998, 3DNow! base instruction set, 21 instructions
	};

	__declspec(noalias safebuffers) __forceinline FeatureBitsCPUId()noexcept{
		{
			int32_t rbf[4];
			__cpuid(rbf, 1);
			eax_1 = static_cast<uint32_t>(rbf[0]);// store eax
			ebx_1 = static_cast<uint32_t>(rbf[1]);// store ebx
			ecx_1 = static_cast<uint32_t>(rbf[2]);// store ecx
			edx_1 = static_cast<uint32_t>(rbf[3]);// store edx
		}
		int32_t tmpeax_7s0;
		{
			int32_t rbf[4];
			__cpuid(rbf, 7);
			tmpeax_7s0 = rbf[0];// eax_7s0 indicates the number of subleaf pages past 0
			ebx_7s0 = static_cast<uint32_t>(rbf[1]);// store ebx
			ecx_7s0 = static_cast<uint32_t>(rbf[2]);// store ecx
			edx_7s0 = static_cast<uint32_t>(rbf[3]);// store edx
		}
		int32_t tmpeax_7s1{};
		if(tmpeax_7s0){// 7 subleaf 1 extensions are supported
			int32_t rbf[4];
			__cpuidex(rbf, 7, 1);
			tmpeax_7s1 = rbf[0];
		}
		// ebx, ecx and edx are currently discarded
		eax_7s1 = static_cast<uint32_t>(tmpeax_7s1);// store eax
		/* for if the feature level of 0x80000000 and onward is required:
		int32_t tmpeax_0x80000000;
		{
			int32_t rbf[4];
			__cpuid(rbf, 0x80000000);
			// ebx, ecx and edx are currently discarded
			tmpeax_0x80000000 = rbf[0];
		}*/
		{
			int32_t rbf[4];
			__cpuid(rbf, 0x80000001);
			// eax is currently discarded
			ebx_0x80000001 = static_cast<uint32_t>(rbf[1]);// store ebx
			ecx_0x80000001 = static_cast<uint32_t>(rbf[2]);// store ecx
			edx_0x80000001 = static_cast<uint32_t>(rbf[3]);// store edx
		}
	}
};
static_assert(sizeof FeatureBitsCPUId == 4 * 11, L"the struct size of FeatureBitsCPUId is wrong");

extern __declspec(selectany) FeatureBitsCPUId const gk_FBCPUId;// This auto self-constructs at runtime during global initialization, without any external dependencies in software.

// re-enable warning messages for files including this header
#pragma warning(pop)