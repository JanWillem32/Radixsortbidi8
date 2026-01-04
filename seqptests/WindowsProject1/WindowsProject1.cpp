// MIT License
// Copyright (c) 2025-2026 Jan-Willem Krans (janwillem32 <at> hotmail <dot> com)
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// WindowsProject1.cpp : Defines the entry point for the application.

#include "pch.h"
#include "..\..\Radixsortbidi8.hpp"
#include <Aclapi.h>
#include <cmath>
#include <algorithm>// just for std::sort() (wich strictly doesn't do the same) and std::stable_sort()

// disable warning messages for this file only:
// C4559: 'x': redefinition; the function gains __declspec(noalias)
// C4711: function 'x' selected for automatic inline expansion
#pragma warning(disable: 4559 4711)

#ifdef _WIN64
__declspec(noalias safebuffers) wchar_t *WritePaddedu64(wchar_t *pwcOut, std::uint64_t n)noexcept{
	// 18446744073709551615 is the maximum output by this function, the output is padded on the left with spaces if required to get to 20 characters
	static std::uint64_t constexpr fourspaces{static_cast<std::uint64_t>(L' ') << 48 | static_cast<std::uint64_t>(L' ') << 32 | static_cast<std::uint64_t>(L' ') << 16 | static_cast<std::uint64_t>(L' ')};
	reinterpret_cast<std::uint64_t UNALIGNED *>(pwcOut)[0] = fourspaces;
	reinterpret_cast<std::uint64_t UNALIGNED *>(pwcOut)[1] = fourspaces;
	reinterpret_cast<std::uint64_t UNALIGNED *>(pwcOut)[2] = fourspaces;
	reinterpret_cast<std::uint64_t UNALIGNED *>(pwcOut)[3] = fourspaces;
	reinterpret_cast<std::uint64_t UNALIGNED *>(pwcOut)[4] = fourspaces;// writes one wchar_t more than required, but it gets overwritten by the first output in the loop anyway
	pwcOut += 19;// make it point at the least significand digit first
	// original code:
	//do{
	//*pwcOut-- = static_cast<wchar_t>(static_cast<std::uint32_t>(n % 10ui64) + L'0'); the last digit is never substituted with a space
	//n /= 10ui64;
	//}while(n);
	// assembly checked: the x64 compiler eliminates the need for the div instruction here, the x86-32 compiler isn't able to optimize this routine.
	// 64-bit builds do use the 64-bit unsigned division by invariant integers using multiplication method, but the compiler is only capable of selecting the most basic variant of its methods, and not the simplified versions.
	// 32-bit builds use the 64-bit division support library (__aulldvrm(), __aulldiv(), __alldvrm() and __alldiv()) leaving the modulo and division operations completely unoptimized.
#ifdef _WIN64
	if(n > 9ui64){
		std::uint64_t q;
		do{
			// 64-bit unsigned division by invariant integers using multiplication
			// n / 10
			static std::uint64_t constexpr mprime{0xCCCCCCCCCCCCCCCDui64}, d{0x000000000000000Aui64};
			static std::uint8_t constexpr sh_post{3ui8};
			q = __umulh(n, mprime) >> sh_post;
			// n % 10
			// there is no trivially computable remainder for this method
			// given that we actually only need the bottom 4 bits here (the remainder is 4 bits: ceil(log2(divisor - 1 + 1))), a little optimization is allowed
			//std::uint32_t r{static_cast<std::uint32_t>(n) - static_cast<std::uint32_t>(q) * static_cast<std::uint32_t>(d)};
			//*pwcOut-- = static_cast<wchar_t>(r + L'0');
			std::uint32_t rm{static_cast<std::uint32_t>(q) * 4ui32 + static_cast<std::uint32_t>(q)};// lea, q * 5
			rm = rm * 2ui32 - L'0';// lea, q * 10 - L'0'
			*pwcOut-- = static_cast<wchar_t>(static_cast<std::uint32_t>(n) - rm);// sub, n - (q * 10 - L'0')
			n = q;
		}while(q > 9ui64);
	}
	*pwcOut = static_cast<wchar_t>(static_cast<std::uint32_t>(n) + L'0');// the last digit is never substituted with a space
#else
	std::uint32_t nlo{static_cast<std::uint32_t>(n)}, nhi{static_cast<std::uint32_t>(n >> 32)};
	if(nhi){// at most ten iterations of division by ten are needed to reduce the size of the dividend from a 64-bit unsigned integer to a 32-bit unsigned integer
		std::uint32_t qhi;
		do{
			// 64-bit unsigned division by invariant integers using multiplication
			// n / 10
			static std::uint8_t constexpr sh_post{3ui8};
			static std::uint64_t constexpr mprime{0xCCCCCCCCCCCCCCCDui64}, d{0x000000000000000Aui64};
			//std::uint64_t q{__umulh(n, mprime) >> sh_post};
			// given that the top and bottom half of mprime only differ by one, a little optimization is allowed
			std::uint32_t constexpr mprimehi{static_cast<std::uint32_t>(mprime >> 32)};
			static_assert(static_cast<std::uint32_t>(mprime & 0x00000000FFFFFFFFui64) == mprimehi + 1ui32);
			// unsigned 64-by-64-to-128-bit multiply
			std::uint64_t j2{__emulu(nlo, mprimehi)};
			std::uint32_t n1a{static_cast<std::uint32_t>(j2)}, n2{static_cast<std::uint32_t>(j2 >> 32)};
			// calculate __emulu(nlo, mprimehi + 1ui32) and __emulu(nhi, mprimehi + 1ui32) as __emulu(nlo, mprimehi) + nlo and __emulu(nhi, mprimehi) + nhi
			// sum table, from least to most significant word
			// nlo nhi
			// n1a n2
			//     n1a n2
			//     n2a n3
			//         n2a n3
			assert(n2 < 0xFFFFFFFFui32);// even __emulu(0xFFFFFFFFui32, 0xFFFFFFFFui32) will only output 0xFFFFFFFE00000001ui64, so adding the carry to just n2 is not a problem here
			std::uint32_t n0, n1;
			unsigned char EmptyCarry0{_addcarry_u32(_addcarry_u32(_addcarry_u32(0, nlo, n1a, &n0), nhi, n2, &n1), n2, 0ui32, &n2)};// add nlo and nhi to the earlier computed parts
			assert(!EmptyCarry0);// the last add-with-carry cannot carry out
			static_cast<void>(EmptyCarry0);
			static_cast<void>(n0);// the lowest part is discarded, as it cannot carry out when it has no further addend
			std::uint64_t j3{__emulu(nhi, mprimehi)};
			std::uint32_t n2a{static_cast<std::uint32_t>(j3)}, n3{static_cast<std::uint32_t>(j3 >> 32)};
			unsigned char EmptyCarry1{_addcarry_u32(_addcarry_u32(_addcarry_u32(0, n1, n1a, &n1), n2, n3, &n2), n3, 0ui32, &n3)};
			assert(!EmptyCarry1);// the last add-with-carry cannot carry out
			static_cast<void>(EmptyCarry1);
			unsigned char EmptyCarry2{_addcarry_u32(_addcarry_u32(_addcarry_u32(0, n1, n2a, &n1), n2, n2a, &n2), n3, 0ui32, &n3)};
			assert(!EmptyCarry2);// the last add-with-carry cannot carry out
			static_cast<void>(EmptyCarry2);
			// n0 and n1 are discarded
			std::uint32_t at_1[2]{n2, n3}; std::uint64_t t_1{*reinterpret_cast<std::uint64_t UNALIGNED *>(at_1)};// recompose
			std::uint64_t q{t_1 >> sh_post};
			std::uint32_t qlo{static_cast<std::uint32_t>(q)};
			qhi = static_cast<std::uint32_t>(q >> 32);
			// n % 10
			// there is no trivially computable remainder for this method
			// given that we actually only need the bottom 4 bits here (the remainder is 4 bits: ceil(log2(divisor - 1 + 1))), a little optimization is allowed
			//std::uint32_t r{nlo - qlo * static_cast<std::uint32_t>(d)};
			//*pwcOut-- = static_cast<wchar_t>(r + L'0');
			std::uint32_t rm{qlo * 4ui32 + qlo};// lea, q * 5
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
		std::uint32_t q;
		do{
			// 32-bit unsigned division by invariant integers using multiplication
			// n / 10
			static std::uint8_t constexpr sh_post{3ui8};
			static std::uint32_t constexpr mprime{0xCCCCCCCDui32}, d{0x0000000Aui32};
			q = static_cast<std::uint32_t>(__emulu(nlo, mprime) >> 32) >> sh_post;
			// n % 10
			// there is no trivially computable remainder for this method
			//std::uint32_t r{nlo - q * d};
			//*pwcOut-- = static_cast<wchar_t>(r + L'0');
			std::uint32_t rm{q * 4ui32 + q};// lea, q * 5
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
extern "C" __declspec(noalias safebuffers) wchar_t *__vectorcall WritePaddedu64ra(wchar_t *pwcOut, std::uint32_t nhi, __m128i nlo)noexcept;// .asm file
__declspec(noalias safebuffers) __forceinline wchar_t *WritePaddedu64(wchar_t *pwcOut, std::uint64_t n)noexcept{return{WritePaddedu64ra(pwcOut, static_cast<std::uint32_t>(n >> 32), _mm_cvtsi32_si128(static_cast<std::int32_t>(n)))};}
#endif

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
__declspec(noalias safebuffers naked) std::uint16_t __vectorcall Getx87StatusAndControlWords(std::uint16_t &ControlWord)noexcept{
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
		std::uintptr_t pProcessEnvironmentBlock{__readgsqword(0x60)};
#else
		std::uintptr_t pProcessEnvironmentBlock{__readfsdword(0x30)};
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
		UNICODE_STRING *pDesktopInfo{reinterpret_cast<UNICODE_STRING *>(reinterpret_cast<std::uintptr_t>(pUPP) + 0xC0)};
#else
		UNICODE_STRING *pDesktopInfo{reinterpret_cast<UNICODE_STRING *>(reinterpret_cast<std::uintptr_t>(pUPP) + 0x78)};
#endif
		assert(pDesktopInfo->Buffer);// can point to an empty string if simply unnamed
		//assert(pDesktopInfo->Length); empty string is legal
		//assert(pDesktopInfo->MaximumLength); empty string is legal

		// floating point environment intitialization verification
#ifdef _M_IX86
		// verify the initial x87 status and control word values
		std::uint16_t ControlWord;
		std::uint16_t StatusWord{Getx87StatusAndControlWords(ControlWord)};
		assert(!(StatusWord & ((1 << 7) | (1 << 6) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 1)));// The Interrupt Request, Stack Fault, Precision, Underflow, Overflow, Zero divide, Denormalized and Invalid operation exception flags should not be set.
		ControlWord &= ~((1 << 15) | (1 << 14) | (1 << 13) | (1 << 12) | (1 << 7) | (1 << 6));// mask out the unused bits
		assert(ControlWord == ((1 << 9) | (1 << 5) | (1 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | 1));// The Rounding Control should be set to nound to nearest or to even if equidistant, the Precision Control should be set to 53 bits (IEEE 754 double equivalent), and the flags for all 6 exception masks should be set.
#endif
		// verify the initial mxcsr value
		std::uint32_t mxcsr{_mm_getcsr()};
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
	*reinterpret_cast<std::uint64_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint64_t>(L'\n') << 32 | static_cast<std::uint64_t>(L'w') << 16 | static_cast<std::uint64_t>(L' ');// the last wchar_t is correctly set to zero here
	// output debug strings to the system
	OutputDebugStringW(szTicksRu64Text);

	{
		// Set time critical process and thread priority and single-processor operating mode.
		// Set the security descriptor to allow changing the process information.
		// Note: NtCurrentProcess()/ZwCurrentProcess(), NtCurrentThread()/ZwCurrentThread() and NtCurrentSession()/ZwCurrentSession() resolve to macros defined to HANDLE (void *) values of (sign-extended) -1, -2 and -3 respectively in Wdm.h. Due to being hard-coded in user- and kernel-mode drivers like this, these values are pretty certain to never change on this platform.
		DWORD dr{SetSecurityInfo(reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(-1)), SE_KERNEL_OBJECT, PROCESS_SET_INFORMATION, nullptr, nullptr, nullptr, nullptr)};
		if(ERROR_SUCCESS != dr){
			MessageBoxW(nullptr, L"SetSecurityInfo() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}
		BOOL boSetPriorityClass{SetPriorityClass(reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(-1)), REALTIME_PRIORITY_CLASS)};
		if(!boSetPriorityClass){
			MessageBoxW(nullptr, L"SetPriorityClass() failed", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
			return{0};// failure status
		}

		// Enable the permissions to use large pages for VirtualAlloc().
		HANDLE hToken;
		BOOL boOpenProcessToken{OpenProcessToken(reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(-1)), TOKEN_ADJUST_PRIVILEGES, &hToken)};
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

	{// unit tests with the 3 simulated 80-bit long double types
		// direct sorting tests with the 80-bit long double types

		rsbd8::helper::longdoubletest80 aji80[7]{
			{0, 0xFFFFu},// -inf
			{0, 0x7FFFu},// +inf
			{0x8000000000000000u, 0xFFFFu},// QNaN, machine indeterminate
			{0xFFFFFFFFFFFFFFFFu, 0x7FFEu},// max normal
			{0, 1u},// min normal
			{0xFFFFFFFFFFFFFFFFu, 0},// max subnormal
			{1u, 0}};// min subnormal
		rsbd8::helper::longdoubletest80 ajo80[_countof(aji80)], ajb80[_countof(aji80)];
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

		rsbd8::helper::longdoubletest96 aji96[7]{
			{0, 0xABABFFFFu},// -inf
			{0, 0xD0017FFFu},// +inf
			{0x8000000000000000u, 0xEEEEFFFFu},// QNaN, machine indeterminate
			{0xFFFFFFFFFFFFFFFFu, 0x01017FFEu},// max normal
			{0, 0xFFF80001u},// min normal
			{0xFFFFFFFFFFFFFFFFu, 0xC7C80000u},// max subnormal
			{1u, 0xB3710000u}};// min subnormal
		rsbd8::helper::longdoubletest96 ajo96[_countof(aji96)], ajb96[_countof(aji96)];
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

		rsbd8::helper::longdoubletest128 aji128[7]{
			{0, 0xBBBBAAAAABABFFFFu},// -inf
			{0, 0x22221111D0017FFFu},// +inf
			{0x8000000000000000u, 0x44443333EEEEFFFFu},// QNaN, machine indeterminate
			{0xFFFFFFFFFFFFFFFFu, 0x6666555501017FFEu},// max normal
			{0, 0x88887777FFF80001u},// min normal
			{0xFFFFFFFFFFFFFFFFu, 0xCCCC9999C7C80000u},// max subnormal
			{1u, 0xFFFFDDDDB3710000u}};// min subnormal
		rsbd8::helper::longdoubletest128 ajo128[_countof(aji128)], ajb128[_countof(aji128)];
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

		rsbd8::helper::longdoubletest80 *ako80[_countof(aji80)], *akb80[_countof(aji80)], *aki80[_countof(aji80)]{
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

		rsbd8::helper::longdoubletest96 *ako96[_countof(aji96)], *akb96[_countof(aji96)], *aki96[_countof(aji96)]{
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

		rsbd8::helper::longdoubletest128 *ako128[_countof(aji128)], *akb128[_countof(aji128)], *aki128[_countof(aji128)]{
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
		static_cast<void>(sizecontainer);
		static std::size_t constexpr offsetco{rsbd8::getoffsetof<&Testmeclass::co>};
		static_cast<void>(offsetco);
		static std::size_t constexpr offsetsco{rsbd8::getoffsetof<&Testmeclass::sco>};
		static_cast<void>(offsetsco);

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

	// allocate 1 GiB for the in- and outputs
	SIZE_T upLargePageSize{GetLargePageMinimum()};
	upLargePageSize = !upLargePageSize? 4096 : upLargePageSize;// just set it to 4096 if the system doesn't support large pages
	assert(!(upLargePageSize - 1 & upLargePageSize));// only one bit should be set in the value of upLargePageSize
	std::size_t upLargePageSizem1{upLargePageSize - 1};
	std::size_t upSizeIn{(upLargePageSizem1 & -static_cast<std::ptrdiff_t>(1073741824)) + 1073741824};// round up to the nearest multiple of upLargePageSize
	void *in{VirtualAlloc(nullptr, upSizeIn, MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)};
	if(!in){
		MessageBoxW(nullptr, L"out of memory failure", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
		return{0};// failure status
	}
	std::size_t upSizeOut{(upLargePageSizem1 & -static_cast<std::ptrdiff_t>(1073741824 + (upLargePageSize >> 1))) + (1073741824 + (upLargePageSize >> 1))};// round up to the nearest multiple of upLargePageSize
	void *oriout{VirtualAlloc(nullptr, upSizeOut, MEM_LARGE_PAGES | MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)};// add half a page
	if(!oriout){
		BOOL boVirtualFree{VirtualFree(in, 0, MEM_RELEASE)};
		static_cast<void>(boVirtualFree);
		assert(boVirtualFree);
		MessageBoxW(nullptr, L"out of memory failure", nullptr, MB_SYSTEMMODAL | MB_ICONERROR);// The default and localized "error" title caption will be used.
		return{0};// failure status
	}
	void *out{reinterpret_cast<std::int8_t *>(oriout) + (upLargePageSize >> 1)};// offset by half a page, this is an optimization using the processor's addressing methods, and this is used in many memory copy routines

	// measure the TSC execution base time to subtract from the results (method taken from an Intel manual)
	SwitchToThread();// prevent context switching during the benchmark
	std::uint64_t u64init;
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
		std::uint64_t u64start{__rdtsc()};
		std::uint64_t u64stop;
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

		srand(static_cast<unsigned>(u64start));// prepare a seed for rand()
	}
	{
		// filled initialization of the input part (only done once)
		static_assert(RAND_MAX == 0x7FFF, L"RAND_MAX changed from 0x7FFF (15 bits of data), update this part of the code");
		std::uint64_t *pFIin{reinterpret_cast<std::uint64_t *>(in)};
		std::uint32_t j{134217728ui32};// in 134217728 batches of 8 bytes
		do{
			*pFIin++ = static_cast<std::uint64_t>(static_cast<unsigned>(rand())) << 60
				| static_cast<std::uint64_t>(static_cast<unsigned>(rand())) << 45 | static_cast<std::uint64_t>(static_cast<unsigned>(rand())) << 30
				| static_cast<std::uint64_t>(static_cast<unsigned>(rand()) << 15) | static_cast<std::uint64_t>(static_cast<unsigned>(rand()));
		}while(--j);
	}

	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort<rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::forcefloatingp>(1073741824, reinterpret_cast<std::uint8_t *>(out), upLargePageSize);// in 1073741824 batches of 1 byte

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"mini rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy<rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::forcefloatingp>(1073741824, reinterpret_cast<std::uint8_t const *>(in), reinterpret_cast<std::uint8_t *>(out), upLargePageSize);// in 1073741824 batches of 1 byte

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"mini rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort<rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::forcefloatingp>(536870912, reinterpret_cast<std::uint16_t *>(out), upLargePageSize);// in 536870912 batches of 2 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"half rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy<rsbd8::sortingdirection::ascfwdorder, rsbd8::sortingmode::forcefloatingp>(536870912, reinterpret_cast<std::uint16_t const *>(in), reinterpret_cast<std::uint16_t *>(out), upLargePageSize);// in 536870912 batches of 2 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"half rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::sort(reinterpret_cast<float *>(out), reinterpret_cast<float *>(out) + 268435456);// in 268435456 batches of 4 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"float std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		//assert(std::is_sorted(reinterpret_cast<float *>(out), reinterpret_cast<float *>(out) + 268435456));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::stable_sort(reinterpret_cast<float *>(out), reinterpret_cast<float *>(out) + 268435456);// in 268435456 batches of 4 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"float std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		//assert(std::is_sorted(reinterpret_cast<float *>(out), reinterpret_cast<float *>(out) + 268435456));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(268435456, reinterpret_cast<float *>(out), upLargePageSize);// in 268435456 batches of 4 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"float rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		//assert(std::is_sorted(reinterpret_cast<float *>(out), reinterpret_cast<float *>(out) + 268435456));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy(268435456, reinterpret_cast<float const *>(in), reinterpret_cast<float *>(out), upLargePageSize);// in 268435456 batches of 4 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"float rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);

		//assert(std::is_sorted(reinterpret_cast<float *>(out), reinterpret_cast<float *>(out) + 268435456));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::sort(reinterpret_cast<double *>(out), reinterpret_cast<double *>(out) + 134217728);// in 134217728 batches of 8 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"double std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		//assert(std::is_sorted(reinterpret_cast<double *>(out), reinterpret_cast<double *>(out) + 134217728));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::stable_sort(reinterpret_cast<double *>(out), reinterpret_cast<double *>(out) + 134217728);// in 134217728 batches of 8 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"double std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		//assert(std::is_sorted(reinterpret_cast<double *>(out), reinterpret_cast<double *>(out) + 134217728));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(134217728, reinterpret_cast<double *>(out), upLargePageSize);// in 134217728 batches of 8 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"double rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		//assert(std::is_sorted(reinterpret_cast<double *>(out), reinterpret_cast<double *>(out) + 134217728));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy(134217728, reinterpret_cast<double const *>(in), reinterpret_cast<double *>(out), upLargePageSize);// in 134217728 batches of 8 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"double rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);

		//assert(std::is_sorted(reinterpret_cast<double *>(out), reinterpret_cast<double *>(out) + 134217728));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(67108864, reinterpret_cast<rsbd8::helper::longdoubletest128 *>(out), upLargePageSize);// in 67108864 batches of 16 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"encapsulated long double rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy(67108864, reinterpret_cast<rsbd8::helper::longdoubletest128 const *>(in), reinterpret_cast<rsbd8::helper::longdoubletest128 *>(out), upLargePageSize);// in 67108864 batches of 16 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"encapsulated long double rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::sort(reinterpret_cast<std::uint64_t *>(out), reinterpret_cast<std::uint64_t *>(out) + 134217728);// in 134217728 batches of 8 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint64_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::uint64_t *>(out), reinterpret_cast<std::uint64_t *>(out) + 134217728));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::stable_sort(reinterpret_cast<std::uint64_t *>(out), reinterpret_cast<std::uint64_t *>(out) + 134217728);// in 134217728 batches of 8 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint64_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::uint64_t *>(out), reinterpret_cast<std::uint64_t *>(out) + 134217728));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(134217728, reinterpret_cast<std::uint64_t *>(out), upLargePageSize);// in 134217728 batches of 8 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint64_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::uint64_t *>(out), reinterpret_cast<std::uint64_t *>(out) + 134217728));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy(134217728, reinterpret_cast<std::uint64_t const *>(in), reinterpret_cast<std::uint64_t *>(out), upLargePageSize);// in 134217728 batches of 8 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint64_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::uint64_t *>(out), reinterpret_cast<std::uint64_t *>(out) + 134217728));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::sort(reinterpret_cast<std::int64_t *>(out), reinterpret_cast<std::int64_t *>(out) + 134217728);// in 134217728 batches of 8 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int64_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::int64_t *>(out), reinterpret_cast<std::int64_t *>(out) + 134217728));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::stable_sort(reinterpret_cast<std::int64_t *>(out), reinterpret_cast<std::int64_t *>(out) + 134217728);// in 134217728 batches of 8 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int64_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::int64_t *>(out), reinterpret_cast<std::int64_t *>(out) + 134217728));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(134217728, reinterpret_cast<std::int64_t *>(out), upLargePageSize);// in 134217728 batches of 8 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int64_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::int64_t *>(out), reinterpret_cast<std::int64_t *>(out) + 134217728));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy(134217728, reinterpret_cast<std::int64_t const *>(in), reinterpret_cast<std::int64_t *>(out), upLargePageSize);// in 134217728 batches of 8 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int64_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::int64_t *>(out), reinterpret_cast<std::int64_t *>(out) + 134217728));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::sort(reinterpret_cast<std::uint32_t *>(out), reinterpret_cast<std::uint32_t *>(out) + 268435456);// in 268435456 batches of 4 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint32_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::uint32_t *>(out), reinterpret_cast<std::uint32_t *>(out) + 268435456));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::stable_sort(reinterpret_cast<std::uint32_t *>(out), reinterpret_cast<std::uint32_t *>(out) + 268435456);// in 268435456 batches of 4 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint32_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::uint32_t *>(out), reinterpret_cast<std::uint32_t *>(out) + 268435456));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(268435456, reinterpret_cast<std::uint32_t *>(out), upLargePageSize);// in 268435456 batches of 4 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint32_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::uint32_t *>(out), reinterpret_cast<std::uint32_t *>(out) + 268435456));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy(268435456, reinterpret_cast<std::uint32_t const *>(in), reinterpret_cast<std::uint32_t *>(out), upLargePageSize);// in 268435456 batches of 4 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint32_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::uint32_t *>(out), reinterpret_cast<std::uint32_t *>(out) + 268435456));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::sort(reinterpret_cast<std::int32_t *>(out), reinterpret_cast<std::int32_t *>(out) + 268435456);// in 268435456 batches of 4 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int32_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::int32_t *>(out), reinterpret_cast<std::int32_t *>(out) + 268435456));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::stable_sort(reinterpret_cast<std::int32_t *>(out), reinterpret_cast<std::int32_t *>(out) + 268435456);// in 268435456 batches of 4 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int32_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::int32_t *>(out), reinterpret_cast<std::int32_t *>(out) + 268435456));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(268435456, reinterpret_cast<std::int32_t *>(out), upLargePageSize);// in 268435456 batches of 4 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int32_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::int32_t *>(out), reinterpret_cast<std::int32_t *>(out) + 268435456));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy(268435456, reinterpret_cast<std::int32_t const *>(in), reinterpret_cast<std::int32_t *>(out), upLargePageSize);// in 268435456 batches of 4 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int32_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::int32_t *>(out), reinterpret_cast<std::int32_t *>(out) + 268435456));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::sort(reinterpret_cast<std::uint16_t *>(out), reinterpret_cast<std::uint16_t *>(out) + 536870912);// in 536870912 batches of 2 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint16_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::uint16_t *>(out), reinterpret_cast<std::uint16_t *>(out) + 536870912));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::stable_sort(reinterpret_cast<std::uint16_t *>(out), reinterpret_cast<std::uint16_t *>(out) + 536870912);// in 536870912 batches of 2 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint16_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::uint16_t *>(out), reinterpret_cast<std::uint16_t *>(out) + 536870912));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(536870912, reinterpret_cast<std::uint16_t *>(out), upLargePageSize);// in 536870912 batches of 2 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint16_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::uint16_t *>(out), reinterpret_cast<std::uint16_t *>(out) + 536870912));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy(536870912, reinterpret_cast<std::uint16_t const *>(in), reinterpret_cast<std::uint16_t *>(out), upLargePageSize);// in 536870912 batches of 2 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint16_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::uint16_t *>(out), reinterpret_cast<std::uint16_t *>(out) + 536870912));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::sort(reinterpret_cast<std::int16_t *>(out), reinterpret_cast<std::int16_t *>(out) + 536870912);// in 536870912 batches of 2 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int16_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::int16_t *>(out), reinterpret_cast<std::int16_t *>(out) + 536870912));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::stable_sort(reinterpret_cast<std::int16_t *>(out), reinterpret_cast<std::int16_t *>(out) + 536870912);// in 536870912 batches of 2 bytes
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int16_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::int16_t *>(out), reinterpret_cast<std::int16_t *>(out) + 536870912));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(536870912, reinterpret_cast<std::int16_t *>(out), upLargePageSize);// in 536870912 batches of 2 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int16_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::int16_t *>(out), reinterpret_cast<std::int16_t *>(out) + 536870912));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy(536870912, reinterpret_cast<std::int16_t const *>(in), reinterpret_cast<std::int16_t *>(out), upLargePageSize);// in 536870912 batches of 2 bytes

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int16_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::int16_t *>(out), reinterpret_cast<std::int16_t *>(out) + 536870912));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::sort(reinterpret_cast<std::uint8_t *>(out), reinterpret_cast<std::uint8_t *>(out) + 1073741824);// in 1073741824 batches of 1 byte
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint8_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::uint8_t *>(out), reinterpret_cast<std::uint8_t *>(out) + 1073741824));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::stable_sort(reinterpret_cast<std::uint8_t *>(out), reinterpret_cast<std::uint8_t *>(out) + 1073741824);// in 1073741824 batches of 1 byte
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint8_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::uint8_t *>(out), reinterpret_cast<std::uint8_t *>(out) + 1073741824));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(1073741824, reinterpret_cast<std::uint8_t *>(out), upLargePageSize);// in 1073741824 batches of 1 byte

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint8_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::uint8_t *>(out), reinterpret_cast<std::uint8_t *>(out) + 1073741824));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy(1073741824, reinterpret_cast<std::uint8_t const *>(in), reinterpret_cast<std::uint8_t *>(out), upLargePageSize);// in 1073741824 batches of 1 byte

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::uint8_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::uint8_t *>(out), reinterpret_cast<std::uint8_t *>(out) + 1073741824));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::sort(reinterpret_cast<std::int8_t *>(out), reinterpret_cast<std::int8_t *>(out) + 1073741824);// in 1073741824 batches of 1 byte
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int8_t std::sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::int8_t *>(out), reinterpret_cast<std::int8_t *>(out) + 1073741824));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};
#ifndef _DEBUG// skip in debug builds as it is way too slow
		std::stable_sort(reinterpret_cast<std::int8_t *>(out), reinterpret_cast<std::int8_t *>(out) + 1073741824);// in 1073741824 batches of 1 byte
#endif
		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int8_t std::stable_sort() test\n");
		OutputDebugStringW(szTicksRu64Text);
#ifndef _DEBUG// skip in debug builds as it is way too slow
		assert(std::is_sorted(reinterpret_cast<std::int8_t *>(out), reinterpret_cast<std::int8_t *>(out) + 1073741824));
#endif
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(1073741824, reinterpret_cast<std::int8_t *>(out), upLargePageSize);// in 1073741824 batches of 1 byte

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int8_t rsbd8::radixsort() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::int8_t *>(out), reinterpret_cast<std::int8_t *>(out) + 1073741824));
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

		std::memcpy(out, in, 1073741824);// dummy copy

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsortcopy(1073741824, reinterpret_cast<std::int8_t const *>(in), reinterpret_cast<std::int8_t *>(out), upLargePageSize);// in 1073741824 batches of 1 byte

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(L"std::int8_t rsbd8::radixsortcopy() test\n");
		OutputDebugStringW(szTicksRu64Text);

		assert(std::is_sorted(reinterpret_cast<std::int8_t *>(out), reinterpret_cast<std::int8_t *>(out) + 1073741824));
	}
	// two basic indirection tests, these warm up the caches differently
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

#ifdef _WIN64
		std::uint64_t const *pSource{reinterpret_cast<std::uint64_t const *>(in)};
		std::uint64_t const **pDest{reinterpret_cast<std::uint64_t const **>(out)};
		std::uint32_t i{134217728ui32};// in 134217728 batches of 8 bytes
#else
		std::uint32_t const *pSource{reinterpret_cast<std::uint32_t const *>(in)};
		std::uint32_t const **pDest{reinterpret_cast<std::uint32_t const **>(out)};
		std::uint32_t i{268435456ui32};// in 268435456 batches of 4 bytes
#endif
		do{
			*pDest++ = pSource++;
		}while(--i);

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(
#ifdef _WIN64
			134217728, reinterpret_cast<std::uint64_t **>(out),// in 134217728 batches of 8 bytes
#else
			268435456, reinterpret_cast<std::uint32_t **>(out),// in 268435456 batches of 4 bytes
#endif
			upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(
#ifdef _WIN64
			L"std::uint64_t * rsbd8::radixsort() test\n"
#else
			L"std::uint32_t * rsbd8::radixsort() test\n"
#endif
		);
		OutputDebugStringW(szTicksRu64Text);
	}
	// run an empty loop to warm up the caches first
	// this acts as a dumb copy loop to the memory at the out pointer for the one next sorting section as well
	{
		// filled initialization of the output part
		Sleep(125);// prevent context switching during the benchmark, allow some time to possibly zero the memory given back by VirtualFree()
		__m128 xf{_mm_undefined_ps()};
		xf = _mm_cmp_ps(xf, xf, _CMP_NLT_US);// set all bits (even works if xf happens to contain NaN), even for code at the SSE2-level, avoid all the floating-point comparison intrinsics of emmintrin.h and earlier, as these will in many cases force the left and right argument to swap or use extra instructions to deal with NaN and not encode to assembly as expected, see immintrin.h for more details
		float *pFIout{reinterpret_cast<float *>(out)};
		std::uint32_t j{67108864ui32};// in 67108864 batches of 16 bytes
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
		std::uint64_t u64start{__rdtsc()};

#ifdef _WIN64
		std::uint64_t const *pSource{reinterpret_cast<std::uint64_t const *>(in)};
		std::uint64_t const **pDest{reinterpret_cast<std::uint64_t const **>(out)};
		std::uint32_t i{134217728ui32};// in 134217728 batches of 8 bytes
#else
		std::uint32_t const *pSource{reinterpret_cast<std::uint32_t const *>(in)};
		std::uint32_t const **pDest{reinterpret_cast<std::uint32_t const **>(out)};
		std::uint32_t i{268435456ui32};// in 268435456 batches of 4 bytes
#endif
		do{
			*pDest++ = pSource++;
		}while(--i);

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		//OutputDebugStringW(L"warming up caches, ignore this benchmark\n");
		//OutputDebugStringW(szTicksRu64Text);
	}
	{// warning! requires a copy of the data at the out pointer here, the in pointer isn't used
		// start measuring
		{
			int cpuInfo[4];// unused
			__cpuid(cpuInfo, 0);// only used for serializing execution
			static_cast<void>(cpuInfo);
			static_cast<void>(cpuInfo[0]);
			static_cast<void>(cpuInfo[1]);
			static_cast<void>(cpuInfo[2]);
			static_cast<void>(cpuInfo[3]);
		}
		std::uint64_t u64start{__rdtsc()};

		rsbd8::radixsort(
#ifdef _WIN64
			134217728, reinterpret_cast<double **>(out),// in 134217728 batches of 8 bytes
#else
			268435456, reinterpret_cast<float **>(out),// in 268435456 batches of 4 bytes
#endif
			upLargePageSize);

		// stop measuring
		std::uint64_t u64stop;
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
		*reinterpret_cast<std::uint32_t UNALIGNED *>(szTicksRu64Text + 20) = static_cast<std::uint32_t>(L'\n');// the last wchar_t is correctly set to zero here
		// output debug strings to the system
		OutputDebugStringW(
#ifdef _WIN64
			L"double * rsbd8::radixsort() test\n"
#else
			L"float * rsbd8::radixsort() test\n"
#endif
		);
		OutputDebugStringW(szTicksRu64Text);
	}

	// benchmark finished time
	WritePaddedu64(szTicksRu64Text, PerfCounter100ns());
	*reinterpret_cast<std::uint64_t UNALIGNED*>(szTicksRu64Text + 20) = static_cast<std::uint64_t>(L'\n') << 32 | static_cast<std::uint64_t>(L'b') << 16 | static_cast<std::uint64_t>(L' ');// the last wchar_t is correctly set to zero here
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
	wcex.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<std::uintptr_t>(COLOR_WINDOWFRAME));
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