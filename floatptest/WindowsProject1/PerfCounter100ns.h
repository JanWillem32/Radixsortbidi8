#pragma once

#include <intrin.h>
#include <profileapi.h>
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC// include Microsoft memory leak detection procedures
#endif
#include <crtdbg.h>

// disable warning messages for this file only:
// C4820: 'n' bytes padding added after data member 'x'
#pragma warning(push)
#pragma warning(disable: 4820)

// lookup tables required for the subroutines that use the "Improved integer division by constants using multiplication" methods
#pragma pack(push, 1)// just to be sure that these arrays get packed perfectly
#ifdef _WIN64
// table lookup for 64-bit words:
// 0x100 to 0x1FF in the right-shifted normalized divisor
// highest output value: 0x7FD00ui32 / 0x100ui32 = 0x7FDui32
// lowest output value: 0x7FD00ui32 / 0x1FFui32 = 0x400ui32
// 11 bits required per table entry
// 352 bytes required if packed
// sampler:
////uint32_t v_0{((1ui32 << 19) - 3ui32 * (1ui32 << 8)) / d_9}; use a table lookup for this:
//// the input is a 9-bit normalized divisor, so there are 256 table entries (the high bit is always set, so there are not 512 table entries)
//uint32_t bitoffset{d_9 * 11ui32};
//uint32_t twobyteoffset{bitoffset >> 4}; bitoffset / 16, using bitoffset / 8 would issue the requirement of 2 bytes of padding at the end of the array because of the 4-byte dereferencing
//uint32_t v_0{*reinterpret_cast<uint32_t UNALIGNED const*>(reinterpret_cast<uint16_t const*>(gk_packedlookuptablev64) - (1 << (9 - 1)) * 11 / 16 + static_cast<size_t>(twobyteoffset))}; table lookup with a compile-time base offset, note: (1 << (9 - 1)) * 11 / 16 only shifts out zeroes at / 16
//uint32_t residualbitoffset{bitoffset & 15ui32}; formally (bitoffset - (1ui32 << (9 - 1)) * 11ui32) & 15ui32, but this can be simplified as the low bits of (1ui32 << (9 - 1)) * 11ui32 are all zero
//v_0 >>= residualbitoffset;
//v_0 &= 0x7FFui32;
struct packedlookuptablev64segment{// note that the compiler refuses to pack bitfields across multiple words, hence this approach
	uint32_t elements[11];
	constexpr packedlookuptablev64segment(uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4, uint32_t v5, uint32_t v6, uint32_t v7,
		uint32_t v8, uint32_t v9, uint32_t v10, uint32_t v11, uint32_t v12, uint32_t v13, uint32_t v14, uint32_t v15,
		uint32_t v16, uint32_t v17, uint32_t v18, uint32_t v19, uint32_t v20, uint32_t v21, uint32_t v22, uint32_t v23,
		uint32_t v24, uint32_t v25, uint32_t v26, uint32_t v27, uint32_t v28, uint32_t v29, uint32_t v30, uint32_t v31)
		: elements{v0 | v1 << 11 | v2 << 22,
			v2 >> 10 | v3 << 1 | v4 << 12 | v5 << 23,
			v5 >> 9 | v6 << 2 | v7 << 13 | v8 << 24,
			v8 >> 8 | v9 << 3 | v10 << 14 | v11 << 25,
			v11 >> 7 | v12 << 4 | v13 << 15 | v14 << 26,
			v14 >> 6 | v15 << 5 | v16 << 16 | v17 << 27,
			v17 >> 5 | v18 << 6 | v19 << 17 | v20 << 28,
			v20 >> 4 | v21 << 7 | v22 << 18 | v23 << 29,
			v23 >> 3 | v24 << 8 | v25 << 19 | v26 << 30,
			v26 >> 2 | v27 << 9 | v28 << 20 | v29 << 31,
			v29 >> 1 | v30 << 10 | v31 << 21}{}
};
extern __declspec(selectany) alignas(64) packedlookuptablev64segment constexpr gk_packedlookuptablev64[8]{// cache line align
	{0x7FD00ui32 / 0x100ui32, 0x7FD00ui32 / 0x101ui32, 0x7FD00ui32 / 0x102ui32, 0x7FD00ui32 / 0x103ui32, 0x7FD00ui32 / 0x104ui32, 0x7FD00ui32 / 0x105ui32, 0x7FD00ui32 / 0x106ui32, 0x7FD00ui32 / 0x107ui32,
	0x7FD00ui32 / 0x108ui32, 0x7FD00ui32 / 0x109ui32, 0x7FD00ui32 / 0x10Aui32, 0x7FD00ui32 / 0x10Bui32, 0x7FD00ui32 / 0x10Cui32, 0x7FD00ui32 / 0x10Dui32, 0x7FD00ui32 / 0x10Eui32, 0x7FD00ui32 / 0x10Fui32,
	0x7FD00ui32 / 0x110ui32, 0x7FD00ui32 / 0x111ui32, 0x7FD00ui32 / 0x112ui32, 0x7FD00ui32 / 0x113ui32, 0x7FD00ui32 / 0x114ui32, 0x7FD00ui32 / 0x115ui32, 0x7FD00ui32 / 0x116ui32, 0x7FD00ui32 / 0x117ui32,
	0x7FD00ui32 / 0x118ui32, 0x7FD00ui32 / 0x119ui32, 0x7FD00ui32 / 0x11Aui32, 0x7FD00ui32 / 0x11Bui32, 0x7FD00ui32 / 0x11Cui32, 0x7FD00ui32 / 0x11Dui32, 0x7FD00ui32 / 0x11Eui32, 0x7FD00ui32 / 0x11Fui32},
	{0x7FD00ui32 / 0x120ui32, 0x7FD00ui32 / 0x121ui32, 0x7FD00ui32 / 0x122ui32, 0x7FD00ui32 / 0x123ui32, 0x7FD00ui32 / 0x124ui32, 0x7FD00ui32 / 0x125ui32, 0x7FD00ui32 / 0x126ui32, 0x7FD00ui32 / 0x127ui32,
	0x7FD00ui32 / 0x128ui32, 0x7FD00ui32 / 0x129ui32, 0x7FD00ui32 / 0x12Aui32, 0x7FD00ui32 / 0x12Bui32, 0x7FD00ui32 / 0x12Cui32, 0x7FD00ui32 / 0x12Dui32, 0x7FD00ui32 / 0x12Eui32, 0x7FD00ui32 / 0x12Fui32,
	0x7FD00ui32 / 0x130ui32, 0x7FD00ui32 / 0x131ui32, 0x7FD00ui32 / 0x132ui32, 0x7FD00ui32 / 0x133ui32, 0x7FD00ui32 / 0x134ui32, 0x7FD00ui32 / 0x135ui32, 0x7FD00ui32 / 0x136ui32, 0x7FD00ui32 / 0x137ui32,
	0x7FD00ui32 / 0x138ui32, 0x7FD00ui32 / 0x139ui32, 0x7FD00ui32 / 0x13Aui32, 0x7FD00ui32 / 0x13Bui32, 0x7FD00ui32 / 0x13Cui32, 0x7FD00ui32 / 0x13Dui32, 0x7FD00ui32 / 0x13Eui32, 0x7FD00ui32 / 0x13Fui32},
	{0x7FD00ui32 / 0x140ui32, 0x7FD00ui32 / 0x141ui32, 0x7FD00ui32 / 0x142ui32, 0x7FD00ui32 / 0x143ui32, 0x7FD00ui32 / 0x144ui32, 0x7FD00ui32 / 0x145ui32, 0x7FD00ui32 / 0x146ui32, 0x7FD00ui32 / 0x147ui32,
	0x7FD00ui32 / 0x148ui32, 0x7FD00ui32 / 0x149ui32, 0x7FD00ui32 / 0x14Aui32, 0x7FD00ui32 / 0x14Bui32, 0x7FD00ui32 / 0x14Cui32, 0x7FD00ui32 / 0x14Dui32, 0x7FD00ui32 / 0x14Eui32, 0x7FD00ui32 / 0x14Fui32,
	0x7FD00ui32 / 0x150ui32, 0x7FD00ui32 / 0x151ui32, 0x7FD00ui32 / 0x152ui32, 0x7FD00ui32 / 0x153ui32, 0x7FD00ui32 / 0x154ui32, 0x7FD00ui32 / 0x155ui32, 0x7FD00ui32 / 0x156ui32, 0x7FD00ui32 / 0x157ui32,
	0x7FD00ui32 / 0x158ui32, 0x7FD00ui32 / 0x159ui32, 0x7FD00ui32 / 0x15Aui32, 0x7FD00ui32 / 0x15Bui32, 0x7FD00ui32 / 0x15Cui32, 0x7FD00ui32 / 0x15Dui32, 0x7FD00ui32 / 0x15Eui32, 0x7FD00ui32 / 0x15Fui32},
	{0x7FD00ui32 / 0x160ui32, 0x7FD00ui32 / 0x161ui32, 0x7FD00ui32 / 0x162ui32, 0x7FD00ui32 / 0x163ui32, 0x7FD00ui32 / 0x164ui32, 0x7FD00ui32 / 0x165ui32, 0x7FD00ui32 / 0x166ui32, 0x7FD00ui32 / 0x167ui32,
	0x7FD00ui32 / 0x168ui32, 0x7FD00ui32 / 0x169ui32, 0x7FD00ui32 / 0x16Aui32, 0x7FD00ui32 / 0x16Bui32, 0x7FD00ui32 / 0x16Cui32, 0x7FD00ui32 / 0x16Dui32, 0x7FD00ui32 / 0x16Eui32, 0x7FD00ui32 / 0x16Fui32,
	0x7FD00ui32 / 0x170ui32, 0x7FD00ui32 / 0x171ui32, 0x7FD00ui32 / 0x172ui32, 0x7FD00ui32 / 0x173ui32, 0x7FD00ui32 / 0x174ui32, 0x7FD00ui32 / 0x175ui32, 0x7FD00ui32 / 0x176ui32, 0x7FD00ui32 / 0x177ui32,
	0x7FD00ui32 / 0x178ui32, 0x7FD00ui32 / 0x179ui32, 0x7FD00ui32 / 0x17Aui32, 0x7FD00ui32 / 0x17Bui32, 0x7FD00ui32 / 0x17Cui32, 0x7FD00ui32 / 0x17Dui32, 0x7FD00ui32 / 0x17Eui32, 0x7FD00ui32 / 0x17Fui32},
	{0x7FD00ui32 / 0x180ui32, 0x7FD00ui32 / 0x181ui32, 0x7FD00ui32 / 0x182ui32, 0x7FD00ui32 / 0x183ui32, 0x7FD00ui32 / 0x184ui32, 0x7FD00ui32 / 0x185ui32, 0x7FD00ui32 / 0x186ui32, 0x7FD00ui32 / 0x187ui32,
	0x7FD00ui32 / 0x188ui32, 0x7FD00ui32 / 0x189ui32, 0x7FD00ui32 / 0x18Aui32, 0x7FD00ui32 / 0x18Bui32, 0x7FD00ui32 / 0x18Cui32, 0x7FD00ui32 / 0x18Dui32, 0x7FD00ui32 / 0x18Eui32, 0x7FD00ui32 / 0x18Fui32,
	0x7FD00ui32 / 0x190ui32, 0x7FD00ui32 / 0x191ui32, 0x7FD00ui32 / 0x192ui32, 0x7FD00ui32 / 0x193ui32, 0x7FD00ui32 / 0x194ui32, 0x7FD00ui32 / 0x195ui32, 0x7FD00ui32 / 0x196ui32, 0x7FD00ui32 / 0x197ui32,
	0x7FD00ui32 / 0x198ui32, 0x7FD00ui32 / 0x199ui32, 0x7FD00ui32 / 0x19Aui32, 0x7FD00ui32 / 0x19Bui32, 0x7FD00ui32 / 0x19Cui32, 0x7FD00ui32 / 0x19Dui32, 0x7FD00ui32 / 0x19Eui32, 0x7FD00ui32 / 0x19Fui32},
	{0x7FD00ui32 / 0x1A0ui32, 0x7FD00ui32 / 0x1A1ui32, 0x7FD00ui32 / 0x1A2ui32, 0x7FD00ui32 / 0x1A3ui32, 0x7FD00ui32 / 0x1A4ui32, 0x7FD00ui32 / 0x1A5ui32, 0x7FD00ui32 / 0x1A6ui32, 0x7FD00ui32 / 0x1A7ui32,
	0x7FD00ui32 / 0x1A8ui32, 0x7FD00ui32 / 0x1A9ui32, 0x7FD00ui32 / 0x1AAui32, 0x7FD00ui32 / 0x1ABui32, 0x7FD00ui32 / 0x1ACui32, 0x7FD00ui32 / 0x1ADui32, 0x7FD00ui32 / 0x1AEui32, 0x7FD00ui32 / 0x1AFui32,
	0x7FD00ui32 / 0x1B0ui32, 0x7FD00ui32 / 0x1B1ui32, 0x7FD00ui32 / 0x1B2ui32, 0x7FD00ui32 / 0x1B3ui32, 0x7FD00ui32 / 0x1B4ui32, 0x7FD00ui32 / 0x1B5ui32, 0x7FD00ui32 / 0x1B6ui32, 0x7FD00ui32 / 0x1B7ui32,
	0x7FD00ui32 / 0x1B8ui32, 0x7FD00ui32 / 0x1B9ui32, 0x7FD00ui32 / 0x1BAui32, 0x7FD00ui32 / 0x1BBui32, 0x7FD00ui32 / 0x1BCui32, 0x7FD00ui32 / 0x1BDui32, 0x7FD00ui32 / 0x1BEui32, 0x7FD00ui32 / 0x1BFui32},
	{0x7FD00ui32 / 0x1C0ui32, 0x7FD00ui32 / 0x1C1ui32, 0x7FD00ui32 / 0x1C2ui32, 0x7FD00ui32 / 0x1C3ui32, 0x7FD00ui32 / 0x1C4ui32, 0x7FD00ui32 / 0x1C5ui32, 0x7FD00ui32 / 0x1C6ui32, 0x7FD00ui32 / 0x1C7ui32,
	0x7FD00ui32 / 0x1C8ui32, 0x7FD00ui32 / 0x1C9ui32, 0x7FD00ui32 / 0x1CAui32, 0x7FD00ui32 / 0x1CBui32, 0x7FD00ui32 / 0x1CCui32, 0x7FD00ui32 / 0x1CDui32, 0x7FD00ui32 / 0x1CEui32, 0x7FD00ui32 / 0x1CFui32,
	0x7FD00ui32 / 0x1D0ui32, 0x7FD00ui32 / 0x1D1ui32, 0x7FD00ui32 / 0x1D2ui32, 0x7FD00ui32 / 0x1D3ui32, 0x7FD00ui32 / 0x1D4ui32, 0x7FD00ui32 / 0x1D5ui32, 0x7FD00ui32 / 0x1D6ui32, 0x7FD00ui32 / 0x1D7ui32,
	0x7FD00ui32 / 0x1D8ui32, 0x7FD00ui32 / 0x1D9ui32, 0x7FD00ui32 / 0x1DAui32, 0x7FD00ui32 / 0x1DBui32, 0x7FD00ui32 / 0x1DCui32, 0x7FD00ui32 / 0x1DDui32, 0x7FD00ui32 / 0x1DEui32, 0x7FD00ui32 / 0x1DFui32},
	{0x7FD00ui32 / 0x1E0ui32, 0x7FD00ui32 / 0x1E1ui32, 0x7FD00ui32 / 0x1E2ui32, 0x7FD00ui32 / 0x1E3ui32, 0x7FD00ui32 / 0x1E4ui32, 0x7FD00ui32 / 0x1E5ui32, 0x7FD00ui32 / 0x1E6ui32, 0x7FD00ui32 / 0x1E7ui32,
	0x7FD00ui32 / 0x1E8ui32, 0x7FD00ui32 / 0x1E9ui32, 0x7FD00ui32 / 0x1EAui32, 0x7FD00ui32 / 0x1EBui32, 0x7FD00ui32 / 0x1ECui32, 0x7FD00ui32 / 0x1EDui32, 0x7FD00ui32 / 0x1EEui32, 0x7FD00ui32 / 0x1EFui32,
	0x7FD00ui32 / 0x1F0ui32, 0x7FD00ui32 / 0x1F1ui32, 0x7FD00ui32 / 0x1F2ui32, 0x7FD00ui32 / 0x1F3ui32, 0x7FD00ui32 / 0x1F4ui32, 0x7FD00ui32 / 0x1F5ui32, 0x7FD00ui32 / 0x1F6ui32, 0x7FD00ui32 / 0x1F7ui32,
	0x7FD00ui32 / 0x1F8ui32, 0x7FD00ui32 / 0x1F9ui32, 0x7FD00ui32 / 0x1FAui32, 0x7FD00ui32 / 0x1FBui32, 0x7FD00ui32 / 0x1FCui32, 0x7FD00ui32 / 0x1FDui32, 0x7FD00ui32 / 0x1FEui32, 0x7FD00ui32 / 0x1FFui32}};
static_assert(sizeof gk_packedlookuptablev64 == 11 * 256 / 8, L"array gk_packedlookuptablev64 or platform settings changed");
#endif
// table lookup for 32-bit words:
// 0x200 to 0x3FF in the right-shifted normalized divisor
// highest output value: 0xFFC200ui32 / 0x200ui32 = 0x7FE1ui32
// lowest output value: 0xFFC200ui32 / 0x3FFui32 = 0x4000ui32
// 15 bits required per table entry
// 960 bytes required if packed
// sampler:
////uint32_t v_0{((1ui32 << 24) - (1ui32 << 14) + (1ui32 << 9)) / d_10}; use a table lookup for this:
//// the input is a 10-bit normalized divisor, so there are 512 table entries (the high bit is always set, so there are not 1024 table entries)
//uint32_t bitoffset{d_10 << 4}; bitoffset = d_10 * 15ui32
//bitoffset -= d_10;
//uint32_t twobyteoffset{bitoffset >> 4}; bitoffset / 16, using bitoffset / 8 would issue the requirement of 2 bytes of padding at the end of the array because of the 4-byte dereferencing
//uint32_t residualbitoffset{bitoffset & 15ui32}; formally (bitoffset - (1ui32 << (10 - 1)) * 15ui32) & 15ui32, but this can be simplified as the low bits of (1ui32 << (10 - 1)) * 15ui32 are all zero
//uint32_t v_0{*reinterpret_cast<uint32_t UNALIGNED const*>(reinterpret_cast<uint16_t const*>(gk_packedlookuptablev32) - (1 << (10 - 1)) * 15 / 16 + static_cast<size_t>(twobyteoffset))}; single-instruction table lookup with a compile-time base offset, note: (1 << (10 - 1)) * 15 / 16 only shifts out zeroes at / 16
//v_0 >>= residualbitoffset;
//v_0 &= 0x7FFFui32;
struct packedlookuptablev32segment{// note that the compiler refuses to pack bitfields across multiple words, hence this approach
	uint32_t elements[15];
	constexpr packedlookuptablev32segment(uint32_t v0, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4, uint32_t v5, uint32_t v6, uint32_t v7,
		uint32_t v8, uint32_t v9, uint32_t v10, uint32_t v11, uint32_t v12, uint32_t v13, uint32_t v14, uint32_t v15,
		uint32_t v16, uint32_t v17, uint32_t v18, uint32_t v19, uint32_t v20, uint32_t v21, uint32_t v22, uint32_t v23,
		uint32_t v24, uint32_t v25, uint32_t v26, uint32_t v27, uint32_t v28, uint32_t v29, uint32_t v30, uint32_t v31)
		: elements{v0 | v1 << 15 | v2 << 30,
			v2 >> 2 | v3 << 13 | v4 << 28,
			v4 >> 4 | v5 << 11 | v6 << 26,
			v6 >> 6 | v7 << 9 | v8 << 24,
			v8 >> 8 | v9 << 7 | v10 << 22,
			v10 >> 10 | v11 << 5 | v12 << 20,
			v12 >> 12 | v13 << 3 | v14 << 18,
			v14 >> 14 | v15 << 1 | v16 << 16 | v17 << 31,
			v17 >> 1 | v18 << 14 | v19 << 29,
			v19 >> 3 | v20 << 12 | v21 << 27,
			v21 >> 5 | v22 << 10 | v23 << 25,
			v23 >> 7 | v24 << 8 | v25 << 23,
			v25 >> 9 | v26 << 6 | v27 << 21,
			v27 >> 11 | v28 << 4 | v29 << 19,
			v29 >> 13 | v30 << 2 | v31 << 17}{}
};
extern __declspec(selectany) alignas(64) packedlookuptablev32segment constexpr gk_packedlookuptablev32[16]{// cache line align
	{0xFFC200ui32 / 0x200ui32, 0xFFC200ui32 / 0x201ui32, 0xFFC200ui32 / 0x202ui32, 0xFFC200ui32 / 0x203ui32, 0xFFC200ui32 / 0x204ui32, 0xFFC200ui32 / 0x205ui32, 0xFFC200ui32 / 0x206ui32, 0xFFC200ui32 / 0x207ui32,
	0xFFC200ui32 / 0x208ui32, 0xFFC200ui32 / 0x209ui32, 0xFFC200ui32 / 0x20Aui32, 0xFFC200ui32 / 0x20Bui32, 0xFFC200ui32 / 0x20Cui32, 0xFFC200ui32 / 0x20Dui32, 0xFFC200ui32 / 0x20Eui32, 0xFFC200ui32 / 0x20Fui32,
	0xFFC200ui32 / 0x210ui32, 0xFFC200ui32 / 0x211ui32, 0xFFC200ui32 / 0x212ui32, 0xFFC200ui32 / 0x213ui32, 0xFFC200ui32 / 0x214ui32, 0xFFC200ui32 / 0x215ui32, 0xFFC200ui32 / 0x216ui32, 0xFFC200ui32 / 0x217ui32,
	0xFFC200ui32 / 0x218ui32, 0xFFC200ui32 / 0x219ui32, 0xFFC200ui32 / 0x21Aui32, 0xFFC200ui32 / 0x21Bui32, 0xFFC200ui32 / 0x21Cui32, 0xFFC200ui32 / 0x21Dui32, 0xFFC200ui32 / 0x21Eui32, 0xFFC200ui32 / 0x21Fui32},
	{0xFFC200ui32 / 0x220ui32, 0xFFC200ui32 / 0x221ui32, 0xFFC200ui32 / 0x222ui32, 0xFFC200ui32 / 0x223ui32, 0xFFC200ui32 / 0x224ui32, 0xFFC200ui32 / 0x225ui32, 0xFFC200ui32 / 0x226ui32, 0xFFC200ui32 / 0x227ui32,
	0xFFC200ui32 / 0x228ui32, 0xFFC200ui32 / 0x229ui32, 0xFFC200ui32 / 0x22Aui32, 0xFFC200ui32 / 0x22Bui32, 0xFFC200ui32 / 0x22Cui32, 0xFFC200ui32 / 0x22Dui32, 0xFFC200ui32 / 0x22Eui32, 0xFFC200ui32 / 0x22Fui32,
	0xFFC200ui32 / 0x230ui32, 0xFFC200ui32 / 0x231ui32, 0xFFC200ui32 / 0x232ui32, 0xFFC200ui32 / 0x233ui32, 0xFFC200ui32 / 0x234ui32, 0xFFC200ui32 / 0x235ui32, 0xFFC200ui32 / 0x236ui32, 0xFFC200ui32 / 0x237ui32,
	0xFFC200ui32 / 0x238ui32, 0xFFC200ui32 / 0x239ui32, 0xFFC200ui32 / 0x23Aui32, 0xFFC200ui32 / 0x23Bui32, 0xFFC200ui32 / 0x23Cui32, 0xFFC200ui32 / 0x23Dui32, 0xFFC200ui32 / 0x23Eui32, 0xFFC200ui32 / 0x23Fui32},
	{0xFFC200ui32 / 0x240ui32, 0xFFC200ui32 / 0x241ui32, 0xFFC200ui32 / 0x242ui32, 0xFFC200ui32 / 0x243ui32, 0xFFC200ui32 / 0x244ui32, 0xFFC200ui32 / 0x245ui32, 0xFFC200ui32 / 0x246ui32, 0xFFC200ui32 / 0x247ui32,
	0xFFC200ui32 / 0x248ui32, 0xFFC200ui32 / 0x249ui32, 0xFFC200ui32 / 0x24Aui32, 0xFFC200ui32 / 0x24Bui32, 0xFFC200ui32 / 0x24Cui32, 0xFFC200ui32 / 0x24Dui32, 0xFFC200ui32 / 0x24Eui32, 0xFFC200ui32 / 0x24Fui32,
	0xFFC200ui32 / 0x250ui32, 0xFFC200ui32 / 0x251ui32, 0xFFC200ui32 / 0x252ui32, 0xFFC200ui32 / 0x253ui32, 0xFFC200ui32 / 0x254ui32, 0xFFC200ui32 / 0x255ui32, 0xFFC200ui32 / 0x256ui32, 0xFFC200ui32 / 0x257ui32,
	0xFFC200ui32 / 0x258ui32, 0xFFC200ui32 / 0x259ui32, 0xFFC200ui32 / 0x25Aui32, 0xFFC200ui32 / 0x25Bui32, 0xFFC200ui32 / 0x25Cui32, 0xFFC200ui32 / 0x25Dui32, 0xFFC200ui32 / 0x25Eui32, 0xFFC200ui32 / 0x25Fui32},
	{0xFFC200ui32 / 0x260ui32, 0xFFC200ui32 / 0x261ui32, 0xFFC200ui32 / 0x262ui32, 0xFFC200ui32 / 0x263ui32, 0xFFC200ui32 / 0x264ui32, 0xFFC200ui32 / 0x265ui32, 0xFFC200ui32 / 0x266ui32, 0xFFC200ui32 / 0x267ui32,
	0xFFC200ui32 / 0x268ui32, 0xFFC200ui32 / 0x269ui32, 0xFFC200ui32 / 0x26Aui32, 0xFFC200ui32 / 0x26Bui32, 0xFFC200ui32 / 0x26Cui32, 0xFFC200ui32 / 0x26Dui32, 0xFFC200ui32 / 0x26Eui32, 0xFFC200ui32 / 0x26Fui32,
	0xFFC200ui32 / 0x270ui32, 0xFFC200ui32 / 0x271ui32, 0xFFC200ui32 / 0x272ui32, 0xFFC200ui32 / 0x273ui32, 0xFFC200ui32 / 0x274ui32, 0xFFC200ui32 / 0x275ui32, 0xFFC200ui32 / 0x276ui32, 0xFFC200ui32 / 0x277ui32,
	0xFFC200ui32 / 0x278ui32, 0xFFC200ui32 / 0x279ui32, 0xFFC200ui32 / 0x27Aui32, 0xFFC200ui32 / 0x27Bui32, 0xFFC200ui32 / 0x27Cui32, 0xFFC200ui32 / 0x27Dui32, 0xFFC200ui32 / 0x27Eui32, 0xFFC200ui32 / 0x27Fui32},
	{0xFFC200ui32 / 0x280ui32, 0xFFC200ui32 / 0x281ui32, 0xFFC200ui32 / 0x282ui32, 0xFFC200ui32 / 0x283ui32, 0xFFC200ui32 / 0x284ui32, 0xFFC200ui32 / 0x285ui32, 0xFFC200ui32 / 0x286ui32, 0xFFC200ui32 / 0x287ui32,
	0xFFC200ui32 / 0x288ui32, 0xFFC200ui32 / 0x289ui32, 0xFFC200ui32 / 0x28Aui32, 0xFFC200ui32 / 0x28Bui32, 0xFFC200ui32 / 0x28Cui32, 0xFFC200ui32 / 0x28Dui32, 0xFFC200ui32 / 0x28Eui32, 0xFFC200ui32 / 0x28Fui32,
	0xFFC200ui32 / 0x290ui32, 0xFFC200ui32 / 0x291ui32, 0xFFC200ui32 / 0x292ui32, 0xFFC200ui32 / 0x293ui32, 0xFFC200ui32 / 0x294ui32, 0xFFC200ui32 / 0x295ui32, 0xFFC200ui32 / 0x296ui32, 0xFFC200ui32 / 0x297ui32,
	0xFFC200ui32 / 0x298ui32, 0xFFC200ui32 / 0x299ui32, 0xFFC200ui32 / 0x29Aui32, 0xFFC200ui32 / 0x29Bui32, 0xFFC200ui32 / 0x29Cui32, 0xFFC200ui32 / 0x29Dui32, 0xFFC200ui32 / 0x29Eui32, 0xFFC200ui32 / 0x29Fui32},
	{0xFFC200ui32 / 0x2A0ui32, 0xFFC200ui32 / 0x2A1ui32, 0xFFC200ui32 / 0x2A2ui32, 0xFFC200ui32 / 0x2A3ui32, 0xFFC200ui32 / 0x2A4ui32, 0xFFC200ui32 / 0x2A5ui32, 0xFFC200ui32 / 0x2A6ui32, 0xFFC200ui32 / 0x2A7ui32,
	0xFFC200ui32 / 0x2A8ui32, 0xFFC200ui32 / 0x2A9ui32, 0xFFC200ui32 / 0x2AAui32, 0xFFC200ui32 / 0x2ABui32, 0xFFC200ui32 / 0x2ACui32, 0xFFC200ui32 / 0x2ADui32, 0xFFC200ui32 / 0x2AEui32, 0xFFC200ui32 / 0x2AFui32,
	0xFFC200ui32 / 0x2B0ui32, 0xFFC200ui32 / 0x2B1ui32, 0xFFC200ui32 / 0x2B2ui32, 0xFFC200ui32 / 0x2B3ui32, 0xFFC200ui32 / 0x2B4ui32, 0xFFC200ui32 / 0x2B5ui32, 0xFFC200ui32 / 0x2B6ui32, 0xFFC200ui32 / 0x2B7ui32,
	0xFFC200ui32 / 0x2B8ui32, 0xFFC200ui32 / 0x2B9ui32, 0xFFC200ui32 / 0x2BAui32, 0xFFC200ui32 / 0x2BBui32, 0xFFC200ui32 / 0x2BCui32, 0xFFC200ui32 / 0x2BDui32, 0xFFC200ui32 / 0x2BEui32, 0xFFC200ui32 / 0x2BFui32},
	{0xFFC200ui32 / 0x2C0ui32, 0xFFC200ui32 / 0x2C1ui32, 0xFFC200ui32 / 0x2C2ui32, 0xFFC200ui32 / 0x2C3ui32, 0xFFC200ui32 / 0x2C4ui32, 0xFFC200ui32 / 0x2C5ui32, 0xFFC200ui32 / 0x2C6ui32, 0xFFC200ui32 / 0x2C7ui32,
	0xFFC200ui32 / 0x2C8ui32, 0xFFC200ui32 / 0x2C9ui32, 0xFFC200ui32 / 0x2CAui32, 0xFFC200ui32 / 0x2CBui32, 0xFFC200ui32 / 0x2CCui32, 0xFFC200ui32 / 0x2CDui32, 0xFFC200ui32 / 0x2CEui32, 0xFFC200ui32 / 0x2CFui32,
	0xFFC200ui32 / 0x2D0ui32, 0xFFC200ui32 / 0x2D1ui32, 0xFFC200ui32 / 0x2D2ui32, 0xFFC200ui32 / 0x2D3ui32, 0xFFC200ui32 / 0x2D4ui32, 0xFFC200ui32 / 0x2D5ui32, 0xFFC200ui32 / 0x2D6ui32, 0xFFC200ui32 / 0x2D7ui32,
	0xFFC200ui32 / 0x2D8ui32, 0xFFC200ui32 / 0x2D9ui32, 0xFFC200ui32 / 0x2DAui32, 0xFFC200ui32 / 0x2DBui32, 0xFFC200ui32 / 0x2DCui32, 0xFFC200ui32 / 0x2DDui32, 0xFFC200ui32 / 0x2DEui32, 0xFFC200ui32 / 0x2DFui32},
	{0xFFC200ui32 / 0x2E0ui32, 0xFFC200ui32 / 0x2E1ui32, 0xFFC200ui32 / 0x2E2ui32, 0xFFC200ui32 / 0x2E3ui32, 0xFFC200ui32 / 0x2E4ui32, 0xFFC200ui32 / 0x2E5ui32, 0xFFC200ui32 / 0x2E6ui32, 0xFFC200ui32 / 0x2E7ui32,
	0xFFC200ui32 / 0x2E8ui32, 0xFFC200ui32 / 0x2E9ui32, 0xFFC200ui32 / 0x2EAui32, 0xFFC200ui32 / 0x2EBui32, 0xFFC200ui32 / 0x2ECui32, 0xFFC200ui32 / 0x2EDui32, 0xFFC200ui32 / 0x2EEui32, 0xFFC200ui32 / 0x2EFui32,
	0xFFC200ui32 / 0x2F0ui32, 0xFFC200ui32 / 0x2F1ui32, 0xFFC200ui32 / 0x2F2ui32, 0xFFC200ui32 / 0x2F3ui32, 0xFFC200ui32 / 0x2F4ui32, 0xFFC200ui32 / 0x2F5ui32, 0xFFC200ui32 / 0x2F6ui32, 0xFFC200ui32 / 0x2F7ui32,
	0xFFC200ui32 / 0x2F8ui32, 0xFFC200ui32 / 0x2F9ui32, 0xFFC200ui32 / 0x2FAui32, 0xFFC200ui32 / 0x2FBui32, 0xFFC200ui32 / 0x2FCui32, 0xFFC200ui32 / 0x2FDui32, 0xFFC200ui32 / 0x2FEui32, 0xFFC200ui32 / 0x2FFui32},
	{0xFFC200ui32 / 0x300ui32, 0xFFC200ui32 / 0x301ui32, 0xFFC200ui32 / 0x302ui32, 0xFFC200ui32 / 0x303ui32, 0xFFC200ui32 / 0x304ui32, 0xFFC200ui32 / 0x305ui32, 0xFFC200ui32 / 0x306ui32, 0xFFC200ui32 / 0x307ui32,
	0xFFC200ui32 / 0x308ui32, 0xFFC200ui32 / 0x309ui32, 0xFFC200ui32 / 0x30Aui32, 0xFFC200ui32 / 0x30Bui32, 0xFFC200ui32 / 0x30Cui32, 0xFFC200ui32 / 0x30Dui32, 0xFFC200ui32 / 0x30Eui32, 0xFFC200ui32 / 0x30Fui32,
	0xFFC200ui32 / 0x310ui32, 0xFFC200ui32 / 0x311ui32, 0xFFC200ui32 / 0x312ui32, 0xFFC200ui32 / 0x313ui32, 0xFFC200ui32 / 0x314ui32, 0xFFC200ui32 / 0x315ui32, 0xFFC200ui32 / 0x316ui32, 0xFFC200ui32 / 0x317ui32,
	0xFFC200ui32 / 0x318ui32, 0xFFC200ui32 / 0x319ui32, 0xFFC200ui32 / 0x31Aui32, 0xFFC200ui32 / 0x31Bui32, 0xFFC200ui32 / 0x31Cui32, 0xFFC200ui32 / 0x31Dui32, 0xFFC200ui32 / 0x31Eui32, 0xFFC200ui32 / 0x31Fui32},
	{0xFFC200ui32 / 0x320ui32, 0xFFC200ui32 / 0x321ui32, 0xFFC200ui32 / 0x322ui32, 0xFFC200ui32 / 0x323ui32, 0xFFC200ui32 / 0x324ui32, 0xFFC200ui32 / 0x325ui32, 0xFFC200ui32 / 0x326ui32, 0xFFC200ui32 / 0x327ui32,
	0xFFC200ui32 / 0x328ui32, 0xFFC200ui32 / 0x329ui32, 0xFFC200ui32 / 0x32Aui32, 0xFFC200ui32 / 0x32Bui32, 0xFFC200ui32 / 0x32Cui32, 0xFFC200ui32 / 0x32Dui32, 0xFFC200ui32 / 0x32Eui32, 0xFFC200ui32 / 0x32Fui32,
	0xFFC200ui32 / 0x330ui32, 0xFFC200ui32 / 0x331ui32, 0xFFC200ui32 / 0x332ui32, 0xFFC200ui32 / 0x333ui32, 0xFFC200ui32 / 0x334ui32, 0xFFC200ui32 / 0x335ui32, 0xFFC200ui32 / 0x336ui32, 0xFFC200ui32 / 0x337ui32,
	0xFFC200ui32 / 0x338ui32, 0xFFC200ui32 / 0x339ui32, 0xFFC200ui32 / 0x33Aui32, 0xFFC200ui32 / 0x33Bui32, 0xFFC200ui32 / 0x33Cui32, 0xFFC200ui32 / 0x33Dui32, 0xFFC200ui32 / 0x33Eui32, 0xFFC200ui32 / 0x33Fui32},
	{0xFFC200ui32 / 0x340ui32, 0xFFC200ui32 / 0x341ui32, 0xFFC200ui32 / 0x342ui32, 0xFFC200ui32 / 0x343ui32, 0xFFC200ui32 / 0x344ui32, 0xFFC200ui32 / 0x345ui32, 0xFFC200ui32 / 0x346ui32, 0xFFC200ui32 / 0x347ui32,
	0xFFC200ui32 / 0x348ui32, 0xFFC200ui32 / 0x349ui32, 0xFFC200ui32 / 0x34Aui32, 0xFFC200ui32 / 0x34Bui32, 0xFFC200ui32 / 0x34Cui32, 0xFFC200ui32 / 0x34Dui32, 0xFFC200ui32 / 0x34Eui32, 0xFFC200ui32 / 0x34Fui32,
	0xFFC200ui32 / 0x350ui32, 0xFFC200ui32 / 0x351ui32, 0xFFC200ui32 / 0x352ui32, 0xFFC200ui32 / 0x353ui32, 0xFFC200ui32 / 0x354ui32, 0xFFC200ui32 / 0x355ui32, 0xFFC200ui32 / 0x356ui32, 0xFFC200ui32 / 0x357ui32,
	0xFFC200ui32 / 0x358ui32, 0xFFC200ui32 / 0x359ui32, 0xFFC200ui32 / 0x35Aui32, 0xFFC200ui32 / 0x35Bui32, 0xFFC200ui32 / 0x35Cui32, 0xFFC200ui32 / 0x35Dui32, 0xFFC200ui32 / 0x35Eui32, 0xFFC200ui32 / 0x35Fui32},
	{0xFFC200ui32 / 0x360ui32, 0xFFC200ui32 / 0x361ui32, 0xFFC200ui32 / 0x362ui32, 0xFFC200ui32 / 0x363ui32, 0xFFC200ui32 / 0x364ui32, 0xFFC200ui32 / 0x365ui32, 0xFFC200ui32 / 0x366ui32, 0xFFC200ui32 / 0x367ui32,
	0xFFC200ui32 / 0x368ui32, 0xFFC200ui32 / 0x369ui32, 0xFFC200ui32 / 0x36Aui32, 0xFFC200ui32 / 0x36Bui32, 0xFFC200ui32 / 0x36Cui32, 0xFFC200ui32 / 0x36Dui32, 0xFFC200ui32 / 0x36Eui32, 0xFFC200ui32 / 0x36Fui32,
	0xFFC200ui32 / 0x370ui32, 0xFFC200ui32 / 0x371ui32, 0xFFC200ui32 / 0x372ui32, 0xFFC200ui32 / 0x373ui32, 0xFFC200ui32 / 0x374ui32, 0xFFC200ui32 / 0x375ui32, 0xFFC200ui32 / 0x376ui32, 0xFFC200ui32 / 0x377ui32,
	0xFFC200ui32 / 0x378ui32, 0xFFC200ui32 / 0x379ui32, 0xFFC200ui32 / 0x37Aui32, 0xFFC200ui32 / 0x37Bui32, 0xFFC200ui32 / 0x37Cui32, 0xFFC200ui32 / 0x37Dui32, 0xFFC200ui32 / 0x37Eui32, 0xFFC200ui32 / 0x37Fui32},
	{0xFFC200ui32 / 0x380ui32, 0xFFC200ui32 / 0x381ui32, 0xFFC200ui32 / 0x382ui32, 0xFFC200ui32 / 0x383ui32, 0xFFC200ui32 / 0x384ui32, 0xFFC200ui32 / 0x385ui32, 0xFFC200ui32 / 0x386ui32, 0xFFC200ui32 / 0x387ui32,
	0xFFC200ui32 / 0x388ui32, 0xFFC200ui32 / 0x389ui32, 0xFFC200ui32 / 0x38Aui32, 0xFFC200ui32 / 0x38Bui32, 0xFFC200ui32 / 0x38Cui32, 0xFFC200ui32 / 0x38Dui32, 0xFFC200ui32 / 0x38Eui32, 0xFFC200ui32 / 0x38Fui32,
	0xFFC200ui32 / 0x390ui32, 0xFFC200ui32 / 0x391ui32, 0xFFC200ui32 / 0x392ui32, 0xFFC200ui32 / 0x393ui32, 0xFFC200ui32 / 0x394ui32, 0xFFC200ui32 / 0x395ui32, 0xFFC200ui32 / 0x396ui32, 0xFFC200ui32 / 0x397ui32,
	0xFFC200ui32 / 0x398ui32, 0xFFC200ui32 / 0x399ui32, 0xFFC200ui32 / 0x39Aui32, 0xFFC200ui32 / 0x39Bui32, 0xFFC200ui32 / 0x39Cui32, 0xFFC200ui32 / 0x39Dui32, 0xFFC200ui32 / 0x39Eui32, 0xFFC200ui32 / 0x39Fui32},
	{0xFFC200ui32 / 0x3A0ui32, 0xFFC200ui32 / 0x3A1ui32, 0xFFC200ui32 / 0x3A2ui32, 0xFFC200ui32 / 0x3A3ui32, 0xFFC200ui32 / 0x3A4ui32, 0xFFC200ui32 / 0x3A5ui32, 0xFFC200ui32 / 0x3A6ui32, 0xFFC200ui32 / 0x3A7ui32,
	0xFFC200ui32 / 0x3A8ui32, 0xFFC200ui32 / 0x3A9ui32, 0xFFC200ui32 / 0x3AAui32, 0xFFC200ui32 / 0x3ABui32, 0xFFC200ui32 / 0x3ACui32, 0xFFC200ui32 / 0x3ADui32, 0xFFC200ui32 / 0x3AEui32, 0xFFC200ui32 / 0x3AFui32,
	0xFFC200ui32 / 0x3B0ui32, 0xFFC200ui32 / 0x3B1ui32, 0xFFC200ui32 / 0x3B2ui32, 0xFFC200ui32 / 0x3B3ui32, 0xFFC200ui32 / 0x3B4ui32, 0xFFC200ui32 / 0x3B5ui32, 0xFFC200ui32 / 0x3B6ui32, 0xFFC200ui32 / 0x3B7ui32,
	0xFFC200ui32 / 0x3B8ui32, 0xFFC200ui32 / 0x3B9ui32, 0xFFC200ui32 / 0x3BAui32, 0xFFC200ui32 / 0x3BBui32, 0xFFC200ui32 / 0x3BCui32, 0xFFC200ui32 / 0x3BDui32, 0xFFC200ui32 / 0x3BEui32, 0xFFC200ui32 / 0x3BFui32},
	{0xFFC200ui32 / 0x3C0ui32, 0xFFC200ui32 / 0x3C1ui32, 0xFFC200ui32 / 0x3C2ui32, 0xFFC200ui32 / 0x3C3ui32, 0xFFC200ui32 / 0x3C4ui32, 0xFFC200ui32 / 0x3C5ui32, 0xFFC200ui32 / 0x3C6ui32, 0xFFC200ui32 / 0x3C7ui32,
	0xFFC200ui32 / 0x3C8ui32, 0xFFC200ui32 / 0x3C9ui32, 0xFFC200ui32 / 0x3CAui32, 0xFFC200ui32 / 0x3CBui32, 0xFFC200ui32 / 0x3CCui32, 0xFFC200ui32 / 0x3CDui32, 0xFFC200ui32 / 0x3CEui32, 0xFFC200ui32 / 0x3CFui32,
	0xFFC200ui32 / 0x3D0ui32, 0xFFC200ui32 / 0x3D1ui32, 0xFFC200ui32 / 0x3D2ui32, 0xFFC200ui32 / 0x3D3ui32, 0xFFC200ui32 / 0x3D4ui32, 0xFFC200ui32 / 0x3D5ui32, 0xFFC200ui32 / 0x3D6ui32, 0xFFC200ui32 / 0x3D7ui32,
	0xFFC200ui32 / 0x3D8ui32, 0xFFC200ui32 / 0x3D9ui32, 0xFFC200ui32 / 0x3DAui32, 0xFFC200ui32 / 0x3DBui32, 0xFFC200ui32 / 0x3DCui32, 0xFFC200ui32 / 0x3DDui32, 0xFFC200ui32 / 0x3DEui32, 0xFFC200ui32 / 0x3DFui32},
	{0xFFC200ui32 / 0x3E0ui32, 0xFFC200ui32 / 0x3E1ui32, 0xFFC200ui32 / 0x3E2ui32, 0xFFC200ui32 / 0x3E3ui32, 0xFFC200ui32 / 0x3E4ui32, 0xFFC200ui32 / 0x3E5ui32, 0xFFC200ui32 / 0x3E6ui32, 0xFFC200ui32 / 0x3E7ui32,
	0xFFC200ui32 / 0x3E8ui32, 0xFFC200ui32 / 0x3E9ui32, 0xFFC200ui32 / 0x3EAui32, 0xFFC200ui32 / 0x3EBui32, 0xFFC200ui32 / 0x3ECui32, 0xFFC200ui32 / 0x3EDui32, 0xFFC200ui32 / 0x3EEui32, 0xFFC200ui32 / 0x3EFui32,
	0xFFC200ui32 / 0x3F0ui32, 0xFFC200ui32 / 0x3F1ui32, 0xFFC200ui32 / 0x3F2ui32, 0xFFC200ui32 / 0x3F3ui32, 0xFFC200ui32 / 0x3F4ui32, 0xFFC200ui32 / 0x3F5ui32, 0xFFC200ui32 / 0x3F6ui32, 0xFFC200ui32 / 0x3F7ui32,
	0xFFC200ui32 / 0x3F8ui32, 0xFFC200ui32 / 0x3F9ui32, 0xFFC200ui32 / 0x3FAui32, 0xFFC200ui32 / 0x3FBui32, 0xFFC200ui32 / 0x3FCui32, 0xFFC200ui32 / 0x3FDui32, 0xFFC200ui32 / 0x3FEui32, 0xFFC200ui32 / 0x3FFui32}};
static_assert(sizeof gk_packedlookuptablev32 == 15 * 512 / 8, L"array gk_packedlookuptablev32 or platform settings changed");
#pragma pack(pop)

struct QPFDivisorConstants{
#ifdef _WIN64
	uint64_t m_d_norm, m_mprime;
#else
	uint32_t m_d_normlo, m_d_normhi, m_v;
#endif
	uint8_t m_Nml;
	__declspec(noalias safebuffers) __forceinline QPFDivisorConstants(){
		LARGE_INTEGER pf;
		BOOL bos{QueryPerformanceFrequency(&pf)};
		static_cast<void>(bos);
		assert(bos);// guaranteed since Windows XP
		uint64_t d{static_cast<uint64_t>(pf.QuadPart)};// non-zero, positive divisor
		assert(d);

		// set up the run-time invariant values for the 128-to-64-bit unsigned division by invariant integers using multiplication routine
#ifdef _WIN64
		// This routine depends on regular compiler intrinsics and gk_packedlookuptablev64[].
		// initialization phase, store these variables for future use: d_norm, mprime and Nml
#else
		// This routine depends on regular compiler intrinsics and gk_packedlookuptablev32[].
		// initialization phase, store these variables for future use: d_normlo, d_normhi, v and Nml
#endif
#ifdef _WIN64
		// l = 1 + floor(log2(d))
		unsigned long lm1;
		_BitScanReverse64(&lm1, d);
		assert(lm1 < 64ul);
		unsigned long Nml{64 - 1 - lm1};
		m_Nml = static_cast<uint8_t>(Nml);
		//unsigned long l{lm1 + 1}; this will cause failed shifts for the case of l = 64
		uint64_t d_norm{d << Nml};
		m_d_norm = d_norm;
		//uint64_t mprime{static_cast<uint64_t>(((((1ui128 << l) - static_cast<uint128_t>(d)) << 64) - 1ui128) / static_cast<uint128_t>(d))};
		// create the normalized reciprocal divisor mprime
		uint32_t d_9{static_cast<uint32_t>(d_norm >> 55)};
		uint64_t d_40{d_norm >> 24};
		//uint32_t v_0{((1ui32 << 19) - 3ui32 * (1ui32 << 8)) / d_9}; use a table lookup for this:
		// the input is a 9-bit normalized divisor, so there are 256 table entries (the high bit is always set, so there are not 512 table entries)
		uint32_t bitoffset{d_9 * 11ui32};
		uint64_t d_0{d_norm & 1ui64};
		uint64_t d_63{d_norm >> 1};
		uint32_t twobyteoffset{bitoffset >> 4};// bitoffset / 16, using bitoffset / 8 would issue the requirement of 2 bytes of padding at the end of the array because of the 4-byte dereferencing
		uint32_t residualbitoffset{bitoffset & 15ui32};// formally (bitoffset - (1ui32 << (9 - 1)) * 11ui32) & 15ui32, but this can be simplified as the low bits of (1ui32 << (9 - 1)) * 11ui32 are all zero
		uint32_t v_0{*reinterpret_cast<uint32_t UNALIGNED const*>(reinterpret_cast<uint16_t const*>(gk_packedlookuptablev64) - (1 << (9 - 1)) * 11 / 16 + static_cast<size_t>(twobyteoffset))};// table lookup with a compile-time base offset, note: (1 << (9 - 1)) * 11 / 16 only shifts out zeroes at / 16
		++d_40;// unconditional increment
		v_0 >>= residualbitoffset;
		d_63 += d_0;// rounded up
		v_0 &= 0x7FFui32;
		uint64_t d_0mask{static_cast<uint64_t>(-static_cast<int64_t>(d_0))};// this will either clear or set all bits in the mask
		uint64_t v_1{static_cast<uint64_t>((v_0 << 11) - 1ui32) - (static_cast<uint64_t>(v_0 * v_0) * d_40 >> 40)};// v_0 * v_0 is allowed because the maximum product does not require a representation of more than 32 bits: 0x7FDui32 * 0x7FDui32 = 0x3FD009ui32
		uint64_t v_2{(v_1 << 13) + (((1ui64 << 60) - v_1 * d_40) * v_1 >> 47)};
		uint64_t e{(v_2 >> 1 & d_0mask) - v_2 * d_63};
		uint64_t v_3{(v_2 << 31) + (__umulh(v_2, e) >> 1)};
		uint64_t v_4ihi, v_4ilo{_umul128(v_3, d_norm, &v_4ihi)};
		_addcarry_u64(_addcarry_u64(0, v_4ilo, d_norm, nullptr), v_4ihi, d_norm, &v_4ihi);// ignore the resulting carry out, assembly checked: the compiler allows nullptr on this parameter without faulting at compile- or run-time
		uint64_t v_4{v_3 - v_4ihi};
		m_mprime = v_4;
#else
		// l = 1 + floor(log2(d))
		//_BitScanReverse64(&lm1, d);
		//unsigned long Nml{64 - 1 - lm1};
		//unsigned long l{lm1 + 1}; this will cause failed shifts for the case of l = 64
		//uint64_t d_norm{d << Nml};
		unsigned long lm1, Nml;
		uint32_t d_normlo, d_normhi;
		if(_BitScanReverse(&lm1, static_cast<uint32_t>(d >> 32))){
			assert(lm1 < 32ul);
			Nml = 64 - 1 - 32 - lm1;
			// __ll_lshift(), __ull_rshift() and __ll_rshift() on x86-32 cannot shift more than 31 bits, exactly the same way as the underlying shld (top left), shl/sal (bottom left), shrd (bottom right), shr (top right, unsigned only), sar (top right, signed only) instructions work, so do not replace these with the generic <<=, <<, >>= or >> operands for the next statements, as these use heuristics for multi-instruction variable amount shifts of up to 64 bits
			uint64_t d_norm{__ll_lshift(d, static_cast<int>(Nml))};
			d_normlo = static_cast<uint32_t>(d_norm), d_normhi = static_cast<uint32_t>(d_norm >> 32);
		}else{
			uint32_t dlo{static_cast<uint32_t>(d)};
			_BitScanReverse(&lm1, dlo);
			assert(lm1 < 32ul);
			Nml = 64 - 1 - lm1;
			d_normlo = 0ui32, d_normhi = dlo << Nml;
		}
		m_Nml = static_cast<uint8_t>(Nml);
		m_d_normlo = d_normlo, m_d_normhi = d_normhi;
		// create the normalized reciprocal divisor v
		uint32_t d_10{d_normhi >> 22};
		uint32_t d_21{d_normhi >> 11};
		//uint32_t v_0{((1ui32 << 24) - (1ui32 << 14) + (1ui32 << 9)) / d_10}; use a table lookup for this:
		// the input is a 10-bit normalized divisor, so there are 512 table entries (the high bit is always set, so there are not 1024 table entries)
		uint32_t bitoffset{d_10 << 4};// bitoffset = d_10 * 15ui32
		uint32_t d_0{d_normhi & 1ui32};
		bitoffset -= d_10;
		uint32_t d_31{d_normhi >> 1};
		uint32_t twobyteoffset{bitoffset >> 4};// bitoffset / 16, using bitoffset / 8 would issue the requirement of 2 bytes of padding at the end of the array because of the 4-byte dereferencing
		uint32_t residualbitoffset{bitoffset & 15ui32};// formally (bitoffset - (1ui32 << (10 - 1)) * 15ui32) & 15ui32, but this can be simplified as the low bits of (1ui32 << (10 - 1)) * 15ui32 are all zero
		uint32_t v_0{*reinterpret_cast<uint32_t UNALIGNED const*>(reinterpret_cast<uint16_t const*>(gk_packedlookuptablev32) - (1 << (10 - 1)) * 15 / 16 + static_cast<size_t>(twobyteoffset))};// single-instruction table lookup with a compile-time base offset, note: (1 << (10 - 1)) * 15 / 16 only shifts out zeroes at / 16
		++d_21;// unconditional increment
		v_0 >>= residualbitoffset;
		d_31 += d_0;// rounded up
		v_0 &= 0x7FFFui32;
		uint32_t d_0mask{static_cast<uint32_t>(-static_cast<int32_t>(d_0))};// this will either clear or set all bits in the mask
		uint32_t v_1{(v_0 << 4) - 1ui32 - static_cast<uint32_t>(__emulu(v_0 * v_0, d_21) >> 32)};// v_0 * v_0 is allowed because the maximum product does not require a representation of more than 32 bits: 0x7FE1ui32 * 0x7FE1ui32 = 0x3FE103C1ui32
		uint32_t e{((d_0mask & v_1) >> 1) - v_1 * d_31};
		uint32_t v_2{(v_1 << 15) + static_cast<uint32_t>(__emulu(v_1, e) >> 33)};
		uint64_t v_3i{__emulu(v_2, d_normhi)};
		uint32_t v_3ihi{static_cast<uint32_t>(v_3i >> 32)};
		_addcarry_u32(_addcarry_u32(0, static_cast<uint32_t>(v_3i), d_normhi, nullptr), v_3ihi, d_normhi, &v_3ihi);// ignore the resulting carry out, assembly checked: the compiler allows nullptr on this parameter without faulting at compile- or run-time
		uint32_t v_3{v_2 - v_3ihi};
		uint32_t v{v_3};
		// improve v for multi-word division, note that if d_normlo is zero, this will do nothing
		uint32_t p{v * d_normhi};
		uint32_t mask0;// carry of the addition in the mask
		_subborrow_u32(_addcarry_u32(0, p, d_normhi, &p), mask0, mask0, &mask0);
		uint32_t pmasked{d_normhi & mask0};
		v += mask0;
		// mask0 && p >= d_normhighest (implemented as !(p < d_normhighest)), set the mask if true
		_addcarry_u32(_subborrow_u32(0, p, pmasked, &p), mask0, 0ui32, &mask0);// ignore the resulting carry out
		v += mask0;
		p -= pmasked & mask0;
		uint64_t t{__emulu(v, d_normlo)};// v * d_normsecondhighest
		uint32_t mask1;// carry of the addition in the mask
		_subborrow_u32(_addcarry_u32(0, p, static_cast<uint32_t>(t >> 32), &p), mask1, mask1, &mask1);// add up the result
		v += mask1;
		// mask1 && (p > d_normhighest || (p == d_normhighest && t0 >= d_normsecondhighest) (implemented as !(p < d_normhighest || (p == d_normhighest && t0 < d_normsecondhighest)))), decrement v by one if true
		_addcarry_u32(_subborrow_u32(_subborrow_u32(0, static_cast<uint32_t>(t), d_normlo & mask1, nullptr), p, d_normhi & mask1, nullptr), v, mask1, &v);// ignore the resulting carry out, assembly checked: the compiler allows nullptr on this parameter without faulting at compile- or run-time
		m_v = v;
#endif
	}
};

extern __declspec(selectany) QPFDivisorConstants const gk_QPFDivisorFactors;// This auto self-constructs at runtime during global initialization, without any external dependencies in software.

// This function converts the elapsed time values obtained fom QueryPerformanceCounter() to 100 ns units.
extern __declspec(noalias safebuffers noinline) uint64_t ConvertPerfCounterTo100ns(uint64_t timerunits);

// This function returns the elapsed time since system boot in 100 ns units.
extern __declspec(noalias safebuffers) __forceinline uint64_t PerfCounter100ns(){
	LARGE_INTEGER pc;
	BOOL bos{QueryPerformanceCounter(&pc)};
	static_cast<void>(bos);
	assert(bos);// guaranteed since Windows XP
	return{ConvertPerfCounterTo100ns(static_cast<uint64_t>(pc.QuadPart))};
}

// re-enable warning messages for files including this header
#pragma warning(pop)