#pragma once
#include <IbWinCppLib/WinCppLib.hpp>
#include <detours/detours.h>

constexpr bool debug =
#ifdef IB_DEBUG
    1;
#else
    0;
#endif

constexpr bool debug_verbose =
#ifdef IB_DEBUG_VERBOSE
    1;
#else
    0;
#endif

inline ib::DebugOStream<> DebugOStream() {
    return ib::DebugOStream(L"IbEverythingExt: ");
}

template<typename T>
LONG IbDetourAttach(_Inout_ T* ppPointer, _In_ T pDetour) {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach((void**)ppPointer, pDetour);
    return DetourTransactionCommit();
}

template<typename T>
LONG IbDetourDetach(_Inout_ T* ppPointer, _In_ T pDetour) {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach((void**)ppPointer, pDetour);
    return DetourTransactionCommit();
}