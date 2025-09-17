// MIT License
// Copyright (c) 2025 Jan-Willem Krans (janwillem32 <at> hotmail <dot> com)
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// WindowsProject1.cpp : Defines the entry point for the application.

#include "pch.h"
#include "..\..\Radixsortbidi8.hpp"
#include <Aclapi.h>
#include <cmath>
#include <algorithm>// just for std::sort() (wich strictly doesn't do the same) and std::stable_sort()
#include <bit>

// disable warning messages for this file only:
// C4559: 'x': redefinition; the function gains __declspec(noalias)
// C4711: function 'x' selected for automatic inline expansion
#pragma warning(disable: 4559 4711)

#ifdef _WIN64
__declspec(noalias safebuffers) wchar_t *WritePaddedu64(wchar_t *pwcOut, uint64_t n)noexcept{
	// 18446744073709551615 is the maximum output by this function, the output is padded on the left with spaces if required to get to 20 characters
	static uint64_t constexpr fourspaces{static_cast<uint64_t>(L' ') << 48 | static_cast<uint64_t>(L' ') << 32 | static_cast<uint64_t>(L' ') << 16 | static_cast<uint64_t>(L' ')};
	reinterpret_cast<uint64_t UNALIGNED *>(pwcOut)[0] = fourspaces;
	reinterpret_cast<uint64_t UNALIGNED *>(pwcOut)[1] = fourspaces;
	reinterpret_cast<uint64_t UNALIGNED *>(pwcOut)[2] = fourspaces;
	reinterpret_cast<uint64_t UNALIGNED *>(pwcOut)[3] = fourspaces;
	reinterpret_cast<uint64_t UNALIGNED *>(pwcOut)[4] = fourspaces;// writes one wchar_t more than required, but it gets overwritten by the first output in the loop anyway
	pwcOut += 19;// make it point at the least significand digit first
	// original code:
	//do{
	//*pwcOut-- = static_cast<wchar_t>(static_cast<uint32_t>(n % 10ui64) + L'0'); the last digit is never substituted with a space
	//n /= 10ui64;
	//}while(n);
	// assembly checked: the x64 compiler eliminates the need for the div instruction here, the x86-32 compiler isn't able to optimize this routine.
	// 64-bit builds do use the 64-bit unsigned division by invariant integers using multiplication method, but the compiler is only capable of selecting the most basic variant of its methods, and not the simplified versions.
	// 32-bit builds use the 64-bit division support library (__aulldvrm(), __aulldiv(), __alldvrm() and __alldiv()) leaving the modulo and division operations completely unoptimized.
#ifdef _WIN64
	if(n > 9ui64){
		uint64_t q;
		do{
			// 64-bit unsigned division by invariant integers using multiplication
			// n / 10
			static uint64_t constexpr mprime{0xCCCCCCCCCCCCCCCDui64}, d{0x000000000000000Aui64};
			static uint8_t constexpr sh_post{3ui8};
			q = __umulh(n, mprime) >> sh_post;
			// n % 10
			// there is no trivially computable remainder for this method
			// given that we actually only need the bottom 4 bits here (the remainder is 4 bits: ceil(log2(divisor - 1 + 1))), a little optimization is allowed
			//uint32_t r{static_cast<uint32_t>(n) - static_cast<uint32_t>(q) * static_cast<uint32_t>(d)};
			//*pwcOut-- = static_cast<wchar_t>(r + L'0');
			uint32_t rm{static_cast<uint32_t>(q) * 4ui32 + static_cast<uint32_t>(q)};// lea, q * 5
			rm = rm * 2ui32 - L'0';// lea, q * 10 - L'0'
			*pwcOut-- = static_cast<wchar_t>(static_cast<uint32_t>(n) - rm);// sub, n - (q * 10 - L'0')
			n = q;
		}while(q > 9ui64);
	}
	*pwcOut = static_cast<wchar_t>(static_cast<uint32_t>(n) + L'0');// the last digit is never substituted with a space
#else
	uint32_t nlo{static_cast<uint32_t>(n)}, nhi{static_cast<uint32_t>(n >> 32)};
	if(nhi){// at most ten iterations of division by ten are needed to reduce the size of the dividend from a 64-bit unsigned integer to a 32-bit unsigned integer
		uint32_t qhi;
		do{
			// 64-bit unsigned division by invariant integers using multiplication
			// n / 10
			static uint8_t constexpr sh_post{3ui8};
			static uint64_t constexpr mprime{0xCCCCCCCCCCCCCCCDui64}, d{0x000000000000000Aui64};
			//uint64_t q{__umulh(n, mprime) >> sh_post};
			// given that the top and bottom half of mprime only differ by one, a little optimization is allowed
			uint32_t constexpr mprimehi{static_cast<uint32_t>(mprime >> 32)};
			static_assert(static_cast<uint32_t>(mprime & 0x00000000FFFFFFFFui64) == mprimehi + 1ui32);
			// unsigned 64-by-64-to-128-bit multiply
			uint64_t j2{__emulu(nlo, mprimehi)};
			uint32_t n1a{static_cast<uint32_t>(j2)}, n2{static_cast<uint32_t>(j2 >> 32)};
			// calculate __emulu(nlo, mprimehi + 1ui32) and __emulu(nhi, mprimehi + 1ui32) as __emulu(nlo, mprimehi) + nlo and __emulu(nhi, mprimehi) + nhi
			// sum table, from least to most significant word
			// nlo nhi
			// n1a n2
			//     n1a n2
			//     n2a n3
			//         n2a n3
			assert(n2 < 0xFFFFFFFFui32);// even __emulu(0xFFFFFFFFui32, 0xFFFFFFFFui32) will only output 0xFFFFFFFE00000001ui64, so adding the carry to just n2 is not a problem here
			uint32_t n0, n1;
			unsigned char EmptyCarry0{_addcarry_u32(_addcarry_u32(_addcarry_u32(0, nlo, n1a, &n0), nhi, n2, &n1), n2, 0ui32, &n2)};// add nlo and nhi to the earlier computed parts
			assert(!EmptyCarry0);// the last add-with-carry cannot carry out
			static_cast<void>(EmptyCarry0);
			static_cast<void>(n0);// the lowest part is discarded, as it cannot carry out when it has no further addend
			uint64_t j3{__emulu(nhi, mprimehi)};
			uint32_t n2a{static_cast<uint32_t>(j3)}, n3{static_cast<uint32_t>(j3 >> 32)};
			unsigned char EmptyCarry1{_addcarry_u32(_addcarry_u32(_addcarry_u32(0, n1, n1a, &n1), n2, n3, &n2), n3, 0ui32, &n3)};
			assert(!EmptyCarry1);// the last add-with-carry cannot carry out
			static_cast<void>(EmptyCarry1);
			unsigned char EmptyCarry2{_addcarry_u32(_addcarry_u32(_addcarry_u32(0, n1, n2a, &n1), n2, n2a, &n2), n3, 0ui32, &n3)};
			assert(!EmptyCarry2);// the last add-with-carry cannot carry out
			static_cast<void>(EmptyCarry2);
			// n0 and n1 are discarded
			uint32_t at_1[2]{n2, n3}; uint64_t t_1{*reinterpret_cast<uint64_t UNALIGNED *>(at_1)};// recompose
			uint64_t q{t_1 >> sh_post};
			uint32_t qlo{static_cast<uint32_t>(q)};
			qhi = static_cast<uint32_t>(q >> 32);
			// n % 10
			// there is no trivially computable remainder for this method
			// given that we actually only need the bottom 4 bits here (the remainder is 4 bits: ceil(log2(divisor - 1 + 1))), a little optimization is allowed
			//uint32_t r{nlo - qlo * static_cast<uint32_t>(d)};
			//*pwcOut-- = static_cast<wchar_t>(r + L'0');
			uint32_t rm{qlo * 4ui32 + qlo};// lea, q * 5
			nhi = qhi;
			rm = rm * 2ui32 - L'0';// lea, q * 10 - L'0'
			*pwcOut-- = static_cast<wchar_t>(nlo - rm);// sub, n - (q * 10 - L'0')
			nlo = qlo;
		}while(qhi);
		assert(nlo > 9ui32);
		goto HandleLowHalf;// skip the first check, as nlo will guaranteed be larger than 9
	}
	if(nlo > 9ui32){// use a simpler loop when the high half of the dividend is zero
HandleLowHalf:
		uint32_t q;
		do{
			// 32-bit unsigned division by invariant integers using multiplication
			// n / 10
			static uint8_t constexpr sh_post{3ui8};
			static uint32_t constexpr mprime{0xCCCCCCCDui32}, d{0x0000000Aui32};
			q = static_cast<uint32_t>(__emulu(nlo, mprime) >> 32) >> sh_post;
			// n % 10
			// there is no trivially computable remainder for this method
			//uint32_t r{nlo - q * d};
			//*pwcOut-- = static_cast<wchar_t>(r + L'0');
			uint32_t rm{q * 4ui32 + q};// lea, q * 5
			rm = rm * 2ui32 - L'0';// lea, q * 10 - L'0'
			*pwcOut-- = static_cast<wchar_t>(nlo - rm);// sub, n - (q * 10 - L'0')
			nlo = q;
		}while(q > 9ui32);
	}
	*pwcOut = static_cast<wchar_t>(nlo + L'0');// the last digit is never substituted with a space
#endif
	return{pwcOut};
}
#else
extern "C" __declspec(noalias safebuffers) wchar_t *__vectorcall WritePaddedu64ra(wchar_t *pwcOut, uint32_t nhi, __m128i nlo)noexcept;// .asm file
__declspec(noalias safebuffers) __forceinline wchar_t *WritePaddedu64(wchar_t *pwcOut, uint64_t n)noexcept{return{WritePaddedu64ra(pwcOut, static_cast<uint32_t>(n >> 32), _mm_cvtsi32_si128(static_cast<int32_t>(n)))};}
#endif

// Radix sort, without the extra buffer
__declspec(noalias safebuffers) void radixsortbufferless(uint16_t arr[], size_t count)noexcept{
	assert(arr);// do not pass a nullptr here, even though it's safe if count is 0
	if(2 < count){// a 0 or 1 count array is legal here
		uint16_t bitmask{1};// least significant bit first, will get shifted up to the top before being shifted out
		uint16_t *arrend{arr - 1 + count};// points to the last member, not past the end of the array
		do{// main loop
			uint16_t *arr1{arr}, *arr0;// pointers to the first found 1 and the last found 0
			// arr1 is set at the default starting point for stage 1
			if(!(*arr & bitmask)){// stage 0: found a 0, scan for a 1
				++arr1;// the default starting point for stage 0
				for(;;){// scan for a 1
					if(*arr1 & bitmask) break;// found a 1
					if(++arr1 == arrend) goto alreadysorted;// scan up to the last member (which doesn't matter if it's a 0 or 1 at this point as all previous items are 0)
				}
			}
			// stage 1: found a 1, scan for a 0
			arr0 = arr1;// the default starting point for stage 1
			do{
				++arr0;
				uint16_t cur;
				for(;;){// scan for a 0
					cur = *arr0;
					if(!(cur & bitmask)) break;// found a 0
					if(arr0 == arrend) goto alreadysorted;// scan including the last member
					++arr0;
				}
				// stage 2: move the found 0 item down
				// cur will contain a value with the current bit set to 0 here, pointed by arr0
				// arr0 will point to the first 0 above a 1 here
				// arr1 will point to the first of the sequence of 1s below that 0 here
				// swap 1 place up in the array, insert cur at 0
				ptrdiff_t i{arr0 - arr1};// requires a right shift if not single-byte, but pointer subtraction is still very efficient
				do{// note: the __movsw() intrinsic doesn't do downwards direction copies unlike x64 asm - rep movsw, so use a loop here
					arr0[i] = arr0[i - 1];
				}while(--i);
				*arr1++ = cur;// point at the first 1 again
			}while(arr0 != arrend);// this part is sorted if arr0copy is pointing at the last member
alreadysorted:
			;// compiler issue, it doesn't like to get a "}" directly after a label
		}while(bitmask <<= 1);// will left shift out at some point
	}else if(2 == count){// rare, but still needs to be processed
		uint16_t lo{arr[0]}, hi{arr[1]};
		if(hi < lo) arr[0] = hi, arr[1] = lo;// one swap if required
	}
}

// Radix sort, without the extra buffer
__declspec(noalias safebuffers) void radixsortbufferlessdown(uint16_t arr[], size_t count)noexcept{
	assert(arr);// do not pass a nullptr here, even though it's safe if count is 0
	if(2 < count){// a 0 or 1 count array is legal here
		uint16_t bitmask{1};// least significant bit first, will get shifted up to the top before being shifted out
		uint16_t *arrend{arr - 1 + count};// points to the last member, not past the end of the array
		do{// main loop
			uint16_t *arr0{arrend}, *arr1;// pointers to the first found 0 and the last found 1
			// arr0 is set at the default starting point for stage 1
			if(*arr & bitmask){// stage 0: found a 1, scan for a 0
				--arr0;// the default starting point for stage 0
				for(;;){// scan for a 0
					if(!(*arr0 & bitmask)) break;// found a 0
					if(--arr0 == arr) goto alreadysorted;// scan up to the first member (which doesn't matter if it's a 0 or 1 at this point as all previous items are 1)
				}
			}
			// stage 1: found a 0, scan for a 1
			arr1 = arr0;// the default starting point for stage 1
			do{
				--arr1;
				uint16_t cur;
				for(;;){// scan for a 1
					cur = *arr1;
					if(cur & bitmask) break;// found a 1
					if(arr1 == arr) goto alreadysorted;// scan including the first member
					--arr1;
				}
				// stage 2: move the found 1 item up
				// cur will contain a value with the current bit set to 1 here, pointed by arr1
				// arr1 will point to the first 1 below a 0 here
				// arr0 will point to the first of the sequence of 0s above that 1 here
				// swap 1 place down in the array, insert cur at 0
				SIZE_T i{static_cast<SIZE_T>(arr0 - arr1)};// requires a right shift if not single-byte, but pointer subtraction is still very efficient
				__movsw(arr1, arr1 + 1, i);// note: unlike x64 asm - rep movsw, this can only direct forward
				--arr0;// point at the first 0 again
				*arr1 = cur;
			}while(arr1 != arr);// this part is sorted if arr1 is pointing at the first member
alreadysorted:
			;// compiler issue, it doesn't like to get a "}" directly after a label
		}while(bitmask <<= 1);// will left shift out at some point
	}else if(2 == count){// rare, but still needs to be processed
		uint16_t lo{arr[0]}, hi{arr[1]};
		if(hi < lo) arr[0] = hi, arr[1] = lo;// one swap if required
	}
}

// Radix sort, two-bit indexing
__declspec(noalias safebuffers) bool radixsort2(uint16_t arr[], size_t count, size_t largepagesize)noexcept{
	assert(arr);// do not pass a nullptr here, even though it's safe if count is 0
	if(2 < count){// a 0 or 1 count array is legal here
		// create the swappable memory here
		assert(largepagesize && !(largepagesize - 1 & largepagesize));// only one bit should be set in the value of largepagesize
		size_t allocsize{(largepagesize - 1 & -static_cast<ptrdiff_t>(count * 2)) + count * 2};// round up to the nearest multiple of largepagesize
		uint16_t *buffer{reinterpret_cast<uint16_t *>(VirtualAlloc(nullptr, allocsize, MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))};
		if(!buffer) return{false};

		size_t offsets[4][8]{};
		bool paritybool;// for when the swap count is odd
		{// count the 4 configurations, all in one go
			ptrdiff_t i{static_cast<ptrdiff_t>(count - 1)};
			do{// 00, 01, 10, 11 counted in the 4 parts
				uint32_t cur{arr[i]};
				buffer[i] = static_cast<uint16_t>(cur);// better to handle it from the start than to do an extra copy at the end, for when the swap count is odd
				static_assert(sizeof(void *) == 8);// 64-bit-only, as size_t is assumed to be 64-bit here
				++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(*offsets) + (cur << 6 & 0xC0));
				++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(*offsets + 1) + (cur << 4 & 0xC0));
				++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(*offsets + 2) + (cur << 2 & 0xC0));
				++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(*offsets + 3) + (cur & 0xC0));
				++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(*offsets + 4) + (cur >> 2 & 0xC0));
				++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(*offsets + 5) + (cur >> 4 & 0xC0));
				++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(*offsets + 6) + (cur >> 6 & 0xC0));
				++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(*offsets + 7) + (cur >> 8 & 0xC0));
			}while(0 <= --i);
			// transform counts into base offsets for each set of 4 items
			uint32_t t{};// t facilitates traversing flat trough the arrays of offsets
			paritybool = false;// for when the swap count is odd
			do{
				size_t count0{offsets[0][t]};// not overwritten here
				size_t count1{offsets[1][t]};
				size_t count2{offsets[2][t]};
				size_t counter{count0 + count1};
				bool b{count0 == count};// if one entry of the array equals count and the rest equals zero this check will indicate it
				offsets[1][t] = counter;
				b |= count1 == count;
				counter += count2;
				b |= count2 == count;
				offsets[2][t] = counter;
				b |= offsets[3][t] == count;
				paritybool ^= b;
				offsets[3][t] = b;// use this spot, it's just wasted space otherwise
			}while(8 > ++t);
		}
		// perform the 2-bit sorting sequence
		// this sequence doesn't use arr and buffer after initialization
		uint16_t *psrc{arr}, *pdst00{buffer};
		if(paritybool){// swap if the count of sorting actions to do is odd
			pdst00 = arr;
			psrc = buffer;
		}
		size_t initskip{offsets[3][0]};
		uint32_t bitselect{};// least significant bit first
		while(initskip){// skip a step if possible
			bitselect += 2;
			if(16 <= bitselect) goto exit;
			initskip = *reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets[3]) + bitselect * (sizeof(void *) >> 1));
		}
		{
			uint16_t *psrcnext{pdst00}, *pdstnext{psrc};// for the next iteration
			uint16_t *pdst01{pdst00 + *reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets[0]) + bitselect * (sizeof(void *) >> 1))};
			uint16_t *pdst10{pdst00 + *reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets[1]) + bitselect * (sizeof(void *) >> 1))};
			uint16_t *pdst11{pdst00 + *reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets[2]) + bitselect * (sizeof(void *) >> 1))};
			size_t i{count};
			for(;;){// main loop
				do{// fill the array
					uint32_t cur{*psrc++};
					uint16_t *pdstl{pdst00};
					uint32_t selh{cur >> bitselect};
					uint16_t *pdsth{pdst01};
					uint32_t sell{selh};
					if(selh &= 2){
						pdstl = pdst10;
						pdsth = pdst11;
					}
					pdstl = (sell &= 1)? pdsth : pdstl;
					sell <<= 1;// 0 or 2
					*pdstl = static_cast<uint16_t>(cur);
					// get the 4 unique truths by a minimum of x64 operations, and minimizing register usage
					uint32_t sel01{sell + selh};// optimization on some platforms: using addition instead of xor (will be masked next anyway)
					uint32_t sel10{sel01};
					sel01 &= sell;
					sell ^= 2;// flip the only bit
					uint32_t sel00{sell + selh};// optimization on some platforms: using addition instead of xor (will be masked next anyway)
					sel10 &= selh;
					selh &= sel00;// sel11
					sel00 &= sell;
					assert(2 == sel01 + sel10 + selh + sel00);// this will displace only one pointer
					pdst01 = reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(pdst01) + sel01);
					pdst10 = reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(pdst10) + sel10);
					pdst11 = reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(pdst11) + selh);
					pdst00 = reinterpret_cast<uint16_t *>(reinterpret_cast<uint8_t *>(pdst00) + sel00);
				}while(--i);
				size_t curskip;// skip a step if possible
				do{
					bitselect += 2;
					if(16 <= bitselect) goto exit;
					curskip = *reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets[3]) + bitselect * (sizeof(void *) >> 1));
				}while(curskip);
				// swap the pointers for the next round, data is moved on each iteration
				pdst00 = pdstnext;
				pdst01 = pdstnext + *reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets[0]) + bitselect * (sizeof(void *) >> 1));
				pdst10 = pdstnext + *reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets[1]) + bitselect * (sizeof(void *) >> 1));
				pdst11 = pdstnext + *reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets[2]) + bitselect * (sizeof(void *) >> 1));
				psrc = psrcnext;
				pdstnext = psrcnext;
				i = count;
				psrcnext = pdst00;
			}
		}
exit:
		BOOL boVirtualFree{VirtualFree(buffer, 0, MEM_RELEASE)};
		static_cast<void>(boVirtualFree);
		assert(boVirtualFree);
	}else if(2 == count){// rare, but still needs to be processed
		uint16_t lo{arr[0]}, hi{arr[1]};
		if(hi < lo) arr[0] = hi, arr[1] = lo;// one swap if required
	}
	return{true};
}

// Radix sort, bidirectional eight-bit indexing
__declspec(noalias safebuffers) bool radixsortbidi8(uint16_t arr[], size_t count, size_t largepagesize)noexcept{
	assert(arr);// do not pass a nullptr here, even though it's safe if count is 0
	if(2 < count){// a 0 or 1 count array is legal here
		// create the swappable memory here
		assert(largepagesize && !(largepagesize - 1 & largepagesize));// only one bit should be set in the value of largepagesize
		size_t allocsize{(largepagesize - 1 & -static_cast<ptrdiff_t>(count * 2)) + count * 2};// round up to the nearest multiple of largepagesize
		uint16_t *buffer{reinterpret_cast<uint16_t *>(VirtualAlloc(nullptr, allocsize, MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))};
		if(!buffer) return{false};

		size_t offsets[4 * 256];// 8 kibibytes of indices on a 64-bit system, but it's worth it
		memset(offsets, 0, sizeof(offsets) / 2);// only the low half
		// count the 256 configurations, all in one go
		ptrdiff_t i{static_cast<ptrdiff_t>(count)};
		while(0 <= (i -= 4)){// the lower half of offsets is zeroed out for the subsequent increments here
			uint32_t cura{arr[i + 3]};
			uint32_t curb{arr[i + 2]};
			uint32_t curc{arr[i + 1]};
			uint32_t curd{arr[i]};
			size_t cur0a{cura & 0xFF};
			size_t cur0b{curb & 0xFF};
			size_t cur0c{curc & 0xFF};
			size_t cur0d{curd & 0xFF};
			buffer[i + 2] = static_cast<uint16_t>(cura);// better to handle it from the start than to do an extra copy at the end, for when the swap count is odd
			buffer[i + 3] = static_cast<uint16_t>(curb);
			buffer[i + 1] = static_cast<uint16_t>(curc);
			buffer[i] = static_cast<uint16_t>(curd);
			cura >>= 8;
			curb >>= 8;
			curc >>= 8;
			curd >>= 8;
			++offsets[cur0a];
			++offsets[cur0b];
			++offsets[cur0c];
			++offsets[cur0d];
			++offsets[256 + static_cast<size_t>(cura)];
			++offsets[256 + static_cast<size_t>(curb)];
			++offsets[256 + static_cast<size_t>(curc)];
			++offsets[256 + static_cast<size_t>(curd)];
		}
		if(count & 2){// possibly finalize 2 entries after the above loop
			uint32_t cura{arr[i + 3]};
			uint32_t curb{arr[i + 2]};
			size_t cur0a{cura & 0xFF};
			size_t cur0b{curb & 0xFF};
			buffer[i + 2] = static_cast<uint16_t>(cura);// better to handle it from the start than to do an extra copy at the end, for when the swap count is odd
			buffer[i + 3] = static_cast<uint16_t>(curb);
			cura >>= 8;
			curb >>= 8;
			++offsets[cur0a];
			++offsets[cur0b];
			++offsets[256 + static_cast<size_t>(cura)];
			++offsets[256 + static_cast<size_t>(curb)];
		}
		if(count & 1){// possibly finalize 1 entry after the above loop
			uint32_t cur{arr[0]};
			size_t cur0{cur & 0xFF};
			buffer[0] = static_cast<uint16_t>(cur);// better to handle it from the start than to do an extra copy at the end, for when the swap count is odd
			cur >>= 8;
			++offsets[cur0];
			++offsets[256 + static_cast<size_t>(cur)];
		}
		// transform counts into base offsets for each set of 256 items
		size_t *t{offsets};
		size_t countm1{count - 1};
		bool paritybool{};// for when the swap count is odd
		uint8_t runsteps{(1 << sizeof(uint16_t)) - 1};// 0 to 2
		uint32_t k{};
		do{
			size_t offset{*t};
			*t++ = 0;// the first offset always starts at zero
			bool b{offset == count};
			uint32_t j{256 - 2};
			do{
				size_t addend{*t};
				*t = offset;
				t[2 * 256 - 1] = offset - 1;// high half
				offset += addend;
				b |= addend == count;
				++t;
			}while(--j);
			b |= *t == count;
			t[2 * 256] = countm1;// high half, the last offset always starts at the end
			*t = offset;
			t[2 * 256 - 1] = offset - 1;// high half
			++t;
			paritybool ^= b;
			runsteps ^= static_cast<uint8_t>(b) << k;
		}while(2 > ++k);
		// perform the bidirectional 8-bit sorting sequence
		if(runsteps){
			uint16_t *psrclo{arr}, *pdst{buffer};
			if(paritybool){// swap if the count of sorting actions to do is odd
				pdst = arr;
				psrclo = buffer;
			}
			size_t *poffset{offsets};
			uint32_t bitselect{};// least significant bit first
			if(!(runsteps & 1)){// skip a step if possible
				runsteps >>= 1;// at least 1 bit is set inside runsteps as by previous check
				bitselect = 8;
				poffset += 256;
			}
			uint16_t *psrchi{psrclo + countm1};
			uint16_t *pdstnext{psrclo};// for the next iteration
			if(countm1 & 1) for(;;){// main loop for even counts
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					uint32_t curlo{*psrclo++};
					uint32_t curhi{*psrchi--};
					size_t sello{curlo >> bitselect & 0xFF};
					size_t selhi{curhi >> bitselect & 0xFF};
					size_t offsetlo{poffset[sello]++};// the next item will be placed one higher
					size_t offsethi{poffset[2 * 256 + selhi]--};// the next item will be placed one lower
					pdst[offsetlo] = static_cast<uint16_t>(curlo);
					pdst[offsethi] = static_cast<uint16_t>(curhi);
				}while(psrclo < psrchi);
				runsteps >>= 1;
				if(!runsteps) break;
				bitselect = 8;
				poffset += 256;
				// swap the pointers for the next round, data is moved on each iteration
				psrclo = pdst;
				psrchi = pdst + countm1;
				pdst = pdstnext;
				// pdstnext = psrclo; only 2 iterations, so pdstnext is unused after this point
			}
			else for(;;){// main loop for odd counts
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					uint32_t curlo{*psrclo++};
					uint32_t curhi{*psrchi--};
					size_t sello{curlo >> bitselect & 0xFF};
					size_t selhi{curhi >> bitselect & 0xFF};
					size_t offsetlo{poffset[sello]++};// the next item will be placed one higher
					size_t offsethi{poffset[2 * 256 + selhi]--};// the next item will be placed one lower
					pdst[offsetlo] = static_cast<uint16_t>(curlo);
					pdst[offsethi] = static_cast<uint16_t>(curhi);
				}while(psrclo < psrchi);
				// fill in the final item
				uint32_t curlo{*psrclo};
				size_t sello{curlo >> bitselect & 0xFF};
				size_t offsetlo{poffset[sello]};
				pdst[offsetlo] = static_cast<uint16_t>(curlo);
				runsteps >>= 1;
				if(!runsteps) break;
				bitselect = 8;
				poffset += 256;
				// swap the pointers for the next round, data is moved on each iteration
				psrclo = pdst;
				psrchi = pdst + countm1;
				pdst = pdstnext;
				// pdstnext = psrclo; only 2 iterations, so pdstnext is unused after this point
			}
		}

		BOOL boVirtualFree{VirtualFree(buffer, 0, MEM_RELEASE)};
		static_cast<void>(boVirtualFree);
		assert(boVirtualFree);
	}else if(2 == count){// rare, but still needs to be processed
		uint16_t lo{arr[0]}, hi{arr[1]};
		if(hi < lo) arr[0] = hi, arr[1] = lo;// one swap if required
	}
	return{true};
}

// Radix sort, unrolled bidirectional eight-bit indexing
__declspec(noalias safebuffers) bool radixsortbidi8unroll(uint16_t arr[], size_t count, size_t largepagesize)noexcept{
	assert(arr);// do not pass a nullptr here, even though it's safe if count is 0
	if(2 < count){// a 0 or 1 count array is legal here
		// create the swappable memory here
		assert(largepagesize && !(largepagesize - 1 & largepagesize));// only one bit should be set in the value of largepagesize
		size_t allocsize{(largepagesize - 1 & -static_cast<ptrdiff_t>(count * 2)) + count * 2};// round up to the nearest multiple of largepagesize
		uint16_t *buffer{reinterpret_cast<uint16_t *>(VirtualAlloc(nullptr, allocsize, MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))};
		if(!buffer) return{false};

		size_t offsets[4 * 256];// 8 kibibytes of indices on a 64-bit system, but it's worth it
		memset(offsets, 0, sizeof(offsets) / 2);// only the low half
		size_t constexpr log2ptrs{static_cast<unsigned int>(std::bit_width(sizeof(void *) - 1))};
		// count the 256 configurations, all in one go
		ptrdiff_t i{static_cast<ptrdiff_t>(count)};
		while(0 <= (i -= 4)){// the lower half of offsets is zeroed out for the subsequent increments here
			uint32_t cura{arr[i + 3]};
			uint32_t curb{arr[i + 2]};
			uint32_t curc{arr[i + 1]};
			uint32_t curd{arr[i]};
			size_t cur0a{cura & 0xFF};
			size_t cur0b{curb & 0xFF};
			size_t cur0c{curc & 0xFF};
			size_t cur0d{curd & 0xFF};
			buffer[i + 2] = static_cast<uint16_t>(cura);// better to handle it from the start than to do an extra copy at the end, for when the swap count is odd
			buffer[i + 3] = static_cast<uint16_t>(curb);
			buffer[i + 1] = static_cast<uint16_t>(curc);
			buffer[i] = static_cast<uint16_t>(curd);
			cura >>= 8;
			curb >>= 8;
			curc >>= 8;
			curd >>= 8;
			++offsets[cur0a];
			++offsets[cur0b];
			++offsets[cur0c];
			++offsets[cur0d];
			++offsets[256 + static_cast<size_t>(cura)];
			++offsets[256 + static_cast<size_t>(curb)];
			++offsets[256 + static_cast<size_t>(curc)];
			++offsets[256 + static_cast<size_t>(curd)];
		}
		if(count & 2){// possibly finalize 2 entries after the above loop
			uint32_t cura{arr[i + 3]};
			uint32_t curb{arr[i + 2]};
			size_t cur0a{cura & 0xFF};
			size_t cur0b{curb & 0xFF};
			buffer[i + 2] = static_cast<uint16_t>(cura);// better to handle it from the start than to do an extra copy at the end, for when the swap count is odd
			buffer[i + 3] = static_cast<uint16_t>(curb);
			cura >>= 8;
			curb >>= 8;
			++offsets[cur0a];
			++offsets[cur0b];
			++offsets[256 + static_cast<size_t>(cura)];
			++offsets[256 + static_cast<size_t>(curb)];
		}
		if(count & 1){// possibly finalize 1 entry after the above loop
			uint32_t cur{arr[0]};
			size_t cur0{cur & 0xFF};
			buffer[0] = static_cast<uint16_t>(cur);// better to handle it from the start than to do an extra copy at the end, for when the swap count is odd
			cur >>= 8;
			++offsets[cur0];
			++offsets[256 + static_cast<size_t>(cur)];
		}
		// transform counts into base offsets for each set of 256 items
		size_t *t{offsets};
		size_t countm1{count - 1};
		bool paritybool{};// for when the swap count is odd
		uint8_t runsteps{(1 << sizeof(uint16_t)) - 1};// 0 to 2
		uint32_t k{};
		do{
			size_t offset{*t};
			*t++ = 0;// the first offset always starts at zero
			bool b{offset == count};
			uint32_t j{256 - 2};
			do{
				size_t addend{*t};
				*t = offset;
				t[2 * 256 - 1] = offset - 1;// high half
				offset += addend;
				b |= addend == count;
				++t;
			}while(--j);
			b |= *t == count;
			t[2 * 256] = countm1;// high half, the last offset always starts at the end
			*t = offset;
			t[2 * 256 - 1] = offset - 1;// high half
			++t;
			paritybool ^= b;
			runsteps ^= static_cast<uint8_t>(b) << k;
		}while(2 > ++k);
		// perform the bidirectional 8-bit sorting sequence
		uint16_t *psrclo{arr}, *pdst{buffer};
		if(paritybool){// swap if the count of sorting actions to do is odd
			pdst = arr;
			psrclo = buffer;
		}
		uint16_t *psrchi{psrclo + countm1};
		uint16_t *pdstnext{psrclo};// for the next iteration
		if(countm1 & 1){// for even counts
			if(runsteps & 1){// skip if possible
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					uint32_t curlo{*psrclo++};
					uint32_t curhi{*psrchi--};
					size_t sello{curlo & 0xFF};
					size_t selhi{curhi & 0xFF};
					size_t offsetlo{offsets[sello]++};// the next item will be placed one higher
					size_t offsethi{offsets[2 * 256 + selhi]--};// the next item will be placed one lower
					pdst[offsetlo] = static_cast<uint16_t>(curlo);
					pdst[offsethi] = static_cast<uint16_t>(curhi);
				}while(psrclo < psrchi);
				// swap the pointers for the next round, data is moved on each iteration
				psrclo = pdst;
				psrchi = pdst + countm1;
				pdst = pdstnext;
				pdstnext = psrclo;
			}
			if(runsteps & 1 << 1){// skip if possible
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					uint32_t curlo{*psrclo++};
					uint32_t curhi{*psrchi--};
					size_t sello{curlo >> (8 - log2ptrs) & sizeof(void *) * 0xFF};
					size_t selhi{curhi >> (8 - log2ptrs) & sizeof(void *) * 0xFF};
					size_t offsetlo{reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets + 256) + sello)[0]++};// the next item will be placed one higher
					size_t offsethi{reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets + 3 * 256) + selhi)[0]--};// the next item will be placed one lower
					pdst[offsetlo] = static_cast<uint16_t>(curlo);
					pdst[offsethi] = static_cast<uint16_t>(curhi);
				}while(psrclo < psrchi);
				// no swapping of pointers after the last entry
			}
		}else{// for odd counts
			if(runsteps & 1){// skip if possible
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					uint32_t curlo{*psrclo++};
					uint32_t curhi{*psrchi--};
					size_t sello{curlo & 0xFF};
					size_t selhi{curhi & 0xFF};
					size_t offsetlo{offsets[sello]++};// the next item will be placed one higher
					size_t offsethi{offsets[2 * 256 + selhi]--};// the next item will be placed one lower
					pdst[offsetlo] = static_cast<uint16_t>(curlo);
					pdst[offsethi] = static_cast<uint16_t>(curhi);
				}while(psrclo < psrchi);
				// fill in the final item
				uint32_t curlo{*psrclo};
				size_t sello{curlo & 0xFF};
				size_t offsetlo{offsets[sello]};
				pdst[offsetlo] = static_cast<uint16_t>(curlo);
				// swap the pointers for the next round, data is moved on each iteration
				psrclo = pdst;
				psrchi = pdst + countm1;
				pdst = pdstnext;
				pdstnext = psrclo;
			}
			if(runsteps & 1 << 1){// skip if possible
				do{// fill the array, two at a time: one low-to-middle, one high-to-middle
					uint32_t curlo{*psrclo++};
					uint32_t curhi{*psrchi--};
					size_t sello{curlo >> (8 - log2ptrs) & sizeof(void *) * 0xFF};
					size_t selhi{curhi >> (8 - log2ptrs) & sizeof(void *) * 0xFF};
					size_t offsetlo{reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets + 256) + sello)[0]++};// the next item will be placed one higher
					size_t offsethi{reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets + 3 * 256) + selhi)[0]--};// the next item will be placed one lower
					pdst[offsetlo] = static_cast<uint16_t>(curlo);
					pdst[offsethi] = static_cast<uint16_t>(curhi);
				}while(psrclo < psrchi);
				// fill in the final item
				uint32_t curlo{*psrclo};
				size_t sello{curlo >> (8 - log2ptrs) & sizeof(void *) * 0xFF};
				size_t offsetlo{reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets + 256) + sello)[0]};
				pdst[offsetlo] = static_cast<uint16_t>(curlo);
				// no swapping of pointers after the last entry
			}
		}

		BOOL boVirtualFree{VirtualFree(buffer, 0, MEM_RELEASE)};
		static_cast<void>(boVirtualFree);
		assert(boVirtualFree);
	}else if(2 == count){// rare, but still needs to be processed
		uint16_t lo{arr[0]}, hi{arr[1]};
		if(hi < lo) arr[0] = hi, arr[1] = lo;// one swap if required
	}
	return{true};
}

// message handler for about box
__declspec(noalias safebuffers) INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)noexcept{
	static_cast<void>(lParam);
	switch(message){
		case WM_INITDIALOG:
			return{static_cast<INT_PTR>(TRUE)};
		case WM_COMMAND:
			if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL){
				BOOL bos{EndDialog(hDlg, LOWORD(wParam))};
				static_cast<void>(bos);
				assert(bos);
				return{static_cast<INT_PTR>(TRUE)};
			}
			break;
	}
	return{static_cast<INT_PTR>(FALSE)};
}

//
// FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
// PURPOSE: Processes messages for the main window.
//
// WM_COMMAND - process the application menu
// WM_PAINT - Paint the main window
// WM_DESTROY - post a quit message and return
//
//
__declspec(noalias safebuffers) LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)noexcept{
	switch(message){
	case WM_COMMAND:
		// Parse the menu selections:
		switch(LOWORD(wParam)){
		case IDM_ABOUT:
			{
				INT_PTR ips{DialogBoxParamW(reinterpret_cast<HINSTANCE>(&__ImageBase), MAKEINTRESOURCEW(IDD_ABOUTBOX), hWnd, About, 0)};
				static_cast<void>(ips);
				assert(ips);
				break;
			}
		case IDM_EXIT:
			{
				BOOL bos{DestroyWindow(hWnd)};
				static_cast<void>(bos);
				assert(bos);
				break;
			}
		default:
			return{DefWindowProcW(hWnd, message, wParam, lParam)};
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc{BeginPaint(hWnd, &ps)};
			static_cast<void>(hdc);// Add any drawing code that uses hdc here...
			BOOL bos{EndPaint(hWnd, &ps)};
			static_cast<void>(bos);
			assert(bos);
			break;
		}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return{DefWindowProcW(hWnd, message, wParam, lParam)};
	}
	return{static_cast<LRESULT>(0)};
}

#if defined(_DEBUG) && defined(_M_IX86)
// verify the initial x87 status and control word values
// the compiler's floating point environment control functions often fail to just report anything properly (#pragma STDC FENV_ACCESS and the items in <cfenv> or <float.h>)
// just issue the instructions in assembly directly
__declspec(noalias safebuffers naked) uint16_t __vectorcall Getx87StatusAndControlWords(uint16_t &ControlWord)noexcept{
	static_cast<void>(ControlWord);// ControlWord in ecx, return value in ax, disable a false compiler warning for ControlWord
	__asm{
		xor eax, eax; do not stall the pipeline with a partial register update (ax in eax)
		fnstcw [ecx]; store the x87 control word
		fnstsw ax; move the x87 status word to the return register
		ret
	}
}
#endif

// Visual C++ allows global function overrides that are executed by the initialization function before wWinMain()
#include <vcruntime_startup.h>
// unused, the project settings for file open mode control are fine how they are
//extern "C" int __CRTDECL _get_startup_file_mode();// the default version of this function only sets the file open mode to text
// unused, the project settings for floating point control are fine how they are
//extern "C" void __CRTDECL _initialize_default_precision();// the default version of this function only sets the precision to 53 bits (double) instead of the x87 default 64 bits (80-bit x86 extended precision format)
//extern "C" void __CRTDECL _initialize_denormal_control();// the default version of this function does nothing
// noenv - this disables the allocation of the environment strings once passed to wmain()
extern "C" wchar_t *__CRTDECL __dcrt_get_wide_environment_from_os(){return{nullptr};}
extern "C" char *__CRTDECL __dcrt_get_narrow_environment_from_os(){assert(false); return{nullptr};}// Unicode program, so this function should not even get linked in
extern "C" bool __CRTDECL _should_initialize_environment(){return{false};}
// noarg - this disables the allocation of the environment strings once passed to wmain() and the lpCmdLine parameter to wWinMain()
extern "C" _crt_argv_mode __CRTDECL _get_startup_argv_mode(){return{_crt_argv_no_arguments};}
extern "C" char *__CRTDECL _get_narrow_winmain_command_line(){assert(false); return{nullptr};}// Unicode program, so this function should not even get linked in
extern "C" wchar_t *__CRTDECL _get_wide_winmain_command_line(){return{nullptr};}
// There are a few more calls used in the initialization function that can be overridden. Most of these are used to help mapping the crt to the system calls. Keep a lookout for changes in vcruntime_startup.h and the files in its installation folder for these in future compiler versions.

__declspec(noalias safebuffers) int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow){
	static_cast<void>(hInstance);// There is no use for storing this, as it's equal to &__ImageBase by definition.
	assert(&__ImageBase == reinterpret_cast<IMAGE_DOS_HEADER *>(hInstance));
	static_cast<void>(hPrevInstance);// hPrevInstance has no meaning. It was used in 16-bit Windows, but is now always zero.
	assert(!hPrevInstance);
	static_cast<void>(lpCmdLine);// The overridden _get_wide_winmain_command_line() will not be returning a string, as it's never allocated.
	assert(!lpCmdLine);
#ifdef _DEBUG
	{
		// lpCmdLine isn't useful, as GetCommandLineW() and GetCommandLineA() are still available for the programs that need to parse the command line.
		// The internal initialization functions that lead to calling wWinMain() parse the output of GetCommandLineW() anyway, and then store the resulting strings by dynamically allocating memory.
		LPWSTR pszCmdLine{GetCommandLineW()};
		assert(pszCmdLine);
		// GetCommandLineW() internally just reads the process environment block:
#ifdef _WIN64
		uintptr_t pProcessEnvironmentBlock{__readgsqword(0x60)};
#else
		uintptr_t pProcessEnvironmentBlock{__readfsdword(0x30)};
#endif
		RTL_USER_PROCESS_PARAMETERS *pUPP{reinterpret_cast<PEB *>(pProcessEnvironmentBlock)->ProcessParameters};
		// these conveniently also include a length parameter (in bytes)
		assert(pUPP->ImagePathName.Buffer);
		assert(pUPP->ImagePathName.Length);
		assert(pUPP->ImagePathName.MaximumLength);
		assert(pUPP->CommandLine.Buffer);
		assert(pUPP->CommandLine.Length);
		assert(pUPP->CommandLine.MaximumLength);
		assert(pUPP->CommandLine.Buffer == pszCmdLine);// GetCommandLineW() simply outputs a pointer to the string in the UNICODE_STRING item that is always allocated for the process
#ifdef _WIN64
		UNICODE_STRING *pDesktopInfo{reinterpret_cast<UNICODE_STRING *>(reinterpret_cast<uintptr_t>(pUPP) + 0xC0)};
#else
		UNICODE_STRING *pDesktopInfo{reinterpret_cast<UNICODE_STRING *>(reinterpret_cast<uintptr_t>(pUPP) + 0x78)};
#endif
		assert(pDesktopInfo->Buffer);// can point to an empty string if simply unnamed
		//assert(pDesktopInfo->Length); empty string is legal
		//assert(pDesktopInfo->MaximumLength); empty string is legal

		// floating point environment intitialization verification
#ifdef _M_IX86
		// verify the initial x87 status and control word values
		uint16_t ControlWord;
		uint16_t StatusWord{Getx87StatusAndControlWords(ControlWord)};
		assert(!(StatusWord & ((1 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 1)));// The Interrupt Request, Stack Fault, Precision, Underflow, Overflow, Zero divide, Denormalized and Invalid operation exception flags should not be set.
		ControlWord &= ~((1 << 15) | (1 << 14) | (1 << 13) | (1 << 12) | (1 << 7) | (1 << 6));// mask out the unused bits
		assert(ControlWord == ((1 << 9) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 1));// The Rounding Control should be set to nound to nearest or to even if equidistant, the Precision Control should be set to 53 bits (IEEE 754 double equivalent), and the flags for all 6 exception masks should be set.
#endif
		// verify the initial mxcsr value
		uint32_t mxcsr{_mm_getcsr()};
		assert(mxcsr == (_MM_MASK_MASK | _MM_ROUND_NEAREST | _MM_FLUSH_ZERO_OFF | _MM_DENORMALS_ZERO_OFF));// no exceptions should be set, all exceptions masked, rounding to nearest or to even if equidistant, subnormal values are not flushed to zero nor interpreted as zero
	}
#endif

	// Verify that the RDTSCP CPU feature which is required for the performance tests is available.
	if(!gk_FBCPUId.RDTSCP){
		MessageBoxW(nullptr, L"RDTSCP CPU feature not available", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
		return{0};// failure status
	}

	// Enable the heap terminate-on-corruption security option.
	BOOL boHeapSetInformation{HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0)};
	static_cast<void>(boHeapSetInformation);
	assert(boHeapSetInformation);

	wchar_t szTicksRu64Text[24];// the debug output strings are filled in here
	// wWinMain entry time
	WritePaddedu64(szTicksRu64Text, PerfCounter100ns());
	*reinterpret_cast<uint64_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint64_t>(L'\n') << 32 | static_cast<uint64_t>(L'w') << 16 | static_cast<uint64_t>(L' ');// the last wchar_t is correctly set to zero here
	// output debug strings to the system
	OutputDebugStringW(szTicksRu64Text);

	{
		// Set time critical process and thread priority and single-processor operating mode.
		// Set the security descriptor to allow changing the process information.
		// Note: NtCurrentProcess()/ZwCurrentProcess(), NtCurrentThread()/ZwCurrentThread() and NtCurrentSession()/ZwCurrentSession() resolve to macros defined to HANDLE (void *) values of (sign-extended) -1, -2 and -3 respectively in Wdm.h. Due to being hard-coded in user- and kernel-mode drivers like this, these values are pretty certain to never change on this platform.
		DWORD dr{SetSecurityInfo(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-1)), SE_KERNEL_OBJECT, PROCESS_SET_INFORMATION, nullptr, nullptr, nullptr, nullptr)};
		if(ERROR_SUCCESS != dr){
			MessageBoxW(nullptr, L"SetSecurityInfo() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}
		BOOL boSetPriorityClass{SetPriorityClass(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-1)), REALTIME_PRIORITY_CLASS)};
		if(!boSetPriorityClass){
			MessageBoxW(nullptr, L"SetPriorityClass() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}
		DWORD_PTR ProcessAffinityMask, SystemAffinityMask;
		BOOL boGetProcessAffinityMask{GetProcessAffinityMask(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-1)), &ProcessAffinityMask, &SystemAffinityMask)};
		if(!boGetProcessAffinityMask){
			MessageBoxW(nullptr, L"GetProcessAffinityMask() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}
		// Setting the affinity masks is required because these methods must be tested with warmed-up caches for constant performance.
		// Context switching to another processor or another processor core might cause changes to power states when context switches take place.
		BOOL boSetProcessAffinityMask{SetProcessAffinityMask(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-1)), SystemAffinityMask & 1)};// only the first processor and only the first core
		if(!boSetProcessAffinityMask){
			MessageBoxW(nullptr, L"SetProcessAffinityMask() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}
		DWORD_PTR dpm{SetThreadAffinityMask(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-2)), SystemAffinityMask & 1)};// only the first processor and only the first core
		static_cast<void>(dpm);// the old mask, this can be 0
		BOOL boSetThreadPriority{SetThreadPriority(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-2)), THREAD_PRIORITY_TIME_CRITICAL)};
		if(!boSetThreadPriority){
			MessageBoxW(nullptr, L"SetThreadPriority() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}

		// Enable the permissions to use large pages for VirtualAlloc().
		HANDLE hToken;
		BOOL boOpenProcessToken{OpenProcessToken(reinterpret_cast<HANDLE>(static_cast<intptr_t>(-1)), TOKEN_ADJUST_PRIVILEGES, &hToken)};
		if(!boOpenProcessToken){
			MessageBoxW(nullptr, L"OpenProcessToken() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}
		// Fill in the struct for AdjustTokenPrivileges().
		struct TOKEN_PRIVILEGES_1UNIT{DWORD PrivilegeCount; LUID_AND_ATTRIBUTES Privilege[1];}Info;
		Info.PrivilegeCount = 1;
		Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
		// Get the LUID.
		BOOL boLookupPrivilegeValueW{LookupPrivilegeValueW(nullptr, SE_LOCK_MEMORY_NAME, &Info.Privilege[0].Luid)};
		if(!boLookupPrivilegeValueW){
			MessageBoxW(nullptr, L"LookupPrivilegeValueW() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}
		// Adjust the lock memory privilege.
		BOOL boAdjustTokenPrivileges{AdjustTokenPrivileges(hToken, FALSE, reinterpret_cast<PTOKEN_PRIVILEGES>(&Info), 0, nullptr, nullptr)};
		BOOL boCloseHandle{CloseHandle(hToken)};// cleanup
		static_cast<void>(boCloseHandle);
		assert(boCloseHandle);
		if(!boAdjustTokenPrivileges){
			MessageBoxW(nullptr, L"AdjustTokenPrivileges() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}
	}

	// allocate 1 GiB for the in- and outputs
	SIZE_T upLargePageSize{GetLargePageMinimum()};
	upLargePageSize = !upLargePageSize? 1 : upLargePageSize;// just set it to 1 if the system doesn't support large pages
	assert(!(upLargePageSize - 1 & upLargePageSize));// only one bit should be set in the value of upLargePageSize
	size_t upLargePageSizem1{upLargePageSize - 1};
	size_t upSizeIn{(upLargePageSizem1 & -static_cast<ptrdiff_t>(1073741824)) + 1073741824};// round up to the nearest multiple of upLargePageSize
	void *in{VirtualAlloc(nullptr, upSizeIn, MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)};
	if(!in){
		MessageBoxW(nullptr, L"out of memory failure", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
		return{0};// failure status
	}
	size_t upSizeOut{(upLargePageSizem1 & -static_cast<ptrdiff_t>(1073741824 + 2048)) + (1073741824 + 2048)};// round up to the nearest multiple of upLargePageSize
	void *oriout{VirtualAlloc(nullptr, upSizeOut, MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)};// add half a page
	if(!oriout){
		BOOL boVirtualFree{VirtualFree(in, 0, MEM_RELEASE)};
		static_cast<void>(boVirtualFree);
		assert(boVirtualFree);
		MessageBoxW(nullptr, L"out of memory failure", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
		return{0};// failure status
	}
	void *out{reinterpret_cast<int8_t *>(oriout) + 2048};// offset by half a page, this is an optimization using the processor's addressing methods, and this is used in many memory copy routines

	// measure the TSC execution base time to subtract from the results (method taken from an Intel manual)
	SwitchToThread();// prevent context switching during the benchmark
	uint64_t u64init;
	{
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		u64init = u64stop - u64start;

		srand(static_cast<unsigned int>(u64start));// prepare a seed for rand()
	}
	{
		// filled initialization of the input part (only done once)
		static_assert(RAND_MAX == 0x7FFF, L"RAND_MAX changed from 0x7FFF (15 bits of data), update this part of the code");
		uint64_t *pFIin{reinterpret_cast<uint64_t *>(in)};
		uint32_t j{134217728ui32};// in 134217728 batches of 8 bytes
		do{
			*pFIin++ = static_cast<uint64_t>(static_cast<unsigned int>(rand())) << 60
				| static_cast<uint64_t>(static_cast<unsigned int>(rand())) << 45 | static_cast<uint64_t>(static_cast<unsigned int>(rand())) << 30
				| static_cast<uint64_t>(static_cast<unsigned int>(rand()) << 15) | static_cast<uint64_t>(static_cast<unsigned int>(rand()));
		}while(--j);
	}

	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
		do{
			_mm_stream_ps(pFIout, xf);
			pFIout += 4;
		}while(--j);

		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		uint32_t *pSource{reinterpret_cast<uint32_t *>(in)};
		uint32_t *pDest{reinterpret_cast<uint32_t *>(out)};
		uint32_t i{134217728ui32};// in 134217728 batches of 8 bytes
		do{
			uint32_t arx{pSource[0]}, ary{pSource[1]};
			pSource += 2;
			pDest[0] = arx;
			pDest[1] = ary;
			pDest += 2;
		}while(--i);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::sort(reinterpret_cast<uint16_t *>(out), reinterpret_cast<uint16_t *>(out) + 256 * 1024 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<uint16_t *>(out), reinterpret_cast<uint16_t *>(out) + 256 * 1024 * 1024 / sizeof(uint16_t)));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
		do{
			_mm_stream_ps(pFIout, xf);
			pFIout += 4;
		}while(--j);

		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		uint32_t *pSource{reinterpret_cast<uint32_t *>(in)};
		uint32_t *pDest{reinterpret_cast<uint32_t *>(out)};
		uint32_t i{134217728ui32};// in 134217728 batches of 8 bytes
		do{
			uint32_t arx{pSource[0]}, ary{pSource[1]};
			pSource += 2;
			pDest[0] = arx;
			pDest[1] = ary;
			pDest += 2;
		}while(--i);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out), reinterpret_cast<uint16_t *>(out) + 256 * 1024 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<uint16_t *>(out), reinterpret_cast<uint16_t *>(out) + 256 * 1024 * 1024 / sizeof(uint16_t)));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
		do{
			_mm_stream_ps(pFIout, xf);
			pFIout += 4;
		}while(--j);

		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		uint32_t *pSource{reinterpret_cast<uint32_t *>(in)};
		uint32_t *pDest{reinterpret_cast<uint32_t *>(out)};
		uint32_t i{134217728ui32};// in 134217728 batches of 8 bytes
		do{
			uint32_t arx{pSource[0]}, ary{pSource[1]};
			pSource += 2;
			pDest[0] = arx;
			pDest[1] = ary;
			pDest += 2;
		}while(--i);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsort2(reinterpret_cast<uint16_t *>(out), 256 * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsort2() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<uint16_t *>(out), reinterpret_cast<uint16_t *>(out) + 256 * 1024 * 1024 / sizeof(uint16_t)));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
		do{
			_mm_stream_ps(pFIout, xf);
			pFIout += 4;
		}while(--j);

		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		uint32_t *pSource{reinterpret_cast<uint32_t *>(in)};
		uint32_t *pDest{reinterpret_cast<uint32_t *>(out)};
		uint32_t i{134217728ui32};// in 134217728 batches of 8 bytes
		do{
			uint32_t arx{pSource[0]}, ary{pSource[1]};
			pSource += 2;
			pDest[0] = arx;
			pDest[1] = ary;
			pDest += 2;
		}while(--i);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8(reinterpret_cast<uint16_t *>(out), 256 * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<uint16_t *>(out), reinterpret_cast<uint16_t *>(out) + 256 * 1024 * 1024 / sizeof(uint16_t)));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
		do{
			_mm_stream_ps(pFIout, xf);
			pFIout += 4;
		}while(--j);

		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		uint32_t *pSource{reinterpret_cast<uint32_t *>(in)};
		uint32_t *pDest{reinterpret_cast<uint32_t *>(out)};
		uint32_t i{134217728ui32};// in 134217728 batches of 8 bytes
		do{
			uint32_t arx{pSource[0]}, ary{pSource[1]};
			pSource += 2;
			pDest[0] = arx;
			pDest[1] = ary;
			pDest += 2;
		}while(--i);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out), 256 * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<uint16_t *>(out), reinterpret_cast<uint16_t *>(out) + 256 * 1024 * 1024 / sizeof(uint16_t)));
	}
	OutputDebugStringW(L"Warning: these absolutely tiny tests can be ruined by minor scheduling and system-wide interruptions.\nDiscard benchmarks that deviate from expected readings, and re-do the benchmarking session as needed.\n");
	// memory layout: 2 tests take 32 MiB, the next 48 tests are spaced apart 4 MiB each to keep very high alignment, so the total is filled to 256 MiB here
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
		do{
			_mm_stream_ps(pFIout, xf);
			pFIout += 4;
		}while(--j);

		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		uint32_t *pSource{reinterpret_cast<uint32_t *>(in)};
		uint32_t *pDest{reinterpret_cast<uint32_t *>(out)};
		uint32_t i{134217728ui32};// in 134217728 batches of 8 bytes
		do{
			uint32_t arx{pSource[0]}, ary{pSource[1]};
			pSource += 2;
			pDest[0] = arx;
			pDest[1] = ary;
			pDest += 2;
		}while(--i);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out), 32 * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 32 MiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + 32 * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + 2 * 32 * 1024 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 32 MiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + 2 * 32 * 1024 * 1024 / sizeof(uint16_t), 4 * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 4 MiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 2 * 4) * 1024 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 4 MiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 2 * 4) * 1024 * 1024 / sizeof(uint16_t), 512 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 512 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 3 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 3 * 4) * 1024 * 1024 / sizeof(uint16_t) + 512 * 1024);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 512 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 4 * 4) * 1024 * 1024 / sizeof(uint16_t), 64 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 64 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 5 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 5 * 4) * 1024 * 1024 / sizeof(uint16_t) + 64 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 64 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 6 * 4) * 1024 * 1024 / sizeof(uint16_t), 8 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 8 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 7 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 7 * 4) * 1024 * 1024 / sizeof(uint16_t) + 8 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 8 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 8 * 4) * 1024 * 1024 / sizeof(uint16_t), 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 1 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 9 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 9 * 4) * 1024 * 1024 / sizeof(uint16_t) + 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 1 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 10 * 4) * 1024 * 1024 / sizeof(uint16_t), 128 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 128 B instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 11 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 11 * 4) * 1024 * 1024 / sizeof(uint16_t) + 128 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 128 B instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 12 * 4) * 1024 * 1024 / sizeof(uint16_t), 16 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 16 B instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 13 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 13 * 4) * 1024 * 1024 / sizeof(uint16_t) + 16 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 16 B instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 14 * 4) * 1024 * 1024 / sizeof(uint16_t), 3 * 512 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 1.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 15 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 15 * 4) * 1024 * 1024 / sizeof(uint16_t) + 3 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 1.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 16 * 4) * 1024 * 1024 / sizeof(uint16_t), 2 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 2 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 17 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 17 * 4) * 1024 * 1024 / sizeof(uint16_t) + 2 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 2 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 18 * 4) * 1024 * 1024 / sizeof(uint16_t), 5 * 512 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 2.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 19 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 19 * 4) * 1024 * 1024 / sizeof(uint16_t) + 5 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 2.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 20 * 4) * 1024 * 1024 / sizeof(uint16_t), 3 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 3 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 21 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 21 * 4) * 1024 * 1024 / sizeof(uint16_t) + 3 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 3 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 22 * 4) * 1024 * 1024 / sizeof(uint16_t), 7 * 512 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 3.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 23 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 23 * 4) * 1024 * 1024 / sizeof(uint16_t) + 7 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 3.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 24 * 4) * 1024 * 1024 / sizeof(uint16_t), 4 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 4 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 25 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 25 * 4) * 1024 * 1024 / sizeof(uint16_t) + 4 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 4 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 26 * 4) * 1024 * 1024 / sizeof(uint16_t), 9 * 512 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 4.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 27 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 27 * 4) * 1024 * 1024 / sizeof(uint16_t) + 9 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 4.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 28 * 4) * 1024 * 1024 / sizeof(uint16_t), 5 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 29 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 29 * 4) * 1024 * 1024 / sizeof(uint16_t) + 5 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 30 * 4) * 1024 * 1024 / sizeof(uint16_t), 11 * 512 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 5.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 31 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 31 * 4) * 1024 * 1024 / sizeof(uint16_t) + 11 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 5.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 32 * 4) * 1024 * 1024 / sizeof(uint16_t), 6 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 6 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 33 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 33 * 4) * 1024 * 1024 / sizeof(uint16_t) + 6 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 6 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 34 * 4) * 1024 * 1024 / sizeof(uint16_t), 13 * 512 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 6.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 35 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 35 * 4) * 1024 * 1024 / sizeof(uint16_t) + 13 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 6.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 36 * 4) * 1024 * 1024 / sizeof(uint16_t), 7 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 7 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 37 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 37 * 4) * 1024 * 1024 / sizeof(uint16_t) + 7 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 7 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 38 * 4) * 1024 * 1024 / sizeof(uint16_t), 15 * 512 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of 7.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 39 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 39 * 4) * 1024 * 1024 / sizeof(uint16_t) + 15 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 7.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 40 * 4) * 1024 * 1024 / sizeof(uint16_t), 4 * 128 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of .5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 41 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 41 * 4) * 1024 * 1024 / sizeof(uint16_t) + 4 * 128 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of .5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 42 * 4) * 1024 * 1024 / sizeof(uint16_t), 5 * 128 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of .625 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 43 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 43 * 4) * 1024 * 1024 / sizeof(uint16_t) + 5 * 128 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of .625 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 44 * 4) * 1024 * 1024 / sizeof(uint16_t), 6 * 128 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of .75 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 45 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 45 * 4) * 1024 * 1024 / sizeof(uint16_t) + 6 * 128 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of .75 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		radixsortbidi8unroll(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 46 * 4) * 1024 * 1024 / sizeof(uint16_t), 7 * 128 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"radixsortbidi8unroll() test of .875 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 47 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 47 * 4) * 1024 * 1024 / sizeof(uint16_t) + 7 * 128 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of .875 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
		do{
			_mm_stream_ps(pFIout, xf);
			pFIout += 4;
		}while(--j);

		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		uint32_t *pSource{reinterpret_cast<uint32_t *>(in)};
		uint32_t *pDest{reinterpret_cast<uint32_t *>(out)};
		uint32_t i{134217728ui32};// in 134217728 batches of 8 bytes
		do{
			uint32_t arx{pSource[0]}, ary{pSource[1]};
			pSource += 2;
			pDest[0] = arx;
			pDest[1] = ary;
			pDest += 2;
		}while(--i);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(256 * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<uint16_t *>(out), reinterpret_cast<uint16_t *>(out) + 256 * 1024 * 1024 / sizeof(uint16_t)));
	}
	OutputDebugStringW(L"Warning: these absolutely tiny tests can be ruined by minor scheduling and system-wide interruptions.\nDiscard benchmarks that deviate from expected readings, and re-do the benchmarking session as needed.\n");
	// memory layout: 2 tests take 32 MiB, the next 48 tests are spaced apart 4 MiB each to keep very high alignment, so the total is filled to 256 MiB here
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
		do{
			_mm_stream_ps(pFIout, xf);
			pFIout += 4;
		}while(--j);

		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		uint32_t *pSource{reinterpret_cast<uint32_t *>(in)};
		uint32_t *pDest{reinterpret_cast<uint32_t *>(out)};
		uint32_t i{134217728ui32};// in 134217728 batches of 8 bytes
		do{
			uint32_t arx{pSource[0]}, ary{pSource[1]};
			pSource += 2;
			pDest[0] = arx;
			pDest[1] = ary;
			pDest += 2;
		}while(--i);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(32 * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 32 MiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + 32 * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + 2 * 32 * 1024 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 32 MiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(4 * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + 2 * 32 * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 4 MiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 2 * 4) * 1024 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 4 MiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(512 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 2 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 512 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 3 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 3 * 4) * 1024 * 1024 / sizeof(uint16_t) + 512 * 1024);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 512 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(64 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 4 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 64 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 5 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 5 * 4) * 1024 * 1024 / sizeof(uint16_t) + 64 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 64 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(8 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 6 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 8 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 7 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 7 * 4) * 1024 * 1024 / sizeof(uint16_t) + 8 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 8 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 8 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 1 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 9 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 9 * 4) * 1024 * 1024 / sizeof(uint16_t) + 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 1 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(128 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 10 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 128 B instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 11 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 11 * 4) * 1024 * 1024 / sizeof(uint16_t) + 128 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 128 B instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(16 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 12 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 16 B instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 13 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 13 * 4) * 1024 * 1024 / sizeof(uint16_t) + 16 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 16 B instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(3 * 512 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 14 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 1.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 15 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 15 * 4) * 1024 * 1024 / sizeof(uint16_t) + 3 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 1.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(2 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 16 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 2 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 17 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 17 * 4) * 1024 * 1024 / sizeof(uint16_t) + 2 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 2 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(5 * 512 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 18 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 2.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 19 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 19 * 4) * 1024 * 1024 / sizeof(uint16_t) + 5 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 2.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(3 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 20 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 3 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 21 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 21 * 4) * 1024 * 1024 / sizeof(uint16_t) + 3 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 3 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(7 * 512 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 22 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 3.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 23 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 23 * 4) * 1024 * 1024 / sizeof(uint16_t) + 7 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 3.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(4 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 24 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 4 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 25 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 25 * 4) * 1024 * 1024 / sizeof(uint16_t) + 4 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 4 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(9 * 512 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 26 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 4.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 27 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 27 * 4) * 1024 * 1024 / sizeof(uint16_t) + 9 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 4.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(5 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 28 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 29 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 29 * 4) * 1024 * 1024 / sizeof(uint16_t) + 5 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(11 * 512 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 30 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 5.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 31 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 31 * 4) * 1024 * 1024 / sizeof(uint16_t) + 11 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 5.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(6 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 32 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 6 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 33 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 33 * 4) * 1024 * 1024 / sizeof(uint16_t) + 6 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 6 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(13 * 512 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 34 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 6.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 35 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 35 * 4) * 1024 * 1024 / sizeof(uint16_t) + 13 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 6.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(7 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 36 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 7 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 37 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 37 * 4) * 1024 * 1024 / sizeof(uint16_t) + 7 * 1024 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 7 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(15 * 512 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 38 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of 7.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 39 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 39 * 4) * 1024 * 1024 / sizeof(uint16_t) + 15 * 512 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of 7.5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(4 * 128 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 40 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of .5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 41 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 41 * 4) * 1024 * 1024 / sizeof(uint16_t) + 4 * 128 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of .5 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(5 * 128 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 42 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of .625 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 43 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 43 * 4) * 1024 * 1024 / sizeof(uint16_t) + 5 * 128 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of .625 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(6 * 128 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 44 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of .75 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 45 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 45 * 4) * 1024 * 1024 / sizeof(uint16_t) + 6 * 128 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of .75 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(7 * 128 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 46 * 4) * 1024 * 1024 / sizeof(uint16_t), upLargePageSize);

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"rsbd8::radixsort() test of .875 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		SwitchToThread();// prevent context switching during the benchmark
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		uint64_t u64start{__rdtsc()};

		std::stable_sort(reinterpret_cast<uint16_t *>(out) + (2 * 32 + 47 * 4) * 1024 * 1024 / sizeof(uint16_t), reinterpret_cast<uint16_t *>(out) + (2 * 32 + 47 * 4) * 1024 * 1024 / sizeof(uint16_t) + 7 * 128 / sizeof(uint16_t));

		// stop measuring
		uint64_t u64stop;
		{
			unsigned int uAux;// unused
			u64stop = __rdtscp(&uAux);
			static_cast<void>(uAux);
		}
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		WritePaddedu64(szTicksRu64Text, u64stop - u64start - u64init);
		*reinterpret_cast<uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::stable_sort() test of .875 KiB instead of 1 GiB\n");
		OutputDebugStringW(szTicksRu64Text);
	}

	// benchmark finished time
	WritePaddedu64(szTicksRu64Text, PerfCounter100ns());
	*reinterpret_cast<uint64_t UNALIGNED*>(szTicksRu64Text + 20) = static_cast<uint64_t>(L'\n') << 32 | static_cast<uint64_t>(L'b') << 16 | static_cast<uint64_t>(L' ');// the last wchar_t is correctly set to zero here
	// output debug strings to the system
	OutputDebugStringW(szTicksRu64Text);

	BOOL boVirtualFreeIn{VirtualFree(in, 0, MEM_RELEASE)};
	static_cast<void>(boVirtualFreeIn);
	assert(boVirtualFreeIn);
	BOOL boVirtualFreeOut{VirtualFree(oriout, 0, MEM_RELEASE)};
	static_cast<void>(boVirtualFreeOut);
	assert(boVirtualFreeOut);

	// perform application initialization
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof WNDCLASSEXW;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);
	wcex.hIcon = static_cast<HICON>(LoadImageW(reinterpret_cast<HINSTANCE>(&__ImageBase), MAKEINTRESOURCEW(IDI_WINDOWSPROJECT1), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
	assert(wcex.hIcon);// internal resource, no allocation of a new item
	wcex.hCursor = static_cast<HCURSOR>(LoadImageW(nullptr, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_DEFAULTCOLOR | LR_SHARED | LR_DEFAULTSIZE));
	assert(wcex.hCursor);// global system resource, no allocation of a new item
	wcex.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<uintptr_t>(COLOR_WINDOWFRAME));
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
	wchar_t const *szkFromResource;
	int lenProjName0endincl{LoadStringW(reinterpret_cast<HINSTANCE>(&__ImageBase), IDC_WINDOWSPROJECT1, reinterpret_cast<LPWSTR>(&szkFromResource), 0)};// get const pointer to resource
	static_cast<void>(lenProjName0endincl);
	assert(lenProjName0endincl);
	assert(szkFromResource);
	wcex.lpszClassName = szkFromResource;
	wcex.hIconSm = static_cast<HICON>(LoadImageW(reinterpret_cast<HINSTANCE>(&__ImageBase), MAKEINTRESOURCEW(IDI_SMALL), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
	assert(wcex.hIconSm);// internal resource, no allocation of a new item
	if(ATOM aClass{RegisterClassExW(&wcex)}){
		int lenAppTitle0endincl{LoadStringW(reinterpret_cast<HINSTANCE>(&__ImageBase), IDS_APP_TITLE, reinterpret_cast<LPWSTR>(&szkFromResource), 0)};// get const pointer to resource
		static_cast<void>(lenAppTitle0endincl);
		assert(lenAppTitle0endincl);
		assert(szkFromResource);
		if(HWND hWnd{CreateWindowExW(0, MAKEINTRESOURCEW(aClass), szkFromResource, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, reinterpret_cast<HINSTANCE>(&__ImageBase), nullptr)}){
			BOOL boShowWindow{ShowWindow(hWnd, nCmdShow)};
			static_cast<void>(boShowWindow);// ShowWindow() returns a BOOL that indicates if the window was previously visible
			BOOL boUpdateWindow{UpdateWindow(hWnd)};
			static_cast<void>(boUpdateWindow);
			if(HACCEL hAccelTable{LoadAcceleratorsW(reinterpret_cast<HINSTANCE>(&__ImageBase), MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1))}){
				// main message loop
				MSG msg;
				BOOL boGetMessageW;
				goto GetFirstMessage;
				do{
					if(boGetMessageW == -1){
						MessageBoxW(hWnd, L"GetMessageW() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
						msg.wParam = 0;// failure status for the return statement
						break;
					}
					if(!TranslateAcceleratorW(msg.hwnd, hAccelTable, &msg)){
						BOOL boTranslateMessage{TranslateMessage(&msg)};
						static_cast<void>(boTranslateMessage);
						LRESULT lrDispatchMessageW{DispatchMessageW(&msg)};
						static_cast<void>(lrDispatchMessageW);
					}
GetFirstMessage:
					boGetMessageW = GetMessageW(&msg, nullptr, 0, 0);
				}while(boGetMessageW);
				BOOL boDestroyAcceleratorTable{DestroyAcceleratorTable(hAccelTable)};
				static_cast<void>(boDestroyAcceleratorTable);
				assert(boDestroyAcceleratorTable);
				BOOL boDestroyWindow{DestroyWindow(hWnd)};
				static_cast<void>(boDestroyWindow);// DestroyWindow() returns a BOOL that indicates if the window was destroyed here or earlier by the system
				BOOL boUnregisterClassW{UnregisterClassW(MAKEINTRESOURCEW(aClass), reinterpret_cast<HINSTANCE>(&__ImageBase))};
				static_cast<void>(boUnregisterClassW);
				assert(boUnregisterClassW);
				return{static_cast<int>(msg.wParam)};
			}else MessageBoxW(hWnd, L"LoadAcceleratorsW() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			// cleanup and return on errors
			BOOL boDestroyWindow{DestroyWindow(hWnd)};
			static_cast<void>(boDestroyWindow);
			assert(boDestroyWindow);
		}else MessageBoxW(nullptr, L"CreateWindowExW() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
		// cleanup and return on errors
		BOOL boUnregisterClassW{UnregisterClassW(MAKEINTRESOURCEW(aClass), reinterpret_cast<HINSTANCE>(&__ImageBase))};
		static_cast<void>(boUnregisterClassW);
		assert(boUnregisterClassW);
	}else MessageBoxW(nullptr, L"RegisterClassExW() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
	return{0};// failure status
}