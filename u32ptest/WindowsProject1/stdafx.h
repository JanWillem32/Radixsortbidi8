// stdafx.h : include file for standard system include files, or project specific include files that are used frequently, but are changed infrequently

#pragma once

// Exclude rarely-used stuff from Windows headers.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

// The system's thread and process environment blocks can be read using the fs (32-bit x86) or gs (64-bit x86) pointers for retrieving a lot of the common environment data. (The operating system does te exact same thing for the system calls listed in these inline functions.)
// These are thread-safe items (mostly applies to InlineDerefTEBLastError()), do not generate any calls and have no further dependencies.
// There are quite a few more items that can be read from the TEB and PEB than the items listed here, these can be added here when needed. Please keep the functions sorted by class (TEB, PEB and then RTL_USER_PROCESS_PARAMETERS) and by internal structure order.
// Note: NtCurrentProcess()/ZwCurrentProcess(), NtCurrentThread()/ZwCurrentThread() and NtCurrentSession()/ZwCurrentSession() resolve to macros defined to HANDLE (void *) values of (sign-extended) -1, -2 and -3 respectively in Wdm.h. Due to being hard-coded in user- and kernel-mode drivers like this, these values are pretty certain to never change on this platform.
#include "targetver.h"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC// include Microsoft memory leak detection procedures
#endif
#include <crtdbg.h>// memory allocation (including plain malloc() and new()) and other debugging enhancements
#include <windows.h>
#include <Winternl.h>// This header contains a tiny bit of data regarding the system's thread and process environment blocks, along with their associated definitions, types, enumerations and structures.
#include <cassert>
#include "FeatureBitsCPUId.h"
#include "PerfCounter100ns.h"

__declspec(noalias safebuffers) __forceinline HANDLE InlineCurrentProcessPseudohandle()noexcept{return{reinterpret_cast<HANDLE>(static_cast<intptr_t>(-1))};}
__declspec(noalias safebuffers) __forceinline HANDLE InlineCurrentThreadPseudohandle()noexcept{return{reinterpret_cast<HANDLE>(static_cast<intptr_t>(-2))};}
__declspec(noalias safebuffers) __forceinline HANDLE InlineCurrentSessionPseudohandle()noexcept{return{reinterpret_cast<HANDLE>(static_cast<intptr_t>(-3))};}

__declspec(noalias safebuffers) __forceinline void *InlineDerefTEBStackBasePtr()noexcept{
#ifdef _WIN64
	uintptr_t pStackBase{__readgsqword(0x8)};
#else
	uintptr_t pStackBase{__readfsdword(0x4)};
#endif
	assert(pStackBase);
	return{reinterpret_cast<void *>(pStackBase)};
}

__declspec(noalias safebuffers) __forceinline void *InlineDerefTEBStackLimitPtr()noexcept{
#ifdef _WIN64
	uintptr_t pStackLimit{__readgsqword(0x10)};
#else
	uintptr_t pStackLimit{__readfsdword(0x8)};
#endif
	assert(pStackLimit);
	return{reinterpret_cast<void *>(pStackLimit)};
}

__declspec(noalias safebuffers) __forceinline uint32_t InlineDerefTEBProcessId()noexcept{
	// GetCurrentProcessId() internal
#ifdef _WIN64
	uint32_t u32ProcessId{__readgsdword(0x40)};
#else
	uint32_t u32ProcessId{__readfsdword(0x20)};
#endif
	assert(u32ProcessId);
	return{u32ProcessId};
}

__declspec(noalias safebuffers) __forceinline uint32_t InlineDerefTEBThreadId()noexcept{
	// GetCurrentThreadId() internal
#ifdef _WIN64
	uint32_t u32ThreadId{__readgsdword(0x48)};
#else
	uint32_t u32ThreadId{__readfsdword(0x24)};
#endif
	assert(u32ThreadId);
	return{u32ThreadId};
}

__declspec(noalias safebuffers) __forceinline PEB *InlineDerefTEBProcessEnvironmentBlockPtr()noexcept{
#ifdef _WIN64
	uintptr_t pProcessEnvironmentBlock{__readgsqword(0x60)};
#else
	uintptr_t pProcessEnvironmentBlock{__readfsdword(0x30)};
#endif
	assert(pProcessEnvironmentBlock);
	return{reinterpret_cast<PEB *>(pProcessEnvironmentBlock)};
}

__declspec(noalias safebuffers) __forceinline uint32_t InlineDerefTEBLastError()noexcept{
	// GetLastError() internal
#ifdef _WIN64
	uint32_t u32LastError{__readgsdword(0x68)};
#else
	uint32_t u32LastError{__readfsdword(0x34)};
#endif
	return{u32LastError};
}

__declspec(noalias safebuffers) __forceinline LCID InlineDerefTEBCurrentLocale()noexcept{
	// GetUserDefaultLCID() internal
#ifdef _WIN64
	uint32_t u32CurrentLocale{__readgsdword(0x108)};
#else
	uint32_t u32CurrentLocale{__readfsdword(0xC4)};
#endif
	assert(u32CurrentLocale);
	return{static_cast<LCID>(u32CurrentLocale)};
}

// __ImageBase is the exe/dll file image base pointer in memory.
extern "C" IMAGE_DOS_HEADER __ImageBase;// created by the linker
// The __ImageBase object created by the linker is available at compile time, there really should be no need for InlineDerefPEBImageBaseAddress() within an exe project. It is only useful for dll projects to get a pointer to the executable's base address.
// The IMAGE_DOS_HEADER * can simply be casted to an HINSTANCE/HMODULE (they are the same thing since 32-bit windows was introduced).
// within dll code: reinterpret_cast<HINSTANCE>(&__ImageBase);// HINSTANCE/HMODULE of this dll file
// within dll code: reinterpret_cast<HINSTANCE>(InlineDerefPEBImageBaseAddress(pProcessEnvironmentBlock));// HINSTANCE/HMODULE of this program
// within exe code: reinterpret_cast<HINSTANCE>(&__ImageBase);// HINSTANCE/HMODULE of this exe file and this program
// For both dll and exe files various resouces can be extracted from this header. code: IMAGE_NT_HEADERS const *pNTHeader{reinterpret_cast<IMAGE_NT_HEADERS const *>(reinterpret_cast<uintptr_t>(pImageBaseAddress) + static_cast<size_t>(static_cast<ULONG>(pImageBaseAddress->e_lfanew)))};
// A nice detail: the "Optional header format" inside which is used to access the resource tables has actually never been optional at all in released OS versions.
#ifdef _DLL// This data is only useful for dll files, by all means use _DLL in the code to detect whether the code is within an exe or a dll file.
__declspec(noalias safebuffers) __forceinline IMAGE_DOS_HEADER *InlineDerefPEBImageBaseAddress(PEB *pProcessEnvironmentBlock)noexcept{
#ifdef _WIN64
	IMAGE_DOS_HEADER *pImageBaseAddress{*reinterpret_cast<IMAGE_DOS_HEADER **>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x10)};// GetModuleHandle(nullptr) internal
#else
	IMAGE_DOS_HEADER *pImageBaseAddress{*reinterpret_cast<IMAGE_DOS_HEADER **>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x8)};// GetModuleHandle(nullptr) internal
#endif
	assert(pImageBaseAddress);
	return{pImageBaseAddress;
}
#endif

__declspec(noalias safebuffers) __forceinline RTL_USER_PROCESS_PARAMETERS *InlineDerefPEBUserProcessParameters(PEB *pProcessEnvironmentBlock)noexcept{
	RTL_USER_PROCESS_PARAMETERS *pUserProcessParameters{pProcessEnvironmentBlock->ProcessParameters};
	assert(pUserProcessParameters);
	return{pUserProcessParameters};
}

__declspec(noalias safebuffers) __forceinline HANDLE InlineDerefPEBProcessHeap(PEB *pProcessEnvironmentBlock)noexcept{
	// GetProcessHeap() internal
#ifdef _WIN64
	HANDLE hProcessHeap{*reinterpret_cast<HANDLE *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x30)};
#else
	HANDLE hProcessHeap{*reinterpret_cast<HANDLE *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x18)};
#endif
	assert(hProcessHeap);
	return{hProcessHeap};
}

__declspec(noalias safebuffers) __forceinline uint32_t InlineDerefPEBNumberOfProcessors(PEB *pProcessEnvironmentBlock)noexcept{
	// GetSystemInfo(&) .dwNumberOfProcessors internal
#ifdef _WIN64
	uint32_t u32NumberOfProcessors{*reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0xB8)};
#else
	uint32_t u32NumberOfProcessors{*reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x64)};
#endif
	assert(u32NumberOfProcessors);
	return{u32NumberOfProcessors};
}

__declspec(noalias safebuffers) __forceinline bool InlineDerefPEBDetectDebugger(PEB *pProcessEnvironmentBlock)noexcept{
	// IsDebuggerPresent() internal
#ifdef _WIN64
	uint32_t u32NtGlobalFlag{*reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0xBC)};
#else
	uint32_t u32NtGlobalFlag{*reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x68)};
#endif
	return{u32NtGlobalFlag == 0x70 || pProcessEnvironmentBlock->BeingDebugged};
}

__declspec(noalias safebuffers) __forceinline uint8_t InlineDerefPEBOSMajorVersion(PEB *pProcessEnvironmentBlock)noexcept{
	// GetVersionEx(&) .dwMajorVersion internal
#ifdef _WIN64
	uint8_t u8OSMajorVersion{*reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x118)};
#else
	uint8_t u8OSMajorVersion{*reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0xA4)};
#endif
	assert(u8OSMajorVersion);
	return{u8OSMajorVersion};// Because of the limitation in the GetVersion() call, only one byte can be populated until GetVersion() is deprecated.
}

__declspec(noalias safebuffers) __forceinline uint8_t InlineDerefPEBOSMinorVersion(PEB *pProcessEnvironmentBlock)noexcept{
	// GetVersionEx(&) .dwMinorVersion internal
#ifdef _WIN64
	uint8_t u8OSMinorVersion{*reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x11C)};
#else
	uint8_t u8OSMinorVersion{*reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0xA8)};
#endif
	return{u8OSMinorVersion};// Because of the limitation in the GetVersion() call, only one byte can be populated until GetVersion() is deprecated.
}

__declspec(noalias safebuffers) __forceinline uint16_t InlineDerefPEBOSBuildNumber(PEB *pProcessEnvironmentBlock)noexcept{
	// GetVersionEx(&) .dwBuildNumber internal
#ifdef _WIN64
	uint16_t u16OSBuildNumber{*reinterpret_cast<uint16_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x120)};
#else
	uint16_t u16OSBuildNumber{*reinterpret_cast<uint16_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0xAC)};
#endif
	assert(u16OSBuildNumber);
	return{u16OSBuildNumber};
}

__declspec(noalias safebuffers) __forceinline uint16_t InlineDerefPEBOSOSCSDVersion(PEB *pProcessEnvironmentBlock)noexcept{
	// GetVersionEx(&) .wServicePackMajor internal
#ifdef _WIN64
	uint16_t u16OSCSDVersion{*reinterpret_cast<uint16_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x120)};
#else
	uint16_t u16OSCSDVersion{*reinterpret_cast<uint16_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0xAC)};
#endif
	assert(u16OSCSDVersion);
	return{u16OSCSDVersion};
}

__declspec(noalias safebuffers) __forceinline uint32_t InlineDerefPEBOSPlatformId(PEB *pProcessEnvironmentBlock)noexcept{
	// GetVersionEx(&) .dwPlatformId internal
#ifdef _WIN64
	uint32_t u32OSPlatformId{*reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x124)};
#else
	uint32_t u32OSPlatformId{*reinterpret_cast<uint32_t *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0xB0)};
#endif
	return{u32OSPlatformId};
}

__declspec(noalias safebuffers) __forceinline uint32_t InlineDerefPEBSessionId(PEB *pProcessEnvironmentBlock)noexcept{
	// ProcessIdToSessionId(GetCurrentProcessId(), &) internal
	uint32_t u32SessionId{pProcessEnvironmentBlock->SessionId};
	assert(u32SessionId);
	return{u32SessionId};
}

// A UNICODE_STRING structure gives a Length in bytes, excluding the zero end, but MaximumLength includes the zero end. This is a bit different compared to how other strings typically are defined.
// This function can actually return nullptr or just a null-terminated empty string for pCSDVersion->Buffer, 0 for pCSDVersion->Length and 0 or 2 for pCSDVersion->MaximumLength when the OS doesn't have a service pack installed.
__declspec(noalias safebuffers) __forceinline UNICODE_STRING *InlineDerefPEBCSDVersion(PEB *pProcessEnvironmentBlock)noexcept{
	// GetVersionEx(&) .szCSDVersion internal
#ifdef _WIN64
	UNICODE_STRING *pCSDVersion{reinterpret_cast<UNICODE_STRING *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x2E8)};
#else
	UNICODE_STRING *pCSDVersion{reinterpret_cast<UNICODE_STRING *>(reinterpret_cast<uintptr_t>(pProcessEnvironmentBlock) + 0x1F0)};
#endif
	return{pCSDVersion};
}

// A UNICODE_STRING structure gives a Length in bytes, excluding the zero end, but MaximumLength includes the zero end. This is a bit different compared to how other strings typically are defined.
__declspec(noalias safebuffers) __forceinline UNICODE_STRING *InlineDerefPEBImagePathName(RTL_USER_PROCESS_PARAMETERS *pUserProcessParameters)noexcept{
	// GetModuleFileNameW(nullptr, &, ?) internal
	UNICODE_STRING *pImagePathName{&pUserProcessParameters->ImagePathName};
	assert(pImagePathName->Buffer);
	assert(pImagePathName->Length);
	assert(pImagePathName->MaximumLength);
	return{pImagePathName};
}

// A UNICODE_STRING structure gives a Length in bytes, excluding the zero end, but MaximumLength includes the zero end. This is a bit different compared to how other strings typically are defined.
__declspec(noalias safebuffers) __forceinline UNICODE_STRING *InlineDerefPEBCommandLine(RTL_USER_PROCESS_PARAMETERS *pUserProcessParameters)noexcept{
	// GetCommandLineW() internal
	UNICODE_STRING *pCommandLine{&pUserProcessParameters->CommandLine};
	assert(pCommandLine->Buffer);
	assert(pCommandLine->Length);
	assert(pCommandLine->MaximumLength);
	return{pCommandLine};
}

// A UNICODE_STRING structure gives a Length in bytes, excluding the zero end, but MaximumLength includes the zero end. This is a bit different compared to how other strings typically are defined.
// This function can actually return nullptr or just a null-terminated empty string for pDesktopInfo->Buffer, 0 for pDesktopInfo->Length and 0 or 2 for pDesktopInfo->MaximumLength when the deskop process is offline.
// Unless the program launches in a previously created, private desktop by another program (not per se by a sandbox, emulator or virtual machine) the string will be "Winsta0\\Default" if launced by the regular desktop or "Winsta0\\Winlogon" if it's launced as a service in the Winlogon domain.
__declspec(noalias safebuffers) __forceinline UNICODE_STRING *InlineDerefPEBDesktopInfo(RTL_USER_PROCESS_PARAMETERS *pUserProcessParameters)noexcept{
#ifdef _WIN64
	UNICODE_STRING *pDesktopInfo{reinterpret_cast<UNICODE_STRING *>(reinterpret_cast<uintptr_t>(pUserProcessParameters) + 0xC0)};
#else
	UNICODE_STRING *pDesktopInfo{reinterpret_cast<UNICODE_STRING *>(reinterpret_cast<uintptr_t>(pUserProcessParameters) + 0x78)};
#endif
	return{pDesktopInfo};
}