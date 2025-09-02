option casemap:none
public InnerRadixSortBidi@@32
public InnerRadixSortBidi8@@32
extrn __chkstk:proc

_TEXT$ segment align(32); force instruction code segment align to 32 (the instruction buffer block size in many modern CPUs) to allow aligned code for call enties and jump targets for loop constructs, so omit .code and make a custom _TEXT block here
InnerRadixSortBidi@@32 proc
; vectorcall function arguments:
; rcx: counts
; rdx: arr
; r8: buffer
; r9: count
; no return value
; This function contains the radix sorting inner loop even for an even size of 64-bit unsigned elements.
; This function is modeled after the C++ function radixsortbidi() in the original project.
; This is a leaf function without internal stack modification or exception handling.
; The instructions are grouped to indicate parallel execution in a modern superscalar CPU with proper register renaming and without any penalties for a full-register write-after-read or write-after-write.
	; layout issue: there are more than 15 objects active in gprs
	; objects of the inner loops (13, so that does fit):
	; r15: curlo
	; r14: pdsthi
	; r13: selhi
	; r12: pdst1hi
	; r11: pdst0hi
	; r10: pdst1nexthi is object 14, which isnt part of the inner loops, but it's used early in the outer loops
	; r9 : curhi
	; r8 : pdst0lo
	; rdi: pdstlo
	; rsi: sello
	; rbp: pdst1lo
	; rbx: psrchi
	; rdx: psrclo
	; ecx: bitselect
	; rax: counts is object 15, which isnt part of the inner loops, but it's used early in the outer loops
	; the last two gprs are special purpose: rsp and rip

	;uint64_t *psrclo{arr}, *pdst0lo{buffer};// just named aliases
	; rdx contains psrclo
	; r8 contains pdst0lo
	;size_t initcount{counts[0]};
	;uint32_t bitselect{};// least significant bit first
	;uint64_t *pdst1hi{pdst0lo + countm1};
	;while(!initcount){// skip a step if all bits for the index are zero
	mov r10, [rcx]; initcount
	mov rax, rcx; counts
	xor ecx, ecx; bitselect
	lea r11, [r8+r9*8-8]; pdst1hi (temporary)
	movq xmm0, rsi; preserve rsi
	movq xmm1, rdi; preserve rdi

	test r10, r10; possible macro-op fusion of test r, r and jnz

	jnz initcountdone

align 16; This is a reasonably common jump target if the entry condition was taken, and nops will very rarely be executed here. Put at least the first 3 instructions (11 bytes of bytecode in this case) in the instruction buffer at this point.
initcountloop:
	;// the version with all ones is much rarer unless working with already sorted array sections and hence not tested here
	;if(64 <= ++bitselect) goto exit;
	inc ecx

	cmp ecx, 64; possible macro-op fusion of cmp r, imm8 and jae

	jae quickexit

	;initcount = counts[bitselect];
	;}
	mov r10, [rax+rcx*8]

	test r10, r10; possible macro-op fusion of test r, r and jz

	jz initcountloop

align 16; This is a common jump target, and nops will very rarely be executed here. Put at least the first 3 instructions (12 bytes of bytecode in this case) in the instruction buffer at this point.
initcountdone:
	;{
	;uint64_t *psrchi{psrclo + countm1};
	;size_t i{count >> 1};// will set the carry bit for the next condition
	;uint64_t *pdst0hi{pdst1hi - initcount};
	;uint64_t *pdst0nextlo{psrclo}, *psrcnextlo{pdst0lo};// for the next iteration
	;uint64_t *psrcnexthi{pdst1hi}, *pdst1nexthi{psrchi};// for the next iteration
	;size_t halfcount{i};// for the internal counter
	;uint64_t *pdst1lo{pdst0hi + 1};
	;if(!(count & 1)) for(;;){// main loop for even counts
	;do{// fill the array, two at a time: one low-to-middle, one high-to-middle
	shl r10, 3
	movq mm0, rbx; preserve rbx, set the x87 registers to the mmx state, so emms will be needed before returning from this function call
	lea rbx, [rdx+r9*8-8]; psrchi
	movq xmm2, r12; preserve r12
	mov r12, r11; pdst1hi

	sub r11, r10; pdst0hi
	movq mm1, rbp; preserve rbp
	movq mm2, rdx; pdst0nextlo
	movq mm3, r8; psrcnextlo
	mov r10, rbx; pdst1nexthi
	movq mm4, r12; psrcnexthi

	lea rbp, [r11+8]; pdst1lo
	movq xmm3, r13; preserve r13
	movq xmm4, r14; preserve r14
	movq xmm5, r15; preserve r15
	test r9, 1; possible macro-op fusion of test r, imm8 and jnz

	jnz mainloopodd; jump if the right shift carried out

align 32; Using instruction buffer block size-aligned innermost loop jump targets is a common optimization. The alignment here is this high because the entire loop is definitely more than 16 bytes of bytecode.
mainloopeven:
	;uint64_t curlo{*psrclo++};
	;uint64_t curhi{*psrchi--};
	mov rsi, [rdx]; curlo (temporary)
	mov r13, [rbx]; curhi (temporary)
	add rdx, 8
	sub rbx, 8
	mov rdi, rbp; pdstlo
	mov r14, r12; pdsthi

	;uint64_t sello{curlo >> bitselect};
	;uint64_t selhi{curhi >> bitselect};
	mov r15, rsi; curlo
	shr rsi, cl; sello
	mov r9, r13; curhi
	shr r13, cl; selhi

	;uint64_t *pdstlo{(sello &= 1)? pdst1lo : pdst0lo};
	;uint64_t *pdsthi{(selhi &= 1)? pdst1hi : pdst0hi};
	;pdst1lo += sello;// lea can shift and add
	;selhi *= 8;// lea cannot subtract
	;sello ^= 1;
	and esi, 1

	cmovz rdi, r8
	and r13d, 1
	lea rbp, [rbp+rsi*8]

	cmovz r14, r11
	shl r13d, 3
	xor esi, 1

	;*pdstlo = curlo;
	;pdst1hi = reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(pdst1hi) - selhi);
	;selhi ^= 8;
	;*pdsthi = curhi;
	;pdst0lo += sello;
	mov [rdi], r15
	sub r12, r13
	xor r13d, 8
	mov [r14], r9
	lea r8, [r8+rsi*8]

	;pdst0hi = reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(pdst0hi) - selhi);
	;}while(psrclo < psrchi);
	sub r11, r13
	cmp rdx, rbx; possible macro-op fusion of cmp r, r and jb

	jb mainloopeven

	;size_t curcount;// skip a step if all bits for the index are zero
	;do{// the version with all ones is much rarer unless working with already sorted array sections and hence not tested here
	;if(64 <= ++bitselect) goto exit;
curcountloopeven:; This is a rare jump target, and nops would often be executed here. Hence there's no realignent here.
	inc ecx

	cmp ecx, 64; possible macro-op fusion of cmp r, imm8 and jae

	jae exit

	;curcount = counts[bitselect];
	;}while(!curcount);
	mov r15, [rax+rcx*8]; curcount

	test r15, r15; possible macro-op fusion of test r, r and jz

	jz curcountloopeven

	;// swap the pointers for the next round, data is moved back and forth on each iteration
	;pdst1hi = pdst1nexthi;
	;pdst0lo = pdst0nextlo;
	;psrchi = psrcnexthi;
	;psrclo = psrcnextlo;
	shl r15, 3; curcount
	mov r11, r10; pdst0hi
	mov r12, r10; pdst1hi
	movq r8, mm2; pdst0lo
	movq rbx, mm4; psrchi
	movq rdx, mm3; psrclo

	;pdst0hi = pdst1nexthi - curcount;
	;psrcnexthi = pdst1nexthi;
	;psrcnextlo = pdst0nextlo;
	;pdst1nexthi = psrchi;
	;pdst0nextlo = psrclo;
	sub r11, r15; pdst0hi
	movq mm4, r10; psrcnexthi
	movq mm3, mm2; psrcnextlo
	mov r10, rbx; pdst1nexthi
	movq mm2, rdx; pdst0nextlo

	;pdst1lo = pdst0hi + 1;
	;}
	lea rbp, [r11+8]; pdst1lo
	jmp mainloopeven
	;else for(;;){// main loop for odd counts
align 32; Using instruction buffer block size-aligned innermost loop jump targets is a common optimization. The alignment here is this high because the entire loop is definitely more than 16 bytes of bytecode.
mainloopodd:
	;uint64_t curlo{*psrclo++};
	;uint64_t curhi{*psrchi--};
	mov rsi, [rdx]; curlo (temporary)
	mov r13, [rbx]; curhi (temporary)
	add rdx, 8
	sub rbx, 8
	mov rdi, rbp; pdstlo
	mov r14, r12; pdsthi

	;uint64_t sello{curlo >> bitselect};
	;uint64_t selhi{curhi >> bitselect};
	mov r15, rsi; curlo
	shr rsi, cl; sello
	mov r9, r13; curhi
	shr r13, cl; selhi

	;uint64_t *pdstlo{(sello &= 1)? pdst1lo : pdst0lo};
	;uint64_t *pdsthi{(selhi &= 1)? pdst1hi : pdst0hi};
	;pdst1lo += sello;// lea can shift and add
	;selhi *= 8;// lea cannot subtract
	;sello ^= 1;
	and esi, 1

	cmovz rdi, r8
	and r13d, 1
	lea rbp, [rbp+rsi*8]

	cmovz r14, r11
	shl r13d, 3
	xor esi, 1

	;*pdstlo = curlo;
	;pdst1hi = reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(pdst1hi) - selhi);
	;selhi ^= 8;
	;*pdsthi = curhi;
	;pdst0lo += sello;
	mov [rdi], r15
	sub r12, r13
	xor r13d, 8
	mov [r14], r9
	lea r8, [r8+rsi*8]

	;pdst0hi = reinterpret_cast<uint64_t *>(reinterpret_cast<uint8_t *>(pdst0hi) - selhi);
	;}while(psrclo < psrchi);
	sub r11, r13
	cmp rdx, rbx; possible macro-op fusion of cmp r, r and jne

	jne mainloopodd

	;// fill in the final item
	;uint64_t curlo{*psrclo};
	;uint64_t currentbit{1ui64 << bitselect};
	mov rsi, [rdx]; curlo (temporary)
	mov edi, 1; currentbit

	shl rdi, cl

	;pdst1lo = (curlo & currentbit)? pdst1lo : pdst0lo;
	;*pdst1lo = curlo;
	test rsi, rdi

	cmovz rbp, r8

	mov [rbp], rsi
	;size_t curcount;// skip a step if all bits for the index are zero
	;do{// the version with all ones is much rarer unless working with already sorted array sections and hence not tested here
	;if(64 <= ++bitselect) goto exit;
curcountloopodd:; This is a rare jump target, and nops would often be executed here. Hence there's no realignent here.
	inc ecx

	cmp ecx, 64; possible macro-op fusion of cmp r, imm8 and jae

	jae exit

	;curcount = counts[bitselect];
	;}while(!curcount);
	mov r15, [rax+rcx*8]; curcount

	test r15, r15; possible macro-op fusion of test r, r and jz

	jz curcountloopodd

	;// swap the pointers for the next round, data is moved back and forth on each iteration
	;pdst1hi = pdst1nexthi;
	;pdst0lo = pdst0nextlo;
	;psrchi = psrcnexthi;
	;psrclo = psrcnextlo;
	shl r15, 3; curcount
	mov r11, r10; pdst0hi
	mov r12, r10; pdst1hi
	movq r8, mm2; pdst0lo
	movq rbx, mm4; psrchi
	movq rdx, mm3; psrclo

	;pdst0hi = pdst1nexthi - curcount;
	;psrcnexthi = pdst1nexthi;
	;psrcnextlo = pdst0nextlo;
	;pdst1nexthi = psrchi;
	;pdst0nextlo = psrclo;
	sub r11, r15; pdst0hi
	movq mm4, r10; psrcnexthi
	movq mm3, mm2; psrcnextlo
	mov r10, rbx; pdst1nexthi
	movq mm2, rdx; pdst0nextlo

	;pdst1lo = pdst0hi + 1;
	;}
	lea rbp, [r11+8]; pdst1lo
	jmp mainloopodd
	;}
align 16; This is a reasonably common jump target, and nops will not be executed here. Put at least the first 3 instructions (10 bytes of bytecode in this case) in the instruction buffer at this point.
exit:
	movq rbx, mm0; restore rbx
	movq rbp, mm1; restore rbp
	emms; reset the x87 registers from the mmx state
	movq rsi, xmm0; restore rsi
	movq rdi, xmm1; restore rdi
	movq r12, xmm2; restore r12
	movq r13, xmm3; restore r13
	movq r14, xmm4; restore r14
	movq r15, xmm5; restore r15
quickexit:; This is a rare jump target, and nops would often be executed here. Hence there's no realignent here.
	ret; callee stack cleanup (32 bytes were passed by register)
InnerRadixSortBidi@@32 endp
align 32; align the next function entry like the very first of this file
InnerRadixSortBidi8@@32 proc
; vectorcall function arguments:
; rcx: arr
; rdx: buffer
; r8: count
; r9b: movetobuffer
; no return value
; This function contains the radix sorting inner loop even for an even size of 64-bit unsigned elements.
; This function is modeled after the C++ function radixsortbidi8() in the original project.
; This is function has no exception handling.
; The instructions are grouped to indicate parallel execution in a modern superscalar CPU with proper register renaming and without any penalties for a full-register write-after-read or write-after-write.
	; This function uses the stack, so this is not a leaf function. Proper prologue and epilogue code is used here, but without an exception frame.
	; prolog part with a stack larger than a 4096-byte page
	mov [rsp+8], rbx
	mov [rsp+16], rbp
	mov [rsp+24], rsi
	mov [rsp+32], rdi
	;// the next part requires the previous check for at least 3 elements
	;size_t offsetslo[8][256]{}, offsetshi[8][256];// 32 kibibytes of indices on a 64-bit system, but it's worth it
	; [rsp+8000h, rsp+8018h) is useful padding to provide 16-byte alignment here
	mov eax, 3 * 8 + 2 * 8 * 8 * 256; set up the rather large stack, give offsetslo (low memory at rsp) and offsetshi (high memory at rsp+4000h) at least 16-byte alignment
	call __chkstk
	sub rsp, rax

	mov [rsp+8000h], r12
	mov [rsp+8008h], r13
	mov [rsp+8010h], r14
	;size_t countm1{count - 1};
	;bool paritybool;// for when the swap count is odd
	;uint8_t runsteps;// 0 to 8
	;{// count the 256 configurations, all in one go
	;ptrdiff_t i{static_cast<ptrdiff_t>(countm1)};
	xorps xmm0, xmm0
	mov rbx, rsp
	mov ebp, 8 * 8 * 256 / 32

align 16; This is a tight loop jump target, and nops will only be executed once here. Put all instructions (15 bytes of bytecode in this case) in the instruction buffer at this point.
clearingloop:
	movaps [rbx], xmm0
	movaps [rbx+16], xmm0
	add rbx, 32
	dec ebp; possible macro-op fusion of dec r and jnz

	jnz clearingloop

	lea r10, [r8-1]; i
	mov edi, 0FFh * 8; common mask
align 32; Using instruction buffer block size-aligned innermost loop jump targets is a common optimization. The alignment here is this high because the entire loop is definitely more than 16 bytes of bytecode.
countingloop:
	;do{// the lower half of offsets is zeroed out for the subsequent increments here
	;uint64_t cur{arr[i]};
	mov rax, [rcx+8*r10]

	;size_t cur0{cur & 0xFF};
	;uint64_t cur1{cur};
	;uint64_t cur2{cur};
	;uint64_t cur3{cur};
	;uint64_t cur4{cur};
	;uint64_t cur5{cur};
	;uint64_t cur6{cur};
	;buffer[i] = cur;// better to handle it from the start than to do an extra copy at the end, for when the swap count is odd
	;cur >>= 56;
	movzx esi, al
	mov ebp, eax
	mov ebx, eax
	mov r11, rax
	mov r12, rax
	mov r13, rax
	mov r14, rax
	mov [rdx+8*r10], rax
	shr rax, 56

	;++offsets[cur0];
	;size_t constexpr log2ptrs{static_cast<unsigned int>(std::bit_width(sizeof(void *) - 1))};
	;cur1 >>= 8 - log2ptrs;
	;cur2 >>= 16 - log2ptrs;
	;cur3 >>= 24 - log2ptrs;
	;cur4 >>= 32 - log2ptrs;
	;cur5 >>= 40 - log2ptrs;
	;cur6 >>= 48 - log2ptrs;
	;++offsets[7 * 256 + static_cast<size_t>(cur)];
	inc qword ptr [rsp+8*rsi]
	shr ebp, 8 - 3
	shr ebx, 16 - 3
	shr r11, 24 - 3
	shr r12, 32 - 3
	shr r13, 40 - 3
	shr r14, 48 - 3
	inc qword ptr [rsp+8*rax+7*8*256]

	;cur1 &= sizeof(void *) * 0xFF;
	;cur2 &= sizeof(void *) * 0xFF;
	;cur3 &= sizeof(void *) * 0xFF;
	;cur4 &= sizeof(void *) * 0xFF;
	;cur5 &= sizeof(void *) * 0xFF;
	;cur6 &= sizeof(void *) * 0xFF;
	and ebp, edi
	and ebx, edi
	and r11d, edi
	and r12d, edi
	and r13d, edi
	and r14d, edi

	;++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets + 256) + static_cast<size_t>(cur1));
	;++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets + 2 * 256) + static_cast<size_t>(cur2));
	;++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets + 3 * 256) + static_cast<size_t>(cur3));
	;++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets + 4 * 256) + static_cast<size_t>(cur4));
	;++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets + 5 * 256) + static_cast<size_t>(cur5));
	;++*reinterpret_cast<size_t *>(reinterpret_cast<uint8_t *>(offsets + 6 * 256) + static_cast<size_t>(cur6));
	;}while(0 <= --i);
	inc qword ptr [rsp+rbp+8*256]
	inc qword ptr [rsp+rbx+2*8*256]
	inc qword ptr [rsp+r11+3*8*256]
	inc qword ptr [rsp+r12+4*8*256]
	inc qword ptr [rsp+r13+5*8*256]
	inc qword ptr [rsp+r14+6*8*256]
	dec r10; possible macro-op fusion of dec r and jns

	jns countingloop

	;// transform counts into base offsets for each set of 256 items
	;size_t *t{offsets};
	;paritybool = false;// for when the swap count is odd
	;runsteps = (1 << sizeof(uint64_t)) - 1;
	;uint32_t k{};
	; paritybool is already assigned here: it's in r9b and can be false or true
	mov rdi, rsp; t
	mov ebp, 0FFh; runsteps
	lea r10, [r8-1]; countm1
	mov r11, rcx; arr
	xor ecx, ecx; k
	xor eax, eax; for breaking a false dependency on the next few boolean operations by sete al
	xor ebx, ebx; for breaking a false dependency on the next few boolean operations by sete bl

align 16; This is a reasonably rare jump target, but nops will only be executed once here. Put at least the first 3 instructions (14 bytes of bytecode in this case) in the instruction buffer at this point.
offsetloop:
	;do{
	;size_t offset{*t};
	;*t++ = 0;// the first offset always starts at zero
	mov r12, [rdi]; offset
	mov qword ptr [rdi], 0
	add rdi, 8

	;bool b{offset == count};
	;uint32_t j{256 - 2};
	cmp r12, r8
	mov esi, 256-2; j

	sete al; b
align 16; This is a reasonably common jump target, and nops will only be executed 8 times here. Put at least the first 3 instructions (11 bytes of bytecode in this case) in the instruction buffer at this point.
offsetloopinner:
	;do{
	;size_t addend{*t};
	;*t = offset;
	;t[8 * 256 - 1] = offset - 1;// high half
	mov r13, [rdi]; addend
	lea r14, [r12-1]
	mov [rdi], r12

	;offset += addend;
	;b |= addend == count;
	add r12, r13
	cmp r13, r8
	mov [rdi+3FF8h], r14

	;++t;
	;}while(--j);
	sete bl
	add rdi, 8

	or eax, ebx
	dec esi; possible macro-op fusion of dec r and jnz

	jnz offsetloopinner

	;b |= *t == count;
	;t[8 * 256] = countm1;// high half, the last offset always starts at the end
	;*t = offset;
	;t[8 * 256 - 1] = offset - 1;// high half
	cmp [rdi], r8
	mov [rdi], r12
	mov [rdi+4000h], r10

	sete bl
	dec r12

	;++t;
	or eax, ebx
	mov [rdi+3FF8h], r12
	add rdi, 8

	;paritybool ^= b;
	;runsteps ^= static_cast<uint8_t>(b) << k;
	;}while(8 > ++k);
	;}
	xor r9d, eax
	shl eax, cl
	inc ecx

	xor ebp, eax
	cmp ecx, 8; possible macro-op fusion of cmp r, imm8 and jb

	jb offsetloop

	;if(runsteps){// perform the bidirectional 8-bit sorting sequence
	test ebp, ebp; possible macro-op fusion of test r, r and jz

	jz exit

	;uint64_t *psrclo{arr}, *pdst{buffer};
	;if(paritybool){// swap if the count of sorting actions to do is odd
	;pdst = arr;
	;psrclo = buffer;
	;}
	; psrclo is already assigned here: it's in r11
	; pdst is already assigned here: it's in rdx
	;size_t *poffset{offsets};
	;uint32_t initialindex{
	;_tzcnt_u32(runsteps)// will run bsf (bit scan forward) on older architectures, which is fine
	;};// at least 1 bit is set inside runsteps as by previous check
	tzcnt ecx, ebp; initialindex
	test r9b, r9b; r9, bool input argument: according to the ABI you have to ignore the upper bytes, but the lower byte must be either 0 or 1
	mov r14, r11
	mov rsi, rsp; poffset

	cmovnz r11, rdx
	cmovnz rdx, r14
	;runsteps >>= initialindex;// skip a step if possible
	;uint32_t bitselect{initialindex * 8};
	;poffset += static_cast<size_t>(initialindex) * 256;
	;uint64_t *psrchi{psrclo + countm1};
	;uint64_t *pdstnext{psrclo};// for the next iteration
	mov eax, ecx
	shr ebp, cl
	shl ecx, 3

	shl eax, 8+3; initialindex*8*256
	lea r12, [r11+8*r10]; psrchi
	mov r8, r11; pdstnext

	add rsi, rax
	; register layout at this point
	; rax: free
	; rcx: bitselect
	; rdx: pdst
	; rbx: free
	; rbp: runsteps
	; rsi: poffset
	; rdi: free
	;  r8: pdstnext
	;  r9: free
	; r10: countm1
	; r11: psrclo
	; r12: psrchi
	; r13: free
	; r14: free
	; r15: occupied
	;if(countm1 & 1) for(;;){// main loop for even counts
	test r10, 1; possible macro-op fusion of test r, imm8 and jnz

	jz mainloopodd

align 32; Using instruction buffer block size-aligned innermost loop jump targets is a common optimization. The alignment here is this high because the entire loop is definitely more than 16 bytes of bytecode.
mainloopeven:
	;do{// fill the array, two at a time: one low-to-middle, one high-to-middle
	;uint64_t curlo{*psrclo++};
	;uint64_t curhi{*psrchi--};
	mov rax, [r11]; sello
	mov rbx, [r12]; selhi
	mov edi, 1
	or r9, -1; set all bits, shorter opcode than the direct move

	;size_t sello{curlo >> bitselect & 0xFF};
	;size_t selhi{curhi >> bitselect & 0xFF};
	mov r13, rax; curlo
	shr rax, cl
	mov r14, rbx; curhi
	shr rbx, cl

	movzx eax, al
	movzx ebx, bl
	add r11, 8

	;size_t offsetlo{poffset[sello]++};// the next item will be placed one higher
	;size_t offsethi{poffset[8 * 256 + selhi]--};// the next item will be placed one lower
	xadd [rsi+rax*8], rdi; offsetlo
	xadd [rsi+rbx*8+4000h], r9; offsethi
	sub r12, 8

	;pdst[offsetlo] = curlo;
	;pdst[offsethi] = curhi;
	;}while(psrclo < psrchi);
	mov [rdx+rdi*8], r13
	mov [rdx+r9*8], r14
	cmp r11, r12; possible macro-op fusion of cmp r, r and jb

	jb mainloopeven

	;runsteps >>= 1;
	shr ebp, 1

	;if(!runsteps) break;
	jz exit

	;uint32_t index{
	;_tzcnt_u32(runsteps)// will run bsf (bit scan forward) on older architectures, which is fine
	;};// at least 1 bit is set inside runsteps as by previous check
	;bitselect += 8;
	;poffset += 256;
	lea eax, [rcx+8]
	tzcnt ecx, ebp; index
	add rsi, 8*256

	;runsteps >>= index;// skip a step if possible
	;bitselect += index * 8;
	mov ebx, ecx
	shr ebp, cl
	lea ecx, [rax+8*rcx]

	;poffset += static_cast<size_t>(index) * 256;
	;// swap the pointers for the next round, data is moved on each iteration
	;psrclo = pdst;
	;psrchi = pdst + countm1;
	;pdst = pdstnext;
	;pdstnext = psrclo;
	;}
	shl ebx, 8+3; index*8*256
	mov r11, rdx
	lea r12, [rdx+8*r10]
	mov rdx, r8

	add rsi, rbx
	mov r8, r11
	jmp mainloopeven
	;else for(;;){// main loop for odd counts
align 32; Using instruction buffer block size-aligned innermost loop jump targets is a common optimization. The alignment here is this high because the entire loop is definitely more than 16 bytes of bytecode.
mainloopodd:
	;do{// fill the array, two at a time: one low-to-middle, one high-to-middle
	;uint64_t curlo{*psrclo++};
	;uint64_t curhi{*psrchi--};
	mov rax, [r11]; sello
	mov rbx, [r12]; selhi
	mov edi, 1
	or r9, -1; set all bits, shorter opcode than the direct move

	;size_t sello{curlo >> bitselect & 0xFF};
	;size_t selhi{curhi >> bitselect & 0xFF};
	mov r13, rax; curlo
	shr rax, cl
	mov r14, rbx; curhi
	shr rbx, cl

	movzx eax, al
	movzx ebx, bl
	add r11, 8

	;size_t offsetlo{poffset[sello]++};// the next item will be placed one higher
	;size_t offsethi{poffset[8 * 256 + selhi]--};// the next item will be placed one lower
	xadd [rsi+rax*8], rdi; offsetlo
	xadd [rsi+rbx*8+4000h], r9; offsethi
	sub r12, 8

	;pdst[offsetlo] = curlo;
	;pdst[offsethi] = curhi;
	;}while(psrclo < psrchi);
	mov [rdx+rdi*8], r13
	mov [rdx+r9*8], r14
	cmp r11, r12; possible macro-op fusion of cmp r, r and jb

	jne mainloopodd

	;// fill in the final item
	;uint64_t curlo{*psrclo};
	mov rax, [r11]; sello

	;size_t sello{curlo >> bitselect & 0xFF};
	;size_t offsetlo{poffset[sello]};
	;pdst[offsetlo] = curlo;
	;runsteps >>= 1;
	mov r13, rax; curlo
	shr rax, cl

	movzx eax, al
	shr ebp, 1

	mov rbx, [rsi+rax*8]; offsetlo

	mov [rdx+rbx*8], r13
	;if(!runsteps) break;
	jz exit

	;uint32_t index{
	;_tzcnt_u32(runsteps)// will run bsf (bit scan forward) on older architectures, which is fine
	;};// at least 1 bit is set inside runsteps as by previous check
	;bitselect += 8;
	;poffset += 256;
	lea eax, [rcx+8]
	tzcnt ecx, ebp; index
	add rsi, 8*256

	;runsteps >>= index;// skip a step if possible
	;bitselect += index * 8;
	mov ebx, ecx
	shr ebp, cl
	lea ecx, [rax+8*rcx]

	;poffset += static_cast<size_t>(index) * 256;
	;// swap the pointers for the next round, data is moved on each iteration
	;psrclo = pdst;
	;psrchi = pdst + countm1;
	;pdst = pdstnext;
	;pdstnext = psrclo;
	;}
	shl ebx, 8+3; initialindex*8*256
	mov r11, rdx
	lea r12, [rdx+8*r10]
	mov rdx, r8

	add rsi, rbx
	mov r8, r11
	jmp mainloopodd
	;}
align 32; This is a rare jump target, but nops will never be executed here. Put at least the first 3 instructions (17 bytes of bytecode in this case) in the instruction buffer at this point.
exit:; epilog
	mov r12, [rsp+8000h]
	mov r13, [rsp+8008h]
	mov r14, [rsp+8010h]
	add rsp, 3 * 8 + 2 * 8 * 8 * 256

	mov rbx, [rsp+8]
	mov rbp, [rsp+16]
	mov rsi, [rsp+24]
	mov rdi, [rsp+32]
	ret; callee stack cleanup (32 bytes were passed by register)
InnerRadixSortBidi8@@32 endp
_TEXT$ ends
end