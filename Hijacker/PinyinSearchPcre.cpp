#include "pch.h"
#include "PinyinSearchPcre.hpp"
#include "config.hpp"
#include "ipc.hpp"
#include "match.hpp"
#include "helper.hpp"

using ModifierValue = uint32_t;
struct Modifier {
    using T = const ModifierValue;

    static T RegEx = 0x800;
};
#pragma pack(push, 1)
struct compile_p2 {
    ib::Byte gap0[24];
    void* result18;
    void* result20;
    ModifierValue modifiers;
    uint32_t int2C;
    char pattern[];
};
#pragma pack(pop)

ModifierValue modifiers;
char pattern_initial;

bool (*regex_compile_p2_real)(void* a1, compile_p2* a2);
bool regex_compile_p2_detour(void* a1, compile_p2* a2) {
    if constexpr (debug) {
        size_t pattern_len = strlen(a2->pattern);
        std::wstring pattern_u16(pattern_len, L'\0');
        pattern_u16.resize(MultiByteToWideChar(CP_UTF8, 0, a2->pattern, pattern_len, pattern_u16.data(), pattern_u16.size()));
        DebugOStream() << L"regex_compile_p2(" << std::hex << a2 << L"{" << a2->modifiers << L", " << a2->int2C << LR"(}, ")" << pattern_u16 << LR"("))"
            << L" on " << GetCurrentThreadId() << L"\n";

        // single thread
    }

    modifiers = a2->modifiers;

    if (!(a2->modifiers & Modifier::RegEx)) {
        pattern_initial = a2->pattern[0];
        if (pattern_initial) {  // force to use regex
            a2->modifiers |= Modifier::RegEx;
            a2->pattern[0] = '$';  // .\[^$*{?+|()
        }
    }

    if constexpr (debug) {
        bool result = regex_compile_p2_real(a1, a2);
        DebugOStream() << L"{" << a2->result18 << L", " << a2->result20 << L"}\n";
        return result;
    }
    return regex_compile_p2_real(a1, a2);
}


#define PCRE_CALL_CONVENTION

using pcre = void;
using pcre_extra = void;
#define PCRE_SPTR const char *


/*************************************************
*        Compile a Regular Expression            *
*************************************************/

/* This function takes a string and returns a pointer to a block of store
holding a compiled version of the expression. The original API for this
function had no error code return variable; it is retained for backwards
compatibility. The new function is given a new name.

Arguments:
  pattern       the regular expression
  options       various option bits
  errorcodeptr  pointer to error code variable (pcre_compile2() only)
                  can be NULL if you don't want a code value
  errorptr      pointer to pointer to error text
  erroroffset   ptr offset in pattern where error was detected
  tables        pointer to character tables or NULL

Returns:        pointer to compiled data block, or NULL on error,
                with errorptr and erroroffset set
*/

pcre* PCRE_CALL_CONVENTION
(*pcre_compile2_real)(const char* pattern, int options, int* errorcodeptr,
    const char** errorptr, int* erroroffset, const unsigned char* tables);

pcre* PCRE_CALL_CONVENTION
pcre_compile2_detour(const char* pattern, int options, int* errorcodeptr,
    const char** errorptr, int* erroroffset, const unsigned char* tables)
{
    if (!(modifiers & Modifier::RegEx)) {
        const_cast<char*>(pattern)[0] = pattern_initial;
    }

    if constexpr (debug) {
        size_t pattern_len = strlen(pattern);
        std::wstring pattern_u16(pattern_len, L'\0');
        pattern_u16.resize(MultiByteToWideChar(CP_UTF8, 0, pattern, pattern_len, pattern_u16.data(), pattern_u16.size()));
        DebugOStream() << LR"(pcre_compile2(")" << pattern_u16 << LR"(", )" << std::hex << options << std::dec << L", " << errorcodeptr << L", " << errorptr << L", " << erroroffset << L", " << tables
            << std::hex << L") on " << GetCurrentThreadId() << L"\n";

        /*
        pcre_compile2(".", PCRE_UCP | PCRE_UTF8 | PCRE_CASELESS, 0x0020E9D8, 0x0020E9A0, 0x0020E9C0, nullptr)
        pcre_compile2(".", PCRE_UCP | PCRE_UTF8 | PCRE_CASELESS, 0x0020E9D8, 0x0020E9A0, 0x0020E9C0, nullptr)
        single thread
        */
    }

    if (modifiers & Modifier::RegEx) {
        if constexpr (debug) {
            pcre* re = pcre_compile2_real(pattern, options, errorcodeptr, errorptr, erroroffset, tables);
            DebugOStream() << re << L'\n';
            return re;
        }

        return pcre_compile2_real(pattern, options, errorcodeptr, errorptr, erroroffset, tables);
    }

    size_t len = strlen(pattern) + 1;
    // new and malloc will make the process crash under Debug config (although they are also in the process heap)
    //char* pat = new char[len];
    //char* pat = (char*)malloc(len);
    char* pat = (char*)HeapAlloc(GetProcessHeap(), 0, len);
    memcpy(pat, pattern, len);
    if constexpr (debug)
        DebugOStream() << (void*)pat << L'\n';
    return (pcre*)pat;
}


/*************************************************
*        Return info about compiled pattern      *
*************************************************/

/* This is a newer "info" function which has an extensible interface so
that additional items can be added compatibly.

Arguments:
  argument_re      points to compiled code
  extra_data       points extra data, or NULL
  what             what information is required
  where            where to put the information

Returns:           0 if data returned, negative on error
*/

int PCRE_CALL_CONVENTION
(*pcre_fullinfo_real)(const pcre* argument_re, const pcre_extra* extra_data,
    int what, void* where);

int PCRE_CALL_CONVENTION
pcre_fullinfo_detour(const pcre* argument_re, const pcre_extra* extra_data,
    int what, void* where)
{
    if constexpr (debug)
        DebugOStream() << L"pcre_fullinfo(" << argument_re << L", " << extra_data << L", " << what << L", " << where << L")\n";

    if (modifiers & Modifier::RegEx) {
        return pcre_fullinfo_real(argument_re, extra_data, what, where);
    }

    constexpr int PCRE_INFO_CAPTURECOUNT = 2;
    if (what == PCRE_INFO_CAPTURECOUNT) {
        *(int*)where = 0;
    }
    return 0;
}


/*************************************************
*         Execute a Regular Expression           *
*************************************************/

/* This function applies a compiled re to a subject string and picks out
portions of the string if it matches. Two elements in the vector are set for
each substring: the offsets to the start and end of the substring.

Arguments:
  argument_re     points to the compiled expression
  extra_data      points to extra data or is NULL
  subject         points to the subject string
  length          length of subject string (may contain binary zeros)
  start_offset    where to start in the subject string
  options         option bits
  offsets         points to a vector of ints to be filled in with offsets
  offsetcount     the number of elements in the vector

Returns:          > 0 => success; value is the number of elements filled in
                  = 0 => success, but offsets is not big enough
                   -1 => failed to match
                 < -1 => some kind of unexpected problem
*/

int PCRE_CALL_CONVENTION
(*pcre_exec_real)(const pcre* argument_re, const pcre_extra* extra_data,
    PCRE_SPTR subject, int length, int start_offset, int options, int* offsets,
    int offsetcount);

int PCRE_CALL_CONVENTION
pcre_exec_detour(const pcre* argument_re, const pcre_extra* extra_data,
    PCRE_SPTR subject, int length, int start_offset, int options, int* offsets,
    int offsetcount)
{
    if constexpr (debug) {
        do {
            thread_local const pcre* last_re = nullptr;
            thread_local size_t count = 0;
            if (argument_re == last_re) {
                if (count++ > 10)
                    break;
            } else {
                last_re = argument_re;
                count = 1;
            }

            std::wstring subject_u16(length, L'\0');
            subject_u16.resize(MultiByteToWideChar(CP_UTF8, 0, subject, length, subject_u16.data(), subject_u16.size()));
            DebugOStream() << L"pcre_exec(" << argument_re << L", " << extra_data << LR"(, ")" << subject_u16 << LR"(", )" << length << L", " << start_offset << L", " << std::hex << options << std::dec << L", " << offsets << L", " << offsetcount
                << std::hex << L") on " << GetCurrentThreadId() << L"\n";

            /*
            pcre_exec(0x03CC42A0, nullptr, "test.txt", 8, 0, 0, 0x0020E910, 30)
            pcre_exec(0x03CC42A0, nullptr, "test.txt", 8, 0, 0, 0x0020EB60, 30)
            pcre_exec(0x03CC42A0, nullptr, "est.txt", 7, 0, PCRE_NOTBOL, 0x0020EB60, 30)
            pcre_exec(0x03CC42A0, nullptr, "st.txt", 6, 0, PCRE_NOTBOL, 0x0020EB60, 30)
            pcre_exec(0x03CC42A0, nullptr, "t.txt", 5, 0, PCRE_NOTBOL, 0x0020EB60, 30)
            pcre_exec(0x03CC42A0, nullptr, ".txt", 4, 0, PCRE_NOTBOL, 0x0020EB60, 30)
            pcre_exec(0x03CC42A0, nullptr, "txt", 3, 0, PCRE_NOTBOL, 0x0020EB60, 30)
            pcre_exec(0x03CC42A0, nullptr, "xt", 2, 0, PCRE_NOTBOL, 0x0020EB60, 30)
            pcre_exec(0x03CC42A0, nullptr, "t", 1, 0, PCRE_NOTBOL, 0x0020EB60, 30)
            multi threads
            */
        } while (false);
    }

    if (modifiers & Modifier::RegEx) {
        return pcre_exec_real(argument_re, extra_data, subject, length, start_offset, options, offsets, offsetcount);
    }

    return match((const char8_t*)argument_re, (const char8_t*)subject, length, config.pinyin_search.flags, offsets, offsetcount);
}

/*
bool (*regex_free_real)(void* result);
bool regex_free_detour(void* result) {
    // a2->result18, a2->result20
    if constexpr (debug)
        DebugOStream() << L"regex_free(" << result << L") on " << std::hex << GetCurrentThreadId() << L"\n";
    return regex_free_real(result);
}
*/

/*
auto HeapFree_real = HeapFree;
_Success_(return != FALSE) BOOL WINAPI HeapFree_detour(
    _Inout_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    __drv_freesMem(Mem) _Frees_ptr_opt_ LPVOID lpMem)
{
    if ((uintptr_t)lpMem & 1) {
        if constexpr (debug)
            DebugOStream() << L"HeapFree(lpMem | 1)\n";
        lpMem = (LPVOID)((uintptr_t)lpMem & ~1);
    }
    return HeapFree_real(hHeap, dwFlags, lpMem);
}
*/

PinyinSearchPcre::PinyinSearchPcre() {
    ib::Addr Everything = ib::ModuleFactory::CurrentProcess().base;
    if (ipc_version.major == 1 && ipc_version.minor == 4 && ipc_version.revision == 1 && ipc_version.build == 1009) {
        regex_compile_p2_real = Everything + 0x5CB70;
        pcre_compile2_real = Everything + 0x193340;
        pcre_fullinfo_real = Everything + 0x1A7BF0;
        pcre_exec_real = Everything + 0x1A69E0;
        //regex_free_real = Everything + 0x5D990;
    } else
        throw;

    IbDetourAttach(&regex_compile_p2_real, regex_compile_p2_detour);
    IbDetourAttach(&pcre_compile2_real, pcre_compile2_detour);
    IbDetourAttach(&pcre_fullinfo_real, pcre_fullinfo_detour);
    IbDetourAttach(&pcre_exec_real, pcre_exec_detour);
    //IbDetourAttach(&regex_free_real, regex_free_detour);
    //IbDetourAttach(&HeapFree_real, HeapFree_detour);
}

PinyinSearchPcre::~PinyinSearchPcre() {
    //IbDetourDetach(&HeapFree_real, HeapFree_detour);
    //IbDetourDetach(&regex_free_real, regex_free_detour);
    IbDetourDetach(&pcre_exec_real, pcre_exec_detour);
    IbDetourDetach(&pcre_fullinfo_real, pcre_fullinfo_detour);
    IbDetourDetach(&pcre_compile2_real, pcre_compile2_detour);
    IbDetourDetach(&regex_compile_p2_real, regex_compile_p2_detour);
}