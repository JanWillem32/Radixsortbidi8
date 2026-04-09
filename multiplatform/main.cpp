#include <iostream>
#include "../Radixsortbidi8.hpp"

int main(){
	std::uint32_t simple[]{18, 33, 21, 0, 5, 1, 13};
	bool succeeded{rsbd8::radixsort(_countof(simple), simple)};
	assert(succeeded);
	assert(0 == simple[0]);
	assert(1 == simple[1]);
	assert(5 == simple[2]);
	assert(13 == simple[3]);
	assert(18 == simple[4]);
	assert(21 == simple[5]);
	assert(33 == simple[6]);

	{// unit tests with the 3 simulated 80-bit long double types
		// direct sorting tests with the 80-bit long double types

		rsbd8::helper::longdoubletest80<false, true, true> aji80[7]{
			{0, 0xFFFFu},// -inf
			{0, 0x7FFFu},// +inf
			{0x8000000000000000u, 0xFFFFu},// QNaN, machine indeterminate
			{0xFFFFFFFFFFFFFFFFu, 0x7FFEu},// max normal
			{0, 1u},// min normal
			{0xFFFFFFFFFFFFFFFFu, 0},// max subnormal
			{1u, 0}};// min subnormal
		rsbd8::helper::longdoubletest80<false, true, true> ajo80[_countof(aji80)], ajb80[_countof(aji80)];
		rsbd8::radixsortcopynoalloc(_countof(aji80), aji80, ajo80, ajb80);
		assert(ajo80[0].mantissa == 0x8000000000000000u && ajo80[0].signexponent == 0xFFFFu);// QNaN, machine indeterminate
		assert(ajo80[1].mantissa == 0 && ajo80[1].signexponent == 0xFFFFu);// -inf
		assert(ajo80[2].mantissa == 1u && ajo80[2].signexponent == 0);// min subnormal
		assert(ajo80[3].mantissa == 0xFFFFFFFFFFFFFFFFu && ajo80[3].signexponent == 0);// max subnormal
		assert(ajo80[4].mantissa == 0 && ajo80[4].signexponent == 1u);// min normal
		assert(ajo80[5].mantissa == 0xFFFFFFFFFFFFFFFFu && ajo80[5].signexponent == 0x7FFEu);// max normal
		assert(ajo80[6].mantissa == 0 && ajo80[6].signexponent == 0x7FFFu);// +inf

		std::memset(ajb80, 0, sizeof(ajb80));
		rsbd8::radixsortnoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(aji80), ajo80, ajb80, true);
		assert(ajb80[0].mantissa == 0 && ajb80[0].signexponent == 0x7FFFu);// +inf
		assert(ajb80[1].mantissa == 0xFFFFFFFFFFFFFFFFu && ajb80[1].signexponent == 0x7FFEu);// max normal
		assert(ajb80[2].mantissa == 0 && ajb80[2].signexponent == 1u);// min normal
		assert(ajb80[3].mantissa == 0xFFFFFFFFFFFFFFFFu && ajb80[3].signexponent == 0);// max subnormal
		assert(ajb80[4].mantissa == 1u && ajb80[4].signexponent == 0);// min subnormal
		assert(ajb80[5].mantissa == 0 && ajb80[5].signexponent == 0xFFFFu);// -inf
		assert(ajb80[6].mantissa == 0x8000000000000000u && ajb80[6].signexponent == 0xFFFFu);// QNaN, machine indeterminate

		rsbd8::helper::longdoubletest96<false, true, true> aji96[7]{
			{0, 0xABABFFFFu},// -inf
			{0, 0xD0017FFFu},// +inf
			{0x8000000000000000u, 0xEEEEFFFFu},// QNaN, machine indeterminate
			{0xFFFFFFFFFFFFFFFFu, 0x01017FFEu},// max normal
			{0, 0xFFF80001u},// min normal
			{0xFFFFFFFFFFFFFFFFu, 0xC7C80000u},// max subnormal
			{1u, 0xB3710000u}};// min subnormal
		rsbd8::helper::longdoubletest96<false, true, true> ajo96[_countof(aji96)], ajb96[_countof(aji96)];
		rsbd8::radixsortcopynoalloc(_countof(aji96), aji96, ajo96, ajb96);
		assert(ajo96[0].mantissa == 0x8000000000000000u && ajo96[0].signexponent == 0xEEEEFFFFu);// QNaN, machine indeterminate
		assert(ajo96[1].mantissa == 0 && ajo96[1].signexponent == 0xABABFFFFu);// -inf
		assert(ajo96[2].mantissa == 1u && ajo96[2].signexponent == 0xB3710000u);// min subnormal
		assert(ajo96[3].mantissa == 0xFFFFFFFFFFFFFFFFu && ajo96[3].signexponent == 0xC7C80000u);// max subnormal
		assert(ajo96[4].mantissa == 0 && ajo96[4].signexponent == 0xFFF80001u);// min normal
		assert(ajo96[5].mantissa == 0xFFFFFFFFFFFFFFFFu && ajo96[5].signexponent == 0x01017FFEu);// max normal
		assert(ajo96[6].mantissa == 0 && ajo96[6].signexponent == 0xD0017FFFu);// +inf

		std::memset(ajb96, 0, sizeof(ajb96));
		rsbd8::radixsortnoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(aji96), ajo96, ajb96, true);
		assert(ajb96[0].mantissa == 0 && ajb96[0].signexponent == 0xD0017FFFu);// +inf
		assert(ajb96[1].mantissa == 0xFFFFFFFFFFFFFFFFu && ajb96[1].signexponent == 0x01017FFEu);// max normal
		assert(ajb96[2].mantissa == 0 && ajb96[2].signexponent == 0xFFF80001u);// min normal
		assert(ajb96[3].mantissa == 0xFFFFFFFFFFFFFFFFu && ajb96[3].signexponent == 0xC7C80000u);// max subnormal
		assert(ajb96[4].mantissa == 1u && ajb96[4].signexponent == 0xB3710000u);// min subnormal
		assert(ajb96[5].mantissa == 0 && ajb96[5].signexponent == 0xABABFFFFu);// -inf
		assert(ajb96[6].mantissa == 0x8000000000000000u && ajb96[6].signexponent == 0xEEEEFFFFu);// QNaN, machine indeterminate

		rsbd8::helper::longdoubletest128<false, true, true> aji128[7]{
			{0, 0xBBBBAAAAABABFFFFu},// -inf
			{0, 0x22221111D0017FFFu},// +inf
			{0x8000000000000000u, 0x44443333EEEEFFFFu},// QNaN, machine indeterminate
			{0xFFFFFFFFFFFFFFFFu, 0x6666555501017FFEu},// max normal
			{0, 0x88887777FFF80001u},// min normal
			{0xFFFFFFFFFFFFFFFFu, 0xCCCC9999C7C80000u},// max subnormal
			{1u, 0xFFFFDDDDB3710000u}};// min subnormal
		rsbd8::helper::longdoubletest128<false, true, true> ajo128[_countof(aji128)], ajb128[_countof(aji128)];
		rsbd8::radixsortcopynoalloc(_countof(aji128), aji128, ajo128, ajb128);
		assert(ajo128[0].mantissa == 0x8000000000000000u && ajo128[0].signexponent == 0x44443333EEEEFFFFu);// QNaN, machine indeterminate
		assert(ajo128[1].mantissa == 0 && ajo128[1].signexponent == 0xBBBBAAAAABABFFFFu);// -inf
		assert(ajo128[2].mantissa == 1u && ajo128[2].signexponent == 0xFFFFDDDDB3710000u);// min subnormal
		assert(ajo128[3].mantissa == 0xFFFFFFFFFFFFFFFFu && ajo128[3].signexponent == 0xCCCC9999C7C80000u);// max subnormal
		assert(ajo128[4].mantissa == 0 && ajo128[4].signexponent == 0x88887777FFF80001u);// min normal
		assert(ajo128[5].mantissa == 0xFFFFFFFFFFFFFFFFu && ajo128[5].signexponent == 0x6666555501017FFEu);// max normal
		assert(ajo128[6].mantissa == 0 && ajo128[6].signexponent == 0x22221111D0017FFFu);// +inf

		std::memset(ajb128, 0, sizeof(ajb128));
		rsbd8::radixsortnoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(aji128), ajo128, ajb128, true);
		assert(ajb128[0].mantissa == 0 && ajb128[0].signexponent == 0x22221111D0017FFFu);// +inf
		assert(ajb128[1].mantissa == 0xFFFFFFFFFFFFFFFFu && ajb128[1].signexponent == 0x6666555501017FFEu);// max normal
		assert(ajb128[2].mantissa == 0 && ajb128[2].signexponent == 0x88887777FFF80001u);// min normal
		assert(ajb128[3].mantissa == 0xFFFFFFFFFFFFFFFFu && ajb128[3].signexponent == 0xCCCC9999C7C80000u);// max subnormal
		assert(ajb128[4].mantissa == 1u && ajb128[4].signexponent == 0xFFFFDDDDB3710000u);// min subnormal
		assert(ajb128[5].mantissa == 0 && ajb128[5].signexponent == 0xBBBBAAAAABABFFFFu);// -inf
		assert(ajb128[6].mantissa == 0x8000000000000000u && ajb128[6].signexponent == 0x44443333EEEEFFFFu);// QNaN, machine indeterminate

		// basic indirect sorting tests with the 80-bit long double types

		rsbd8::helper::longdoubletest80<false, true, true> *ako80[_countof(aji80)], *akb80[_countof(aji80)], *aki80[_countof(aji80)]{
			aji80, aji80 + 1, aji80 + 2, aji80 + 3, aji80 + 4, aji80 + 5, aji80 + 6};// indirect input
		rsbd8::radixsortcopynoalloc(_countof(aki80), aki80, ako80, akb80);
		assert(ako80[0]->mantissa == 0x8000000000000000u && ako80[0]->signexponent == 0xFFFFu);// QNaN, machine indeterminate
		assert(ako80[1]->mantissa == 0 && ako80[1]->signexponent == 0xFFFFu);// -inf
		assert(ako80[2]->mantissa == 1u && ako80[2]->signexponent == 0);// min subnormal
		assert(ako80[3]->mantissa == 0xFFFFFFFFFFFFFFFFu && ako80[3]->signexponent == 0);// max subnormal
		assert(ako80[4]->mantissa == 0 && ako80[4]->signexponent == 1u);// min normal
		assert(ako80[5]->mantissa == 0xFFFFFFFFFFFFFFFFu && ako80[5]->signexponent == 0x7FFEu);// max normal
		assert(ako80[6]->mantissa == 0 && ako80[6]->signexponent == 0x7FFFu);// +inf

		std::memset(akb80, 0, sizeof(akb80));
		rsbd8::radixsortnoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(aki80), ako80, akb80, true);
		assert(akb80[0]->mantissa == 0 && akb80[0]->signexponent == 0x7FFFu);// +inf
		assert(akb80[1]->mantissa == 0xFFFFFFFFFFFFFFFFu && akb80[1]->signexponent == 0x7FFEu);// max normal
		assert(akb80[2]->mantissa == 0 && akb80[2]->signexponent == 1u);// min normal
		assert(akb80[3]->mantissa == 0xFFFFFFFFFFFFFFFFu && akb80[3]->signexponent == 0);// max subnormal
		assert(akb80[4]->mantissa == 1u && akb80[4]->signexponent == 0);// min subnormal
		assert(akb80[5]->mantissa == 0 && akb80[5]->signexponent == 0xFFFFu);// -inf
		assert(akb80[6]->mantissa == 0x8000000000000000u && akb80[6]->signexponent == 0xFFFFu);// QNaN, machine indeterminate

		rsbd8::helper::longdoubletest96<false, true, true> *ako96[_countof(aji96)], *akb96[_countof(aji96)], *aki96[_countof(aji96)]{
			aji96, aji96 + 1, aji96 + 2, aji96 + 3, aji96 + 4, aji96 + 5, aji96 + 6};// indirect input
		rsbd8::radixsortcopynoalloc(_countof(aki96), aki96, ako96, akb96);
		assert(ako96[0]->mantissa == 0x8000000000000000u && ako96[0]->signexponent == 0xEEEEFFFFu);// QNaN, machine indeterminate
		assert(ako96[1]->mantissa == 0 && ako96[1]->signexponent == 0xABABFFFFu);// -inf
		assert(ako96[2]->mantissa == 1u && ako96[2]->signexponent == 0xB3710000u);// min subnormal
		assert(ako96[3]->mantissa == 0xFFFFFFFFFFFFFFFFu && ako96[3]->signexponent == 0xC7C80000u);// max subnormal
		assert(ako96[4]->mantissa == 0 && ako96[4]->signexponent == 0xFFF80001u);// min normal
		assert(ako96[5]->mantissa == 0xFFFFFFFFFFFFFFFFu && ako96[5]->signexponent == 0x01017FFEu);// max normal
		assert(ako96[6]->mantissa == 0 && ako96[6]->signexponent == 0xD0017FFFu);// +inf

		std::memset(akb96, 0, sizeof(akb96));
		rsbd8::radixsortnoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(aki96), ako96, akb96, true);
		assert(akb96[0]->mantissa == 0 && akb96[0]->signexponent == 0xD0017FFFu);// +inf
		assert(akb96[1]->mantissa == 0xFFFFFFFFFFFFFFFFu && akb96[1]->signexponent == 0x01017FFEu);// max normal
		assert(akb96[2]->mantissa == 0 && akb96[2]->signexponent == 0xFFF80001u);// min normal
		assert(akb96[3]->mantissa == 0xFFFFFFFFFFFFFFFFu && akb96[3]->signexponent == 0xC7C80000u);// max subnormal
		assert(akb96[4]->mantissa == 1u && akb96[4]->signexponent == 0xB3710000u);// min subnormal
		assert(akb96[5]->mantissa == 0 && akb96[5]->signexponent == 0xABABFFFFu);// -inf
		assert(akb96[6]->mantissa == 0x8000000000000000u && akb96[6]->signexponent == 0xEEEEFFFFu);// QNaN, machine indeterminate

		rsbd8::helper::longdoubletest128<false, true, true> *ako128[_countof(aji128)], *akb128[_countof(aji128)], *aki128[_countof(aji128)]{
			aji128, aji128 + 1, aji128 + 2, aji128 + 3, aji128 + 4, aji128 + 5, aji128 + 6};// indirect input
		rsbd8::radixsortcopynoalloc(_countof(aki128), aki128, ako128, akb128);
		assert(ako128[0]->mantissa == 0x8000000000000000u && ako128[0]->signexponent == 0x44443333EEEEFFFFu);// QNaN, machine indeterminate
		assert(ako128[1]->mantissa == 0 && ako128[1]->signexponent == 0xBBBBAAAAABABFFFFu);// -inf
		assert(ako128[2]->mantissa == 1u && ako128[2]->signexponent == 0xFFFFDDDDB3710000u);// min subnormal
		assert(ako128[3]->mantissa == 0xFFFFFFFFFFFFFFFFu && ako128[3]->signexponent == 0xCCCC9999C7C80000u);// max subnormal
		assert(ako128[4]->mantissa == 0 && ako128[4]->signexponent == 0x88887777FFF80001u);// min normal
		assert(ako128[5]->mantissa == 0xFFFFFFFFFFFFFFFFu && ako128[5]->signexponent == 0x6666555501017FFEu);// max normal
		assert(ako128[6]->mantissa == 0 && ako128[6]->signexponent == 0x22221111D0017FFFu);// +inf

		std::memset(akb128, 0, sizeof(akb128));
		rsbd8::radixsortnoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(aki128), ako128, akb128, true);
		assert(akb128[0]->mantissa == 0 && akb128[0]->signexponent == 0x22221111D0017FFFu);// +inf
		assert(akb128[1]->mantissa == 0xFFFFFFFFFFFFFFFFu && akb128[1]->signexponent == 0x6666555501017FFEu);// max normal
		assert(akb128[2]->mantissa == 0 && akb128[2]->signexponent == 0x88887777FFF80001u);// min normal
		assert(akb128[3]->mantissa == 0xFFFFFFFFFFFFFFFFu && akb128[3]->signexponent == 0xCCCC9999C7C80000u);// max subnormal
		assert(akb128[4]->mantissa == 1u && akb128[4]->signexponent == 0xFFFFDDDDB3710000u);// min subnormal
		assert(akb128[5]->mantissa == 0 && akb128[5]->signexponent == 0xBBBBAAAAABABFFFFu);// -inf
		assert(akb128[6]->mantissa == 0x8000000000000000u && akb128[6]->signexponent == 0x44443333EEEEFFFFu);// QNaN, machine indeterminate
	}

	{// simple unit tests, mostly to track template compile-time issues
		// 2 unit tests: radixsortcopynoalloc(), single-byte enum, no indirection, (explicit template statement) descending and ascending
		enum cert_v_binencoding64 : std::uint8_t{// in groups of ten
			$0, $1, $2, $3, $4, $5, $6, $7, $8, $9,// 0
			$a, $b, $c, $d, $e, $f, $g, $h, $i, $j,// 10
			$k, $l, $m, $n, $o, $p, $q, $r, $s, $t,// 20
			$u, $v, $w, $x, $y, $z, $A, $B, $C, $D,// 30
			$E, $F, $G, $H, $I, $J, $K, $L, $M, $N,// 40
			$O, $P, $Q, $R, $S, $T, $U, $V, $W, $X,// 50
			$Y, $Z, $$, $_};// 60
		// test sequence 0B_iqUE (oblique), with one item from each row; 0:0, 10:i, 20:q, 30:B, 40:E, 50:U, 60:_
		static cert_v_binencoding64 constexpr tein[7]{$0, $B, $_, $i, $q, $U, $E};
		cert_v_binencoding64 teout[_countof(tein)];
		cert_v_binencoding64 tebuf[_countof(tein)];// dummy, as it's an 8-bit type
		rsbd8::radixsortcopynoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(tein), tein, teout, tebuf);
		assert(teout[0] == $_ && teout[1] == $U && teout[2] == $E && teout[3] == $B && teout[4] == $q && teout[5] == $i && teout[6] == $0);
		rsbd8::radixsortcopynoalloc<rsbd8::sortingdirection::ascfwdorder>(_countof(tein), tein, teout);
		assert(teout[0] == $0 && teout[1] == $i && teout[2] == $q && teout[3] == $B && teout[4] == $E && teout[5] == $U && teout[6] == $_);

		// 1 unit test: radixsortnoalloc(), write to buffer, float (multi-byte), no indirection, (implicit template statement) ascending
		std::uint32_t inm[7]{8, 0, 3, 1u << 31 | 2, 3, 1u << 31 | 18, 1u << 31 | 2};
		std::uint32_t outm[_countof(inm)];
		rsbd8::radixsortnoalloc(_countof(inm), reinterpret_cast<float *>(inm), reinterpret_cast<float *>(outm), true);
		assert(outm[0] == (1u << 31 | 18) && outm[1] == (1u << 31 | 2) && outm[2] == (1u << 31 | 2) && outm[3] == 0 && outm[4] == 3 && outm[5] == 3 && outm[6] == 8);

		// 2 unit test, the same as above, but indirect
		std::uint32_t const *inim[7]{outm + 6, outm + 3, outm + 4, outm + 1, outm + 5, outm, outm + 2};
		std::uint32_t const *outim[_countof(inim)];
		std::uint32_t const *bufim[_countof(inim)];
		rsbd8::radixsortcopynoalloc<rsbd8::sortingdirection::dscrevorder>(_countof(inim), reinterpret_cast<float const *const *>(inim), reinterpret_cast<float const **>(outim), reinterpret_cast<float const **>(bufim));
		assert(outim[0] == inim[0] && outim[1] == inim[4] && outim[2] == inim[2] && outim[3] == inim[1] && outim[4] == inim[6] && outim[5] == inim[3] && outim[6] == inim[5]);
		rsbd8::radixsortnoalloc(_countof(inim), reinterpret_cast<float const **>(inim), reinterpret_cast<float const **>(bufim), false);
		assert(*inim[0] == (1u << 31 | 18) && *inim[1] == (1u << 31 | 2) && *inim[2] == (1u << 31 | 2) && *inim[3] == 0 && *inim[4] == 3 && *inim[5] == 3 && *inim[6] == 8);

		// 6 groups of short unit tests: radixsortcopynoalloc() (and one directly to its implementation), 8-byte with first level getter indirection, (implicit template statement) ascending
		// Part of this test is firing up the debugger in "release mode" to see how well the inlining parallel processing fares, or just read the asm output functions directly. (This generates quite a few similar functions for the various cases though.)
#pragma pack(push, 1)
		class Testmeclass{
			std::uint64_t wasted{};// unused, default to zero for this test class
			char misalignoffset{};// unused, default to zero for this test class
		public:
			std::uint64_t co;
			std::int64_t sco;
			constexpr __forceinline std::uint64_t get()const noexcept{return{co};};
			constexpr __forceinline std::uint64_t getwparam(int)const noexcept{return{co};};
			constexpr __forceinline std::uint64_t bget()noexcept{return{co};};
			constexpr __forceinline std::int64_t sget()const noexcept{return{sco};};
			constexpr __forceinline std::int64_t zget()noexcept{return{sco};};
			constexpr __forceinline Testmeclass(std::uint64_t input)noexcept : co{input}, sco{static_cast<std::int64_t>(input) - 1} {};
		};
#pragma pack(pop)
		static std::size_t constexpr sizecontainer{sizeof(Testmeclass)};
		static_assert(25 == sizecontainer);
		static std::size_t constexpr offsetco{rsbd8::getoffsetof<&Testmeclass::co>};
		static_assert(9 == offsetco);
		static std::size_t constexpr offsetsco{rsbd8::getoffsetof<&Testmeclass::sco>};
		static_assert(17 == offsetsco);

		Testmeclass cin[]{8, 0, 6, 4, 0, 2, 6};
		Testmeclass const *fin[_countof(cin)]{cin, cin + 1, cin + 2, cin + 3, cin + 4, cin + 5, cin + 6};
		Testmeclass const *fout[_countof(cin)];
		Testmeclass const *fbuf[_countof(cin)];

		rsbd8::radixsortcopynoalloc<&Testmeclass::get>(_countof(fin), fin, fout, fbuf);
		rsbd8::radixsortcopynoalloc<&Testmeclass::getwparam>(_countof(fin), fin, fout, fbuf, 8);
		rsbd8::radixsortcopynoalloc<&Testmeclass::co>(_countof(fin), fin, fout, fbuf);
		rsbd8::radixsortcopynoalloc<std::uint64_t, 9>(_countof(fin), fin, fout, fbuf);

		rsbd8::radixsortcopynoalloc<&Testmeclass::sget>(_countof(fin), fin, fout, fbuf);
		rsbd8::radixsortcopynoalloc<&Testmeclass::sco>(_countof(fin), fin, fout, fbuf);
		rsbd8::radixsortcopynoalloc<std::int64_t, 17>(_countof(fin), fin, fout, fbuf);

		// correctly fails to compile (not const-correct):
		//rsbd8::radixsortcopynoalloc<&Testmeclass::bget>(_countof(fin), fin, fout, fbuf);
		// correctly fails to compile (not const-correct):
		//rsbd8::radixsortcopynoalloc<&Testmeclass::zget>(_countof(fin), fin, fout, fbuf);

		Testmeclass *yin[_countof(cin)]{cin, cin + 1, cin + 2, cin + 3, cin + 4, cin + 5, cin + 6};
		Testmeclass *yout[_countof(cin)];
		Testmeclass *ybuf[_countof(cin)];

		rsbd8::radixsortcopynoalloc<&Testmeclass::get>(_countof(yin), yin, yout, ybuf);
		rsbd8::radixsortcopynoalloc<&Testmeclass::getwparam>(_countof(yin), yin, yout, ybuf, 8);
		rsbd8::radixsortcopynoalloc<&Testmeclass::co>(_countof(yin), yin, yout, ybuf);
		rsbd8::radixsortcopynoalloc<std::uint64_t, 9>(_countof(yin), yin, yout, ybuf);

		rsbd8::radixsortcopynoalloc<&Testmeclass::sget>(_countof(yin), yin, yout, ybuf);
		rsbd8::radixsortcopynoalloc<&Testmeclass::sco>(_countof(yin), yin, yout, ybuf);
		rsbd8::radixsortcopynoalloc<std::int64_t, 17>(_countof(yin), yin, yout, ybuf);

		// unlike the commented calls above, these work as intended:
		rsbd8::radixsortcopynoalloc<&Testmeclass::bget>(_countof(yin), yin, yout, ybuf);
		rsbd8::radixsortcopynoalloc<&Testmeclass::zget>(_countof(yin), yin, yout, ybuf);

		// TODO: add more unit tests
	}

	return{0};
}