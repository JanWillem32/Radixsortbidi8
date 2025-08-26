; MIT License
; Copyright (c) 2021 Jan-Willem Krans (janwillem32 <at> hotmail <dot> com)
; Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
; The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

.686p
.xmm
.model flat; no calling convention set, both the "c" and "stdcall" options will only mangle the names with an underscore prefix
option casemap:none
public WritePaddedu64ra@@24

_TEXT$ segment align(32); force instruction code segment align to 32 (the instruction buffer block size in many modern CPUs) to allow aligned code for call enties and jump targets for loop constructs, so omit .code and make a custom _TEXT block here
WritePaddedu64ra@@24 proc
; vectorcall function arguments:
; ecx: pointer to the output array of 20 wide characters
; edx: the high half the input value
; xmm0[31:0]: the low half the input value
; return value in eax
; This function writes a right-aligned 20-wide-character decimal number array from a 64-bit unsigned integer value and returns a pointer to the character that is the leftmost digit.
; This function is modeled after the C++ function WritePaddedu64() in the original project.
; This is a leaf function without internal stack modification or exception handling.
; The instructions are grouped to indicate parallel execution in a modern superscalar CPU with proper register renaming and without any penalties for a full-register write-after-read or write-after-write.
	;// 18446744073709551615 is the maximum output by this function, the output is padded on the left with spaces if required to get to 20 characters
	;static uint64_t constexpr fourspaces{static_cast<uint64_t>(L' ') << 48 | static_cast<uint64_t>(L' ') << 32 | static_cast<uint64_t>(L' ') << 16 | static_cast<uint64_t>(L' ')};
	;reinterpret_cast<uint64_t UNALIGNED *>(pwcOut)[0] = fourspaces;
	;reinterpret_cast<uint64_t UNALIGNED *>(pwcOut)[1] = fourspaces;
	;reinterpret_cast<uint64_t UNALIGNED *>(pwcOut)[2] = fourspaces;
	;reinterpret_cast<uint64_t UNALIGNED *>(pwcOut)[3] = fourspaces;
	;reinterpret_cast<uint64_t UNALIGNED *>(pwcOut)[4] = fourspaces;// writes one wchar_t more than required, but it gets overwritten by the first output in the loop anyway
	;// original code:
	;//do{
	;//*pwcOut-- = static_cast<wchar_t>(static_cast<uint32_t>(n % 10ui64) + L'0'); the last digit is never substituted with a space
	;//n /= 10ui64;
	;//}while(n);
	;// assembly checked: the x64 compiler eliminates the need for the div instruction here, the x86-32 compiler isn't able to optimize this routine.
	;// 64-bit builds do use the 64-bit unsigned division by invariant integers using multiplication method, but the compiler is only capable of selecting the most basic variant of its methods, and not the simplified versions.
	;// 32-bit builds use the 64-bit division support library (__aulldvrm(), __aulldiv(), __alldvrm() and __alldiv()) leaving the modulo and division operations completely unoptimized.
	;uint32_t nlo{static_cast<uint32_t>(n)}, nhi{static_cast<uint32_t>(n >> 32)};
	movd xmm1, ebx; preserve ebx
	mov ebx, 00200020h; two spaces concatenated
	movd eax, xmm0; the low half the input value
	movd xmm2, ebp; preserve ebp
	mov ebp, 0CCCCCCCCh
	pcmpeqd xmm3, xmm3; set all bits

	mov [ecx], ebx
	mov [ecx+4], ebx
	mov [ecx+8], ebx
	mov [ecx+12], ebx
	mov [ecx+16], ebx
	mov [ecx+20], ebx
	mov [ecx+24], ebx
	mov [ecx+28], ebx
	mov [ecx+32], ebx
	mov [ecx+36], ebx
	;pwcOut += 19;// make it point at the least significand digit first
	;if(nhi){// at most ten iterations of division by ten are needed to reduce the size of the dividend from a 64-bit unsigned integer to a 32-bit unsigned integer
	add ecx, 2*19
	test edx, edx; possible macro-op fusion of test r, r and jz

	jz OnlyLowHalf

	psllq xmm3, 1; {-2, -2}
	movd xmm4, ecx; store pointer
	mov ebx, edx; the high half the input value
	movd xmm5, esi; preserve esi
	movd xmm6, edi; preserve edi

align 32; Using instruction buffer block size-aligned innermost loop jump targets is a common optimization. The alignment here is this high because the entire loop is definitely more than 16 bytes of bytecode.
HandleHighHalf:
	;uint32_t qhi;
	;do{
	;// 64-bit unsigned division by invariant integers using multiplication
	;// n / 10
	;static uint8_t constexpr sh_post{3ui8};
	;static uint64_t constexpr mprime{0xCCCCCCCCCCCCCCCDui64}, d{0x000000000000000Aui64};
	;//uint64_t q{__umulh(n, mprime) >> sh_post};
	;// given that the top and bottom half of mprime only differ by one, a little optimization is allowed
	;uint32_t constexpr mprimehi{static_cast<uint32_t>(mprime >> 32)};
	;static_assert(static_cast<uint32_t>(mprime & 0x00000000FFFFFFFFui64) == mprimehi + 1ui32);
	;// unsigned 64-by-64-to-128-bit multiply
	;uint64_t j2{__emulu(nlo, mprimehi)};
	;uint32_t n1a{static_cast<uint32_t>(j2)}, n2{static_cast<uint32_t>(j2 >> 32)};
	mov esi, eax; nlo
	mul ebp
	paddq xmm4, xmm3; pointer-2, it now points too low for the current character in this iteration, but that will be corrected when writing to memory

	;// calculate __emulu(nlo, mprimehi + 1ui32) and __emulu(nhi, mprimehi + 1ui32) as __emulu(nlo, mprimehi) + nlo and __emulu(nhi, mprimehi) + nhi
	;// sum table, from least to most significant word
	;// nlo nhi
	;// n1a n2
	;//     n1a n2
	;//     n2a n3
	;//         n2a n3
	;assert(n2 < 0xFFFFFFFFui32);// even __emulu(0xFFFFFFFFui32, 0xFFFFFFFFui32) will only output 0xFFFFFFFE00000001ui64, so adding the carry to just n2 is not a problem here
	;uint32_t n0, n1;
	;unsigned char EmptyCarry0{_addcarry_u32(_addcarry_u32(_addcarry_u32(0, nlo, n1a, &n0), nhi, n2, &n1), n2, 0ui32, &n2)};// add nlo and nhi to the earlier computed parts
	;assert(!EmptyCarry0);// the last add-with-carry cannot carry out
	;static_cast<void>(EmptyCarry0);
	;static_cast<void>(n0);// the lowest part is discarded, as it cannot carry out when it has no further addend
	mov ecx, eax; n1a
	add eax, esi; n0 (discarded)
	mov edi, edx; n2
	mov eax, ebx; nhi

	adc ebx, edx; n1

	adc edi, 0; n2
	;uint64_t j3{__emulu(nhi, mprimehi)};
	;uint32_t n2a{static_cast<uint32_t>(j3)}, n3{static_cast<uint32_t>(j3 >> 32)};
	;unsigned char EmptyCarry1{_addcarry_u32(_addcarry_u32(_addcarry_u32(0, n1, n1a, &n1), n2, n3, &n2), n3, 0ui32, &n3)};
	;assert(!EmptyCarry1);// the last add-with-carry cannot carry out
	;static_cast<void>(EmptyCarry1);
	mul ebp
	add ebx, ecx; n1
	movd ecx, xmm4; pointer-2

	adc edi, edx; n2

	adc edx, 0; n3
	;unsigned char EmptyCarry2{_addcarry_u32(_addcarry_u32(_addcarry_u32(0, n1, n2a, &n1), n2, n2a, &n2), n3, 0ui32, &n3)};
	;assert(!EmptyCarry2);// the last add-with-carry cannot carry out
	;static_cast<void>(EmptyCarry2);
	;// n0 and n1 are discarded
	add ebx, eax; n1 (discarded)

	adc eax, edi; n2

	adc edx, 0; n3

	;uint32_t at_1[2]{n2, n3}; uint64_t t_1{*reinterpret_cast<uint64_t UNALIGNED *>(at_1)};// recompose
	;uint64_t q{t_1 >> sh_post};
	;uint32_t qlo{static_cast<uint32_t>(q)};
	;qhi = static_cast<uint32_t>(q >> 32);
	shrd eax, edx, 3
	shr edx, 3

	;// n % 10
	;// there is no trivially computable remainder for this method
	;// given that we actually only need the bottom 4 bits here (the remainder is 4 bits: ceil(log2(divisor - 1 + 1))), a little optimization is allowed
	;//uint32_t r{nlo - qlo * static_cast<uint32_t>(d)};
	;//*pwcOut-- = static_cast<wchar_t>(r + L'0');
	;uint32_t rm{qlo * 4ui32 + qlo};// lea, q * 5
	;nhi = qhi;
	lea edi, [eax*4+eax]
	mov ebx, edx

	;rm = rm * 2ui32 - L'0';// lea, q * 10 - L'0'
	lea edi, [edi*2-30h]; L'0'

	;*pwcOut-- = static_cast<wchar_t>(nlo - rm);// sub, n - (q * 10 - L'0')
	;nlo = qlo;
	;}while(qhi);
	sub esi, edi

	mov [ecx+2], si; correct pointer offset
	test edx, edx; possible macro-op fusion of test r, r and jnz

	jnz HandleHighHalf

	;assert(nlo > 9ui32);
	;goto HandleLowHalf;// skip the first check, as nlo will guaranteed be larger than 9
	inc ebp; 0CCCCCCCDh
	movd esi, xmm5; restore esi
	movd edi, xmm6; restore edi
	jmp HandleLowHalf

	;}
align 16; This is a common jump target, and nops will not be executed here. Put at least the first 3 instructions (10 bytes of bytecode in this case) in the instruction buffer at this point.
OnlyLowHalf:
	;if(nlo > 9ui32){// use a simpler loop when the high half of the dividend is zero
	cmp eax, 9; possible macro-op fusion of cmp r, i and jbe

	jbe SetLastCharacter

	inc ebp; 0CCCCCCCDh

align 32; Using instruction buffer block size-aligned innermost loop jump targets is a common optimization. The alignment here is this high because the entire loop is 32 bytes of bytecode.
HandleLowHalf:
	;uint32_t q;
	;do{
	;// 32-bit unsigned division by invariant integers using multiplication
	;// n / 10
	;static uint8_t constexpr sh_post{3ui8};
	;static uint32_t constexpr mprime{0xCCCCCCCDui32}, d{0x0000000Aui32};
	;q = static_cast<uint32_t>(__emulu(nlo, mprime) >> 32) >> sh_post;
	mov ebx, eax
	mul ebp

	shr edx, 3

	;// n % 10
	;// there is no trivially computable remainder for this method
	;//uint32_t r{nlo - q * d};
	;//*pwcOut-- = static_cast<wchar_t>(r + L'0');
	;uint32_t rm{q * 4ui32 + q};// lea, q * 5
	lea eax, [edx*4+edx]

	;rm = rm * 2ui32 - L'0';// lea, q * 10 - L'0'
	lea eax, [eax*2-30h]; L'0'

	;*pwcOut-- = static_cast<wchar_t>(nlo - rm);// sub, n - (q * 10 - L'0')
	;nlo = q;
	;}while(q > 9ui32);
	sub ebx, eax
	mov eax, edx

	mov [ecx], bx
	sub ecx, 2
	cmp edx, 9; possible macro-op fusion of cmp r, i and ja

	ja HandleLowHalf

	;}
; This is a rare jump target, and not worth realignment with possibly executing nops for in the sequence above.
SetLastCharacter:
	;*pwcOut = static_cast<wchar_t>(nlo + L'0');// the last digit is never substituted with a space
	;return{pwcOut};
	add eax, 30h; L'0'
	movd ebx, xmm1; restore ebx
	movd ebp, xmm2; restore ebp

	mov [ecx], ax
	mov eax, ecx
	ret; callee stack cleanup (24 bytes were passed by register)
WritePaddedu64ra@@24 endp
_TEXT$ ends
end