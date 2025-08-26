#include "stdafx.h"

extern __declspec(noalias safebuffers noinline) uint64_t ConvertPerfCounterTo100ns(uint64_t timerunits){
	// compute timerunits * 10000000
#ifdef _WIN64
	uint64_t highn, lown{_umul128(10000000ui64, timerunits, &highn)};
#else
	uint32_t timerunitslo{static_cast<uint32_t>(timerunits)}, timerunitshi{static_cast<uint32_t>(timerunits >> 32)};
	// do limited long multiplication
	// this next computation cannot overflow because the maximum value we can compute here is 10000000 * FFFFFFFFFFFFFFFFh = 184467440737095516150000000 = 98967FFFFFFFFFFF676980h, which takes 88 bits (11 bytes)
	// we will use 3 registers of 32 bits (12 bytes) in these calculations and imagine a fourth register (the top 32 bits) set to zero
	uint64_t i0{__emulu(10000000ui32, timerunitslo)};
	uint32_t lownlo{static_cast<uint32_t>(i0)}, lownhi{static_cast<uint32_t>(i0 >> 32)};
	uint64_t i1{__emulu(10000000ui32, timerunitshi)};
	uint32_t highnlo{static_cast<uint32_t>(i1 >> 32)};
	unsigned char EmptyCarry{_addcarry_u32(_addcarry_u32(0, lownhi, static_cast<uint32_t>(i1), &lownhi), highnlo, 0ui32, &highnlo)};
	assert(!EmptyCarry);// the last add-with-carry cannot carry out
	static_cast<void>(EmptyCarry);
	uint32_t highnhi{};
#endif

	// compute timerunits * 10000000 / frequency
#ifdef _WIN64
	// 128-by-64-to-64-bit unsigned division by invariant integers using multiplication
	uint8_t Nml{gk_QPFDivisorFactors.m_Nml};
	uint64_t n_2{__shiftleft128(lown, highn, Nml)};
	uint64_t n_10{lown << Nml};
	uint64_t d_norm{gk_QPFDivisorFactors.m_d_norm};
	uint64_t mn_1{static_cast<uint64_t>(static_cast<int64_t>(n_10) >> (64 - 1))};
	//uint128_t n_adj{static_cast<uint128_t>(n_10) + (static_cast<uint128_t>(mn_1 & d_norm) | static_cast<uint128_t>(mn_1) << 64)}; unlike what is stated in the paper, n_adj needs to be double the word size
	uint64_t n_adj0{mn_1 & d_norm}, n_adj1;// unlike what is stated in the paper, n_adj needs to be double the word size
	_addcarry_u64(_addcarry_u64(0, n_adj0, n_10, &n_adj0), mn_1, 0ui64, &n_adj1);// ignore the resulting carry out
	//uint64_t q_1{n_2 + static_cast<uint64_t>((__emulu128(mprime, n_2 - mn_1) + n_adj) >> 64)};
	uint64_t q_1, q_1i{_umul128(gk_QPFDivisorFactors.m_mprime, n_2 - mn_1, &q_1)};
	_addcarry_u64(_addcarry_u64(0, q_1i, n_adj0, &q_1i), q_1, n_adj1, &q_1);// ignore the resulting carry out, discard q_1i
	q_1 += n_2;
	//uint128_t dr{n - (static_cast<uint128_t>(d_norm) << 64) + __emulu128(~q_1, d_norm)};
	//uint64_t lowdr{static_cast<uint64_t>(dr), highdr{static_cast<uint64_t>(dr >> 64)};
	uint64_t highdri, lowdri{_umul128(~q_1, d_norm, &highdri)};
	highdri -= d_norm;
	uint64_t lowdr, highdr;
	_addcarry_u64(_addcarry_u64(0, lowdri, n_10, &lowdr), highdri, n_2, &highdr);// ignore the resulting carry out
	uint64_t q{highdr + 1ui64 + q_1 /*- 0x10000000000000000ui128 + 0x10000000000000000ui128*/};// - 0x10000000000000000ui128 + 0x10000000000000000ui128 would do nothing
	//uint64_t r{(lowdr + (d_norm /*- 0x10000000000000000ui128*/ & highdr)) >> Nml}; - 0x10000000000000000ui128 would do nothing
#else
	//uint64_t n_normhi{__shiftleft128(lown, highn, Nml)};
	//uint64_t n_normlo{lown << Nml};
	// the 128-bit value is shifted up by 32 bits if Nml is large
	uint8_t Nml{gk_QPFDivisorFactors.m_Nml};
	if(Nml & 32){
		highnhi = highnlo;
		highnlo = lownhi;
		lownhi = lownlo;
		lownlo = 0ui32;
	}
	uint32_t alowni[2]{lownlo, lownhi}; uint64_t lowni{*reinterpret_cast<uint64_t UNALIGNED *>(alowni)};// recompose
	uint32_t amiddleni[2]{lownhi, highnlo}; uint64_t middleni{*reinterpret_cast<uint64_t UNALIGNED *>(amiddleni)};// recompose
	uint32_t ahighni[2]{highnlo, highnhi}; uint64_t highni{*reinterpret_cast<uint64_t UNALIGNED *>(ahighni)};// recompose
	// __ll_lshift(), __ull_rshift() and __ll_rshift() on x86-32 cannot shift more than 31 bits, exactly the same way as the underlying shld (top left), shl/sal (bottom left), shrd (bottom right), shr (top right, unsigned only), sar (top right, signed only) instructions work, so do not replace these with the generic <<=, <<, >>= or >> operands for the next statements, as these use heuristics for multi-instruction variable amount shifts of up to 64 bits
	uint32_t n3{static_cast<uint32_t>(__ll_lshift(highni,  static_cast<int>(Nml)) >> 32)};
	uint32_t n2{static_cast<uint32_t>(__ll_lshift(middleni, static_cast<int>(Nml)) >> 32)};
	uint64_t ntmp{__ll_lshift(lowni, static_cast<int>(Nml))};
	uint32_t n0{static_cast<uint32_t>(ntmp)}, n1{static_cast<uint32_t>(ntmp >> 32)};
	// divide normalized parts of the dividend n3, n2 and n1 by using the normalized reciprocal divisor v
	// 96-by-64-bit unsigned division by invariant integers using multiplication
	uint32_t v{gk_QPFDivisorFactors.m_v}, d_normlo{gk_QPFDivisorFactors.m_d_normlo}, d_normhi{gk_QPFDivisorFactors.m_d_normhi};
	uint64_t qrihi{__emulu(n3, v)};
	uint32_t rihi{static_cast<uint32_t>(qrihi)};
	uint32_t qhi{static_cast<uint32_t>(qrihi >> 32)};
	_addcarry_u32(_addcarry_u32(0, rihi, n2, &rihi), qhi, n3, &qhi);// ignore the resulting carry out
	n2 -= qhi * d_normhi;// multiply-reverse subtract to get the highest word of the unadjusted remainder
	uint64_t thi{__emulu(qhi, d_normlo)};
	_subborrow_u32(_subborrow_u32(0, n1, d_normlo, &n1), n2, d_normhi, &n2);// ignore the resulting carry out
	_subborrow_u32(_subborrow_u32(0, n1, static_cast<uint32_t>(thi), &n1), n2, static_cast<uint32_t>(thi >> 32), &n2);// ignore the resulting carry out
	qhi += 2ui32;// this is allowed to overflow
	uint32_t mask2;// rhighest >= rihi (implemented as !(rhighest < rihi)), unpredictable condition, set the mask if true
	_addcarry_u32(_subborrow_u32(0, n2, rihi, nullptr), 0ui32, 0xFFFFFFFFui32, &mask2);// ignore the resulting carry out, assembly checked: the compiler allows nullptr on this parameter without faulting at compile- or run-time
	qhi += mask2;
	_addcarry_u32(_addcarry_u32(0, n1, d_normlo & mask2, &n1), n2, d_normhi & mask2, &n2);// ignore the resulting carry out
	uint32_t mask3;// r < d_norm, likely condition, set the mask if true, using only the carry out in this particular case is possible
	_subborrow_u32(_subborrow_u32(_subborrow_u32(0, n1, d_normlo, &n1), n2, d_normhi, &n2), mask3, mask3, &mask3);// ignore the resulting carry out
	qhi += mask3;
	_addcarry_u32(_addcarry_u32(0, n1, d_normlo & mask3, &n1), n2, d_normhi & mask3, &n2);// ignore the resulting carry out
	// divide normalized parts of the dividend n2, n1 and n0 by using the normalized reciprocal divisor v
	// 96-by-64-bit unsigned division by invariant integers using multiplication
	uint64_t qrilo{__emulu(n2, v)};
	uint32_t rilo{static_cast<uint32_t>(qrilo)};
	uint32_t qlo{static_cast<uint32_t>(qrilo >> 32)};
	_addcarry_u32(_addcarry_u32(0, rilo, n1, &rilo), qlo, n2, &qlo);// ignore the resulting carry out
	n1 -= qlo * d_normhi;// multiply-reverse subtract to get the highest word of the unadjusted remainder
	uint64_t tlo{__emulu(qlo, d_normlo)};
	_subborrow_u32(_subborrow_u32(0, n0, d_normlo, &n0), n1, d_normhi, &n1);// ignore the resulting carry out
	_subborrow_u32(_subborrow_u32(0, n0, static_cast<uint32_t>(tlo), &n0), n1, static_cast<uint32_t>(tlo >> 32), &n1);// ignore the resulting carry out
	qlo += 2ui32;// this is allowed to overflow
	uint32_t mask4;// rhighest >= rilo (implemented as !(rhighest < rilo)), unpredictable condition, set the mask if true
	_addcarry_u32(_subborrow_u32(0, n1, rilo, nullptr), 0ui32, 0xFFFFFFFFui32, &mask4);// ignore the resulting carry out, assembly checked: the compiler allows nullptr on this parameter without faulting at compile- or run-time
	qlo += mask4;
	_addcarry_u32(_addcarry_u32(0, n0, d_normlo & mask4, &n0), n1, d_normhi & mask4, &n1);// ignore the resulting carry out
	uint32_t mask5;// r < d_norm, likely condition, set the mask if true, using only the carry out in this particular case is possible
	_subborrow_u32(_subborrow_u32(_subborrow_u32(0, n0, d_normlo, &n0), n1, d_normhi, &n1), mask5, mask5, &mask5);// ignore the resulting carry out
	qlo += mask5;
	//_addcarry_u32(_addcarry_u32(0, n0, d_normlo & mask5, &n0), n1, d_normhi & mask5, &n1); ignore the resulting carry out
	uint32_t aq[2]{qlo, qhi}; uint64_t q{*reinterpret_cast<uint64_t UNALIGNED *>(aq)};// recompose
	// the 64-bit value is shifted down by 32 bits if Nml is large
	//if(Nml & 32){
	//n0 = n1;
	//n1 = 0ui32;
	//}
	//uint32_t ar[2]{n0, n1}; uint64_t r{*reinterpret_cast<uint64_t UNALIGNED *>(ar)}; recompose
	//r = __ull_rshift(r, static_cast<int>(Nml));
#endif
	return{q};
}