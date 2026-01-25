; MIT License
; Copyright (c) 2026 Jan-Willem Krans (janwillem32 <at> hotmail <dot> com)
; Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
; The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

option casemap:none
public mainforward@@64
public mainbackward@@48
public mergeforward@@24
public mergebackward@@24

_TEXT$ segment align(32); force instruction code segment align to 32 (the instruction buffer block size in many modern CPUs) to allow aligned code for call enties and jump targets for loop constructs, so omit .code and make a custom _TEXT block here
mainforward@@64 proc
; vectorcall function arguments:
; ecx: usemultithread
; rdx: psrclo
; r8: pdst
; r9: poffset (32-bit unsigned integer indices version)
; xmm4[31:0]: count
; xmm5[31:0]: shifter
; no return value
; Test for performance issues on the sorting main loop with indirection: no varparameters, 64-bit unsigned integers, multithreaded
; This function is modeled after the C++ function radixsortnoallocmulti2threadmain() in the original project.
; configuration: 64-bit unsigned integer with simple indirection, no offsets, ascending forward ordered sorting, 32-bit indices
; This is a leaf function without internal stack modification or exception handling.
; The instructions are grouped to indicate parallel execution in a modern superscalar CPU with proper register renaming and without any penalties for a full-register write-after-read or write-after-write.
	;std::ptrdiff_t j{static_cast<std::ptrdiff_t>((count + 1) >> (2 + usemultithread))};// rounded down in the bottom part
	movd r10d, xmm4; j
	prefetcht0 [rdx]
	movq xmm0, rbx; preserve rbx
	movq xmm1, rbp; preserve rbp (rbp/r13 can't be a base register with no displacement, so these are not used as a pointer here)
	movq xmm2, rsi; preserve rsi
	movq xmm3, rdi; preserve rdi
	add ecx, 2

	inc r10d
	punpcklqdq xmm5, xmm0; {shifter, rbx}
	movq xmm0, r12; preserve r12
	punpcklqdq xmm4, xmm1; {count, rbp}
	movq xmm1, r13; preserve r13 (rbp/r13 can't be a base register with no displacement, so these are not used as a pointer here)
	punpcklqdq xmm2, xmm3; {rsi, rdi}
	movq xmm3, r14; preserve r14

	shr r10d, cl
	prefetcht0 [rdx+64]
	prefetcht0 [rdx+64*2]
	prefetcht0 [rdx+64*3]
	punpcklqdq xmm0, xmm1; {r12, r13}
	movq xmm1, r15; preserve r15
	movd ecx, xmm5; shifter

	punpcklqdq xmm3, xmm1; {r14, r15}
	movq xmm1, r8; pdst
	prefetcht0 [rdx+64*4]
	prefetcht0 [rdx+64*5]
	prefetcht0 [rdx+64*6]
	prefetcht0 [rdx+64*7]
	;while(0 <= --j){// fill the array, four at a time
	dec r10d

	js skipmainloop; possible macro-op fusion of dec r and js
align 32; high alignment for a very common jump target
mainloop:
	;V *pa{psrclo[0]};
	;V *pb{psrclo[1]};
	;V *pc{psrclo[2]};
	;V *pd{psrclo[3]};
	mov r11, [rdx]; pa
	mov r12, [rdx+8]; pb
	mov r14, [rdx+8*2]; pc
	mov r15, [rdx+8*3]; pd
	prefetcht0 [rdx+8*4+64*7]
	mov ebp, 1; offseta

	;psrclo += 4;
	;auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
	;auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
	;auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
	;auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
	;auto outa{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
	;auto outb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
	;auto outc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
	;auto outd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
	mov r8, [r11]; outd
	mov rax, [r12]; outa
	mov rbx, [r14]; outb
	mov r13, [r15]; outc
	mov esi, ebp; offsetb
	mov edi, ebp; offsetc

	;auto[cura, curb, curc, curd]{filtershift8<isabsvalue, issignmode, isfltpmode, T, U>(outa, outb, outc, outd, shifter)};
	shr r8, cl
	shr rax, cl
	shr rbx, cl
	shr r13, cl
	mov ecx, ebp; offsetd

	movzx r8d, r8b
	movzx eax, al
	movzx ebx, bl
	movzx r13d, r13b
	add rdx, 8*4

	;std::size_t offseta{poffset[cura]++};// the next item will be placed one higher
	;std::size_t offsetb{poffset[curb]++};
	;std::size_t offsetc{poffset[curc]++};
	;std::size_t offsetd{poffset[curd]++};
	xadd [r9+r8*4], ebp; also increments the index by 1 (read+write operation), but the address internally only has to be resolved once
	movq r8, xmm1; pdst
	xadd [r9+rax*4], esi
	xadd [r9+rbx*4], edi
	xadd [r9+r13*4], ecx

	;pdst[offseta] = pa;
	;pdst[offsetb] = pb;
	;pdst[offsetc] = pc;
	;pdst[offsetd] = pd;
	mov [r8+rbp*8], r11
	mov [r8+rsi*8], r12
	mov [r8+rdi*8], r14
	mov [r8+rcx*8], r15
	movd ecx, xmm5; shifter
	dec r10d

	jns mainloop; possible macro-op fusion of dec r and jns

	;}
skipmainloop:; rare jump target, so no additional padding for alignement here
	;if(2 & count + 1){// fill in the final two items for a remainder of 2 or 3
	movd esi, xmm4; count

	inc esi

	test esi, 2

	jz skiplasttwo; possible macro-op fusion of test r, i and jz

	;V *pa{psrclo[0]};
	;V *pb{psrclo[1]};
	mov r11, [rdx]; pa
	mov r12, [rdx+8]; pb
	mov ebp, 1; offseta

	;psrclo += 2;
	;auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
	;auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
	;auto outa{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
	;auto outb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
	mov rdi, [r11]; outa
	mov rbx, [r12]; outb
	mov esi, ebp; offsetb

	;auto[cura, curb]{filtershift8<isabsvalue, issignmode, isfltpmode, T, U>(outa, outb, shifter)};
	shr rdi, cl
	shr rbx, cl

	movzx edi, dil
	movzx ebx, bl

	;std::size_t offseta{poffset[cura]++};// the next item will be placed one higher
	;std::size_t offsetb{poffset[curb]++};
	xadd [r9+rdi*4], ebp; also increments the index by 1 (read+write operation), but the address internally only has to be resolved once
	xadd [r9+rbx*4], esi
	add rdx, 8*2

	;pdst[offseta] = pa;
	;pdst[offsetb] = pb;
	mov [r8+rbp*8], r11
	mov [r8+rsi*8], r12

	;}
	;}
skiplasttwo:; rare jump target, so no additional padding for alignement here
	;if(!(1 & count)){// fill in the final item for odd counts
	test esi, 1; esi was already incremented by 1 earlier

	jz exit; possible macro-op fusion of test r, i and jz

	;V *p{psrclo[0]};
	mov r14, [rdx]; p

	;auto im{indirectinput1<indirection1, isindexed2, T, V>(p, varparameters...)};
	;auto out{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
	mov r13, [r14]; out

	;std::size_t cur{filtershift8<isabsvalue, issignmode, isfltpmode, T, U>(out, shifter)};
	shr r13, cl

	movzx r13d, r13b

	;std::size_t offset{poffset[cur]};
	mov r13d, [r9+r13*4]

	;pdst[offset] = p;
	mov [r8+r13*8], r14
	;}
exit:; rare jump target, so no additional padding for alignement here
	movq rsi, xmm2; restore rsi
	punpckhqdq xmm2, xmm2; rdi
	movq r12, xmm0; restore r12
	punpckhqdq xmm0, xmm0; r13
	movq r14, xmm3; restore r14
	punpckhqdq xmm3, xmm3; r15
	punpckhqdq xmm5, xmm5; rbx
	punpckhqdq xmm4, xmm4; rbp

	movq rdi, xmm2; restore rdi
	movq r13, xmm0; restore r13
	movq r15, xmm3; restore r15
	movq rbx, xmm5; restore rbx
	movq rbp, xmm4; restore rbp
	ret; callee stack cleanup (64 bytes were passed by register)
mainforward@@64 endp
align 32; align the next function entry like the very first of this file
mainbackward@@48 proc
; vectorcall function arguments:
; ecx: shifter
; rdx: psrchi
; r8: pdst
; r9: poffset (32-bit unsigned integer indices version)
; xmm4[31:0]: count
; no return value
; Test for performance issues on the sorting main loop with indirection: no varparameters, 64-bit unsigned integers, multithreaded
; This function is modeled after the C++ function radixsortnoallocmulti2threadmainmtc() in the original project.
; configuration: 64-bit unsigned integer with simple indirection, no offsets, ascending forward ordered sorting, 32-bit indices
; This is a leaf function without internal stack modification or exception handling.
; The instructions are grouped to indicate parallel execution in a modern superscalar CPU with proper register renaming and without any penalties for a full-register write-after-read or write-after-write.
	;std::size_t j{(count + 1 + 4) >> 3};// rounded up in the top part
	movd r10d, xmm4; j
	prefetcht0 [rdx]
	prefetcht0 [rdx-64]
	prefetcht0 [rdx-64*2]
	movd xmm0, ecx; shifter
	movq xmm1, rbx; preserve rbx
	movq xmm2, rbp; preserve rbp (rbp/r13 can't be a base register with no displacement, so these are not used as a pointer here)
	movq xmm3, rsi; preserve rsi
	movq xmm5, rdi; preserve rdi
	movq xmm4, r12; preserve r12

	add r10d, 5
	prefetcht0 [rdx-64*3]
	prefetcht0 [rdx-64*4]
	prefetcht0 [rdx-64*5]
	punpcklqdq xmm0, xmm1; {shifter, rbx}
	movq xmm1, r13; preserve r13 (rbp/r13 can't be a base register with no displacement, so these are not used as a pointer here)
	punpcklqdq xmm2, xmm3; {rbp, rsi}
	movq xmm3, r14; preserve r14
	punpcklqdq xmm5, xmm4; {rdi, r12}
	movq xmm4, r15; preserve r15

	shr r10d, 3
	prefetcht0 [rdx-64*6]
	prefetcht0 [rdx-64*7]
	punpcklqdq xmm1, xmm3; {r13, r14}
	movq xmm3, r8; pdst
	;do{// fill the array, four at a time
align 32; high alignment for a very common jump target
mainloop:
	;V *pa{psrchi[0]};
	;V *pb{psrchi[-1]};
	;V *pc{psrchi[-2]};
	;V *pd{psrchi[-3]};
	mov r11, [rdx]; pa
	mov r12, [rdx-8]; pb
	mov r14, [rdx-8*2]; pc
	mov r15, [rdx-8*3]; pd
	prefetcht0 [rdx-8*4-64*7]
	or ebp, -1; offseta

	;psrchi -= 4;
	;auto ima{indirectinput1<indirection1, isindexed2, T, V>(pa, varparameters...)};
	;auto imb{indirectinput1<indirection1, isindexed2, T, V>(pb, varparameters...)};
	;auto imc{indirectinput1<indirection1, isindexed2, T, V>(pc, varparameters...)};
	;auto imd{indirectinput1<indirection1, isindexed2, T, V>(pd, varparameters...)};
	;auto outa{indirectinput2<indirection1, indirection2, isindexed2, T>(ima, varparameters...)};
	;auto outb{indirectinput2<indirection1, indirection2, isindexed2, T>(imb, varparameters...)};
	;auto outc{indirectinput2<indirection1, indirection2, isindexed2, T>(imc, varparameters...)};
	;auto outd{indirectinput2<indirection1, indirection2, isindexed2, T>(imd, varparameters...)};
	mov rax, [r11]; outa
	mov rbx, [r12]; outb
	mov r13, [r14]; outc
	mov r8, [r15]; outd
	mov esi, ebp; offsetb
	mov edi, ebp; offsetc

	;auto[cura, curb, curc, curd]{filtershift8<isabsvalue, issignmode, isfltpmode, T, U>(outa, outb, outc, outd, shifter)};
	shr rax, cl
	shr rbx, cl
	shr r13, cl
	shr r8, cl
	mov ecx, ebp; offsetd

	movzx eax, al
	movzx ebx, bl
	movzx r13d, r13b
	movzx r8d, r8b
	sub rdx, 8*4

	;std::size_t offseta{poffset[cura]--};// the next item will be placed one lower
	;std::size_t offsetb{poffset[curb]--};
	;std::size_t offsetc{poffset[curc]--};
	;std::size_t offsetd{poffset[curd]--};
	xadd [r9+rax*4], ebp; also decrements the index by 1 (read+write operation), but the address internally only has to be resolved once
	movq rax, xmm3; pdst
	xadd [r9+rbx*4], esi
	xadd [r9+r13*4], edi
	xadd [r9+r8*4], ecx

	;pdst[offseta] = pa;
	;pdst[offsetb] = pb;
	;pdst[offsetc] = pc;
	;pdst[offsetd] = pd;
	mov [rax+rbp*8], r11
	mov [rax+rsi*8], r12
	mov [rax+rdi*8], r14
	mov [rax+rcx*8], r15
	movd ecx, xmm0; shifter
	;}while(--j);
	dec r10d

	jnz mainloop; possible macro-op fusion of dec r and jnz

	movq rbp, xmm2; restore rbp
	punpckhqdq xmm2, xmm2; rsi
	movq rdi, xmm5; restore rdi
	punpckhqdq xmm5, xmm5; r12
	movq r13, xmm1; restore r13
	punpckhqdq xmm1, xmm1; r14
	punpckhqdq xmm0, xmm0; rbx
	movq r15, xmm4; restore r15

	movq rsi, xmm2; restore rsi
	movq r12, xmm5; restore r12
	movq r14, xmm1; restore r14
	movq rbx, xmm0; restore rbx
	ret; callee stack cleanup (48 bytes were passed by register)
mainbackward@@48 endp
align 32; align the next function entry like the very first of this file
mergeforward@@24 proc
; vectorcall function arguments:
; rcx: count
; rdx: input
; r8: output
; no return value
; Test for performance issues on the sorting main loop with indirection: no varparameters, 64-bit unsigned integers, multithreaded
; This function is modeled after the C++ function mergehalvesmain() in the original project.
; configuration: 64-bit unsigned integer with simple indirection, no offsets, ascending forward ordered sorting
; This is a leaf function without internal stack modification or exception handling.
; The instructions are grouped to indicate parallel execution in a modern superscalar CPU with proper register renaming and without any penalties for a full-register write-after-read or write-after-write.
	;// process the lower half here (rounded down)
	;std::size_t halfcount{count >> 1};
	;std::intptr_t const *pdatalo, *pdatahi;
	;if constexpr(!isrevorder) pdatalo = reinterpret_cast<std::intptr_t const *>(input), pdatahi = reinterpret_cast<std::intptr_t const *>(input) + halfcount;
	;else pdatalo = reinterpret_cast<std::intptr_t const *>(input) + halfcount, pdatahi = reinterpret_cast<std::intptr_t const *>(input);
	;std::intptr_t *pout{reinterpret_cast<std::intptr_t *>(output)};
	;--halfcount;// rounded down and one less, as the final item is handled outside of the loop
	;std::intptr_t plo{*pdatalo}, phi{*pdatahi};
	shr rcx, 1; halfcount
	prefetcht0 [rdx]
	mov r11, [rdx]; plo
	prefetcht0 [rdx+64]
	prefetcht0 [rdx+64*2]
	prefetcht0 [rdx+64*3]
	prefetcht0 [rdx+64*4]
	movq xmm0, rbp; preserve rbp (rbp/r13 can't be a base register with no displacement, so these are not used as a pointer here)

	lea r10, [rdx+rcx*8]; pdatahi, pdatalo in rdx, pout in r8
	prefetcht0 [rdx+64*5]
	prefetcht0 [rdx+64*6]
	prefetcht0 [rdx+64*7]
	dec rcx
	movq xmm1, r13; preserve r13 (rbp/r13 can't be a base register with no displacement, so these are not used as a pointer here)
	movq xmm2, r15; preserve r15
	movq xmm3, r14; preserve r14

	prefetcht0 [r10]
	mov r9, [r10]; phi
	;auto imlo{indirectinput1<indirection1, isindexed2, T, V>(reinterpret_cast<V *>(plo), varparameters...)};
	;auto imhi{indirectinput1<indirection1, isindexed2, T, V>(reinterpret_cast<V *>(phi), varparameters...)};
	;auto curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
	;auto curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
	;auto[complo, comphi]{convertinput<isabsvalue, issignmode, isfltpmode, T>(curlo, curhi)};
	mov r13, [r11]; complo
	prefetcht0 [r10+64]
	prefetcht0 [r10+64*2]
	prefetcht0 [r10+64*3]
	movq xmm4, r12; preserve r12

	mov rbp, [r9]; comphi
	prefetcht0 [r10+64*4]
	prefetcht0 [r10+64*5]
	prefetcht0 [r10+64*6]
	prefetcht0 [r10+64*7]
	movq xmm5, rbx; preserve rbx
	punpcklqdq xmm0, xmm1; {rbp, r13}
	movq xmm1, rdi; preserve rdi

	;do{
align 32; high alignment for a very common jump target
mainloop:
	;// line breaks are placed for clarity, based on the dependency sequences
	;std::intptr_t mask{-static_cast<std::intptr_t>(!isdescsort? comphi < complo : complo < comphi)};
	;std::intptr_t notmask{~mask};
	cmp rbp, r13
	mov r12, r9; outhi

	sbb r15, r15; notmask
	mov rbx, r11; outlo

	mov r14, r15; mask
	not r15

	;pdatahi -= mask;
	;std::intptr_t outhi{phi & mask};
	;pdatalo -= notmask;
	;std::intptr_t outlo{plo & notmask};
	lea rax, [r14*8]; temp for hi
	and r12, r14
	lea rdi, [r15*8]; temp for lo
	and rbx, r15

	sub r10, rax
	mov rax, r14; platesthi
	sub rdx, rdi
	mov rdi, r15; platestlo

	;std::intptr_t platesthi{reinterpret_cast<std::intptr_t>(pdatahi) & mask};
	;std::intptr_t platestlo{reinterpret_cast<std::intptr_t>(pdatalo) & notmask};
	;outhi |= outlo;
	and rax, r10
	and rdi, rdx
	or r12, rbx

	;platesthi |= platestlo;
	;phi &= notmask;
	;plo &= mask;
	or rax, rdi
	and r9, r15
	and r11, r14

	;std::intptr_t latesthi{*reinterpret_cast<std::intptr_t const *>(platesthi)};
	;comphi &= static_cast<M>(notmask);
	;*pout++ = outhi;
	prefetcht0 [rax+8+64*7]
	mov rax, [rax]; latesthi
	and rbp, r15
	mov [r8], r12

	;auto im{indirectinput1<indirection1, isindexed2, T, V>(reinterpret_cast<V *>(latesthi), varparameters...)};
	;auto cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
	;auto complatesthi{convertinput<isabsvalue, issignmode, isfltpmode, T>(cur)};// convert the value for unsigned comparison
	;complo &= static_cast<M>(mask);
	;std::intptr_t latestlo{latesthi};
	;latesthi &= mask;
	mov rbx, [rax]; complatesthi
	and r13, r14
	mov rdi, rax; latestlo
	and rax, r14

	;auto complatestlo{complatesthi};
	;complatesthi &= static_cast<M>(mask);
	;latestlo &= notmask;
	;phi |= latesthi;
	mov r12, rbx; complatestlo
	and rbx, r14
	and rdi, r15
	or r9, rax

	;complatestlo &= static_cast<M>(notmask);
	;comphi |= complatesthi;
	;plo |= latestlo;
	and r12, r15
	or rbp, rbx
	or r11, rdi
	add r8, 8

	;complo |= complatestlo;
	;}while(--halfcount);
	or r13, r12
	dec rcx

	jnz mainloop; possible macro-op fusion of dec r and jnz

	;// finalise
	;if(!isdescsort? comphi < complo : complo < comphi){
	;plo = phi;
	;}
	cmp rbp, r13
	movq rbp, xmm0; restore rbp
	punpckhqdq xmm0, xmm0; r13
	movq r15, xmm2; restore r15

	cmovb r11, r9
	movq r14, xmm3; restore r14
	movq r12, xmm4; restore r12
	movq rbx, xmm5; restore rbx

	;*pout = plo;
	mov [r8], r11
	movq rdi, xmm1; restore rdi
	movq r13, xmm0; restore r13
	ret; callee stack cleanup (24 bytes were passed by register)
mergeforward@@24 endp
align 32; align the next function entry like the very first of this file
mergebackward@@24 proc
; vectorcall function arguments:
; rcx: count
; rdx: input
; r8: output
; no return value
; Test for performance issues on the sorting main loop with indirection: no varparameters, 64-bit unsigned integers, multithreaded
; This function is modeled after the C++ function mergehalvesmtc() in the original project.
; configuration: 64-bit unsigned integer with simple indirection, no offsets, ascending forward ordered sorting
; This is a leaf function without internal stack modification or exception handling.
; The instructions are grouped to indicate parallel execution in a modern superscalar CPU with proper register renaming and without any penalties for a full-register write-after-read or write-after-write.
	;// process the lower half here (rounded down)
	;std::size_t halfcount{count >> 1};
	;std::intptr_t const *pdatalo, *pdatahi;
	;if constexpr(!isrevorder) pdatahi = reinterpret_cast<std::intptr_t const *>(input) + (count - 1), pdatalo = reinterpret_cast<std::intptr_t const *>(input) + halfcount;
	;else pdatahi = reinterpret_cast<std::intptr_t const *>(input) + halfcount, pdatalo = reinterpret_cast<std::intptr_t const *>(input) + (count - 1);
	;std::intptr_t *pout{reinterpret_cast<std::intptr_t *>(output) + (count - 1)};
	;--halfcount;// rounded down and one less, as the final item is handled outside of the loop
	;std::intptr_t phi{*pdatahi}, plo{*pdatalo};
	lea r11, [rdx+rcx*8-8]; pdatahi
	lea r8, [r8+rcx*8-8]; pout
	mov rax, rcx; count
	shr rcx, 1; halfcount
	movq xmm0, rdx; input

	mov r10, [r11]; phi
	;auto imlo{indirectinput1<indirection1, isindexed2, T, V>(reinterpret_cast<V *>(plo), varparameters...)};
	;auto imhi{indirectinput1<indirection1, isindexed2, T, V>(reinterpret_cast<V *>(phi), varparameters...)};
	;auto curlo{indirectinput2<indirection1, indirection2, isindexed2, T>(imlo, varparameters...)};
	;auto curhi{indirectinput2<indirection1, indirection2, isindexed2, T>(imhi, varparameters...)};
	;auto[complo, comphi]{convertinput<isabsvalue, issignmode, isfltpmode, T>(curlo, curhi)};
	lea rdx, [rdx+rcx*8]; pdatalo
	movq xmm1, rbp; preserve rbp (rbp/r13 can't be a base register with no displacement, so these are not used as a pointer here)
	movq xmm2, r13; preserve r13 (rbp/r13 can't be a base register with no displacement, so these are not used as a pointer here)
	movq xmm3, r15; preserve r15

	mov rbp, [r10]; comphi
	prefetcht0 [rdx]
	mov r9, [rdx]; plo
	dec rcx
	prefetcht0 [rdx-64]
	prefetcht0 [rdx-64*2]
	prefetcht0 [rdx-64*3]
	prefetcht0 [rdx-64*4]
	prefetcht0 [rdx-64*5]
	prefetcht0 [rdx-64*6]
	prefetcht0 [rdx-64*7]
	movq xmm5, r12; preserve r12
	movq xmm4, r14; preserve r14
	punpcklqdq xmm0, xmm2; {input, r13}

	prefetcht0 [r9]
	mov r13, [r9]; complo
	prefetcht0 [r9-64]
	prefetcht0 [r9-64*2]
	prefetcht0 [r9-64*3]
	prefetcht0 [r9-64*4]
	prefetcht0 [r9-64*5]
	prefetcht0 [r9-64*6]
	prefetcht0 [r9-64*7]
	movq xmm2, rbx; preserve rbx
	punpcklqdq xmm1, xmm3; {rbp, r15}
	movq xmm3, rdi; preserve rdi
	punpcklqdq xmm4, xmm5; {r14, r12}
	movq xmm5, rsi; preserve rsi

	;do{
align 32; high alignment for a very common jump target
mainloop:
	;// line breaks are placed for clarity, based on the dependency sequences
	;std::intptr_t mask{-static_cast<std::intptr_t>(!isdescsort? comphi < complo : complo < comphi)};
	;std::intptr_t notmask{~mask};
	cmp rbp, r13
	mov r12, rdx; outlo

	sbb r15, r15; notmask
	mov rbx, r10; outhi

	mov r14, r15; mask
	not r15

	;pdatalo += mask;
	;pdatahi += notmask;
	lea rdx, [rdx+r14*8]
	mov rdi, r14; platestlo
	lea r11, [r11+r15*8]
	mov rsi, r15; platesthi

	;std::intptr_t platestlo{reinterpret_cast<std::intptr_t>(pdatalo) & mask};
	;std::intptr_t platesthi{reinterpret_cast<std::intptr_t>(pdatahi) & notmask};
	;std::intptr_t outlo{plo & mask};
	;std::intptr_t outhi{phi & notmask};
	and rdi, rdx
	and rsi, r11
	and r12, r14
	and rbx, r15

	;platestlo |= platesthi;
	;outlo |= outhi;
	;plo &= notmask;
	;complo &= static_cast<M>(notmask);
	or rdi, rsi
	or r12, rbx
	and r9, r15
	and rbp, r15

	;std::intptr_t latestlo{*reinterpret_cast<std::intptr_t const *>(platestlo)};
	;*pout-- = outlo;
	;phi &= mask;
	prefetcht0 [rdi-8-64*7]
	mov rdi, [rdi]
	mov [r8], r12
	and r10, r14

	;auto im{indirectinput1<indirection1, isindexed2, T, V>(reinterpret_cast<V *>(latestlo), varparameters...)};
	;auto cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
	;auto complatestlo{convertinput<isabsvalue, issignmode, isfltpmode, T>(cur)};// convert the value for unsigned comparison
	;std::intptr_t latesthi{latestlo};
	;latestlo &= mask;
	;comphi &= static_cast<M>(mask);
	mov rsi, [rdi]; complatestlo
	mov rbx, rdi; latesthi
	and rdi, r14
	and r13, r14

	;auto complatesthi{complatestlo};
	;complatestlo &= static_cast<M>(mask);
	;plo |= latestlo;
	;latesthi &= notmask;
	mov r12, rsi; complatesthi
	and rsi, r14
	or r9, rdi
	and rbx, r15

	;complatesthi &= static_cast<M>(notmask);
	;complo |= complatestlo;
	;phi |= latesthi;
	and r12, r15
	or rbp, rsi
	or r10, rbx
	sub r8, 8

	;comphi |= complatesthi;
	or r13, r12
	;}while(--halfcount);
	dec rcx

	jnz mainloop

	;if(1 & count){// odd counts will be handled here
	test eax, 1

	jz exit

	;// line breaks are placed for clarity, based on the dependency sequences
	;std::intptr_t mask{-static_cast<std::intptr_t>(!isdescsort? comphi < complo : complo < comphi)};
	;std::intptr_t notmask{~mask};
	cmp rbp, r13
	movq rax, xmm0; input

	sbb r15, r15; notmask
	mov r12, rdx; outlo
	mov rbx, r10; outhi

	mov r14, r15; mask
	not r15

	;pdatalo += mask;
	;std::intptr_t outlo{plo & mask};
	;pdatahi += notmask;
	;std::intptr_t outhi{phi & notmask};
	add rdx, r14
	and r12, r14
	add r11, r15
	and rbx, r15

	;// the only modification here is this part
	;// never sample beyond the lower division (half of count, rounded down) of the array
	;// this doesn't happen to the upper half, as it has 1 more item to process in odd-count cases
	;if constexpr(!isrevorder){
	;if(pdatalo < reinterpret_cast<std::intptr_t const *>(input)) pdatalo = pdatahi;
	;}else if(pdatahi < reinterpret_cast<std::intptr_t const *>(input)) pdatahi = pdatalo;
	;outlo |= outhi;
	;plo &= notmask;
	cmp rdx, rax
	mov rdi, r14; platestlo
	mov rsi, r15; platesthi

	cmovb rdx, r11
	or r12, rbx
	and r9, r15

	;std::intptr_t platestlo{reinterpret_cast<std::intptr_t>(pdatalo) & mask};
	;std::intptr_t platesthi{reinterpret_cast<std::intptr_t>(pdatahi) & notmask};
	;*pout-- = outlo;
	and rdi, rdx
	and rsi, r11
	mov [r8], r12

	;platestlo |= platesthi;
	;phi &= mask;
	;complo &= static_cast<M>(notmask);
	or rdi, rsi
	and r10, r14
	and rbp, r15

	;std::intptr_t latestlo{*reinterpret_cast<std::intptr_t const *>(platestlo)};
	;comphi &= static_cast<M>(mask);
	mov rdi, [rdi]
	and r13, r14
	sub r8, 8

	;auto im{indirectinput1<indirection1, isindexed2, T, V>(reinterpret_cast<V *>(latestlo), varparameters...)};
	;auto cur{indirectinput2<indirection1, indirection2, isindexed2, T>(im, varparameters...)};
	;auto complatestlo{convertinput<isabsvalue, issignmode, isfltpmode, T>(cur)};// convert the value for unsigned comparison
	;std::intptr_t latesthi{latestlo};
	;latestlo &= mask;
	mov rsi, [rdi]; complatestlo
	mov rbx, rdi; latesthi
	and rdi, r14

	;auto complatesthi{complatestlo};
	;complatestlo &= static_cast<M>(mask);
	;latesthi &= notmask;
	;plo |= latestlo;
	mov r12, rsi; complatesthi
	and rsi, r14
	and rbx, r15
	or r9, rdi

	;complatesthi &= static_cast<M>(notmask);
	;complo |= complatestlo;
	;phi |= latesthi;
	and r12, r15
	or rbp, rsi
	or r10, rbx

	;comphi |= complatesthi;
	or r13, r12

	;}
exit:; rare jump target, so no additional padding for alignement here
	;// finalise
	;if(!isdescsort? comphi < complo : complo < comphi){
	;phi = plo;
	;}
	cmp rbp, r13
	movq rbp, xmm1; restore rbp
	punpckhqdq xmm1, xmm1; r15
	movq r14, xmm4; restore r14
	punpckhqdq xmm4, xmm4; r12

	cmovb r10, r9
	punpckhqdq xmm0, xmm0; r13
	movq rbx, xmm2; restore rbx
	movq rdi, xmm3; restore rdi
	movq rsi, xmm5; restore rsi

	;*pout = phi;
	mov [r8], r10
	movq r15, xmm1; restore r15
	movq r12, xmm4; restore r12
	movq r13, xmm0; restore r13
	ret; callee stack cleanup (24 bytes were passed by register)
mergebackward@@24 endp
_TEXT$ ends
end