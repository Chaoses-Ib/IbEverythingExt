#include "pch.h"
#include "PinyinSearchPcre.hpp"
#include "config.hpp"
#include "ipc.hpp"
#include "match.hpp"
#include "helper.hpp"

/*
* Region list:
* regcomp_p3
* regcomp_p2
* regcomp
* pcre_compile2 (not in use)
* pcre_fullinfo (not in use)
* regexec
* pcre_exec (not in use)
* free (not in use)
*/

#pragma region regcomp_p3

//bool global_regex;

void regcomp_p3_common(char* search, char* filter) {
    if constexpr (debug)
        DebugOStream() << LR"(regcomp_p3_common(")" << search << LR"(", ")" << filter << LR"("))" << L"\n";
}

uint64_t (*regcomp_p3_14_real)(__int64 a1, int a2, int a3, int a4, int a5, int regex, int a7, int a8, char* search, int a10, char* filter, int sort, int a13, int a14, int a15);
uint64_t regcomp_p3_14_detour(__int64 a1, int a2, int a3, int a4, int a5, int regex, int a7, int a8, char* search, int a10, char* filter, int sort, int a13, int a14, int a15) {
    //global_regex = regex;

    regcomp_p3_common(search, filter);
    return regcomp_p3_14_real(a1, a2, a3, a4, a5, regex /*1*/, a7, a8, search, a10, filter, sort, a13, a14, a15);
}

uint64_t (*regcomp_p3_15_real)(void* a1);
uint64_t regcomp_p3_15_detour(void* a1) {
    /*
    uint32_t* regex = (uint32_t*)a1 + 808;
    global_regex = *regex;
    */

    // enabling global regex will make the entire search content be treated as
    // a regular expression (even if there are modifiers and spaces)
    /*
    if (!global_regex)
        *regex = 1;
    */

    regcomp_p3_common(ib::Addr(a1).offset<uint64_t>(406)[0], ib::Addr(a1).offset<uint64_t>(408)[0]);
    return regcomp_p3_15_real(a1);
}

#pragma endregion


#pragma region regcomp_p2

struct Modifier {
    using Value = uint32_t;
    using T = const Value;

    static T Case = 0x1;  // "Match Case" or `case:`
    static T WholeWord = 0x2;  // "Match Whole Word", `wholeword:` or `ww:`
    static T Path = 0x4;  // "Match Path" or `path:`
    static T Diacritics = 0x8;  // "Match Diacritics" or `diacritics:`
    static T File = 0x10;  // `file:`
    static T Folder = 0x20;  // `folder:`
    static T FastAsciiSearch = 0x40;  // `ascii:` (`utf8:` to disable)

    static T WholeFilename = 0x200;  // `wholefilename:` or `wfn:`

    static T RegEx = 0x800;  // "Enable Regex" or `regex:`
    static T WildcardWholeFilename = 0x1000;  // "Match whole filename when using wildcards"
    static T Alphanumeric = 0x2000;
    static T WildcardEx = 0x4000;  // `wildcards:`
};
Modifier::Value modifiers;
char termtext_initial;

void regcomp_p2_common(Modifier::Value* modifiers_p, char8_t* termtext, size_t* termtext_len) {
    if constexpr (debug) {
        std::wstring termtext_u16(*termtext_len, L'\0');
        termtext_u16.resize(MultiByteToWideChar(CP_UTF8, 0, (char*)termtext, *termtext_len, termtext_u16.data(), termtext_u16.size()));
        DebugOStream() << L"regcomp_p2_common(" << std::hex << *modifiers_p << LR"(, ")" << termtext_u16 << LR"("))"
            << L" on " << GetCurrentThreadId() << L"\n";

        // single thread
    }

    modifiers = *modifiers_p;

    std::u8string_view pattern(termtext, *termtext_len);

    if (pattern.empty())
        return;

    // post-modifiers must be processed first (or it may return with a termtext containing post-modifiers)
    // NoProcess post-modifier
    if (pattern.ends_with(u8";np")) {
        *termtext_len -= 3;
        return;
    }

    // return if term is a regex or term is case sensitive
    if (modifiers & Modifier::RegEx || modifiers & Modifier::Case)
        return;

    // unsupported modifiers
    if (modifiers & Modifier::WildcardEx || modifiers & Modifier::WholeWord)
        return;

    //#TODO: wildcards are not supported
    if (!(modifiers & Modifier::Alphanumeric) && pattern.find_first_of(u8"*?") != pattern.npos)
        return;


    // return if no lower letter
    if (std::find_if(pattern.begin(), pattern.end(), [](char8_t c) {
        return u8'a' <= c && c <= u8'z';
        }) == pattern.end())
    {
        return;
    }
    // could be `CPP;py`, but that's a rare case

    
    // "Match path when a search term contains a path separator"
    if (!(modifiers & Modifier::Path) && (
        std::find(pattern.begin(), pattern.end(), u8'\\') != pattern.end()
        || pattern.size() > 1 && pattern[1] == u8':' && 'A' <= std::toupper(pattern[0]) && std::toupper(pattern[0]) <= 'Z'
    )) {
        *modifiers_p |= Modifier::Path;
    }

    // set regex modifier
    *modifiers_p |= Modifier::RegEx;  // may cause crashes under some versions?

    // bypass fast regex optimazation
    termtext_initial = termtext[0];
    // .\[^$*{?+|()
    // $: invalid when termtext is a single char
    termtext[0] = u8'.';
    //#TODO: or nofastregex: (v1.5.0.1291)
}

#pragma pack(push, 1)
struct regcomp_p2_14 {
    ib::Byte gap0[24];
    void* result18;
    void* result20;
    Modifier::Value modifiers;
    uint32_t int2C;
    char8_t termtext[];
};
#pragma pack(pop)

bool (*regcomp_p2_14_real)(void* a1, regcomp_p2_14* a2);
bool regcomp_p2_14_detour(void* a1, regcomp_p2_14* a2) {
    if constexpr (debug)
        DebugOStream() << L"regcomp_p2_14(" << std::hex << a2 << L"{" << a2->int2C << "})\n";

    size_t termtext_len = strlen((char*)a2->termtext);
    regcomp_p2_common(&a2->modifiers, a2->termtext, &termtext_len);
    a2->termtext[termtext_len] = u8'\0';
    
    if constexpr (debug) {
        bool result = regcomp_p2_14_real(a1, a2);
        DebugOStream() << L"-> {" << a2->result18 << L", " << a2->result20 << L", " << std::hex << a2->modifiers << L"}\n";
        return result;
    }
    return regcomp_p2_14_real(a1, a2);
}

bool (*regcomp_p2_15_real)(void** a1);
bool regcomp_p2_15_detour(void** a1) {
    char8_t* termtext = ib::Addr(a1[1]) + 288;
    size_t* termtext_len = ib::Addr(a1[1]) + 256;
    assert(termtext[*termtext_len] == u8'\0');

    regcomp_p2_common(ib::Addr(a1[1]) + 280, termtext, termtext_len);
    termtext[*termtext_len] = u8'\0';
    return regcomp_p2_15_real(a1);
}

#pragma endregion


#pragma region regcomp

#define PCRE_CALL_CONVENTION

/* Options, mostly defined by POSIX, but with some extras. */

#define REG_ICASE     0x0001   /* Maps to PCRE_CASELESS */
#define REG_NEWLINE   0x0002   /* Maps to PCRE_MULTILINE */
#define REG_NOTBOL    0x0004   /* Maps to PCRE_NOTBOL */
#define REG_NOTEOL    0x0008   /* Maps to PCRE_NOTEOL */
#define REG_DOTALL    0x0010   /* NOT defined by POSIX; maps to PCRE_DOTALL */
#define REG_NOSUB     0x0020   /* Maps to PCRE_NO_AUTO_CAPTURE */
#define REG_UTF8      0x0040   /* NOT defined by POSIX; maps to PCRE_UTF8 */
#define REG_STARTEND  0x0080   /* BSD feature: pass subject string by so,eo */
#define REG_NOTEMPTY  0x0100   /* NOT defined by POSIX; maps to PCRE_NOTEMPTY */
#define REG_UNGREEDY  0x0200   /* NOT defined by POSIX; maps to PCRE_UNGREEDY */
#define REG_UCP       0x0400   /* NOT defined by POSIX; maps to PCRE_UCP */

/* Error values. Not all these are relevant or used by the wrapper. */

enum {
    REG_ASSERT = 1,  /* internal error ? */
    REG_BADBR,       /* invalid repeat counts in {} */
    REG_BADPAT,      /* pattern error */
    REG_BADRPT,      /* ? * + invalid */
    REG_EBRACE,      /* unbalanced {} */
    REG_EBRACK,      /* unbalanced [] */
    REG_ECOLLATE,    /* collation error - not relevant */
    REG_ECTYPE,      /* bad class */
    REG_EESCAPE,     /* bad escape sequence */
    REG_EMPTY,       /* empty expression */
    REG_EPAREN,      /* unbalanced () */
    REG_ERANGE,      /* bad range inside [] */
    REG_ESIZE,       /* expression too big */
    REG_ESPACE,      /* failed to get memory */
    REG_ESUBREG,     /* bad back reference */
    REG_INVARG,      /* bad argument */
    REG_NOMATCH      /* match failed */
};

/* The structure representing a compiled regular expression. */

struct regex_t {
    void* re_pcre;
    size_t re_nsub;
    size_t re_erroffset;
};

/* The structure in which a captured offset is returned. */

using regoff_t = int;

struct regmatch_t {
    regoff_t rm_so;
    regoff_t rm_eo;
};


/*************************************************
*            Compile a regular expression        *
*************************************************/

/*
Arguments:
  preg        points to a structure for recording the compiled expression
  pattern     the pattern to compile
  cflags      compilation flags

Returns:      0 on success
              various non-zero codes on failure
*/

int PCRE_CALL_CONVENTION
(*regcomp_real)(regex_t* preg, const char* pattern, int cflags);

int PCRE_CALL_CONVENTION
regcomp_detour(regex_t* preg, const char* pattern, int cflags)
{
    if constexpr (debug) {
        size_t length = strlen(pattern);
        std::wstring pattern_u16(length, L'\0');
        pattern_u16.resize(MultiByteToWideChar(CP_UTF8, 0, pattern, length, pattern_u16.data(), pattern_u16.size()));
        DebugOStream() << LR"(regcomp(")" << pattern_u16 << LR"(", )" << std::hex << cflags << L")\n";
    }

    if (modifiers & Modifier::RegEx) {
        return regcomp_real(preg, pattern, cflags);
    }

    const_cast<char*>(pattern)[0] = termtext_initial;
    
    preg->re_pcre = compile((const char8_t*)pattern, {
        .match_at_start = bool(modifiers & Modifier::WholeFilename),
        .match_at_end = bool(modifiers & Modifier::WholeFilename)
        }, &config.pinyin_search.flags);
    preg->re_nsub = 0;
    preg->re_erroffset = (size_t)-1;

    if constexpr (debug)
        DebugOStream() << L"-> " << preg->re_pcre << L'\n';

    return 0;
}

#pragma endregion


#pragma region pcre_compile2 (not in use)

using pcre = void;
using pcre_extra = void;
using PCRE_SPTR = const char*;

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

[[deprecated]]
static auto _comment = R"*(
pcre* PCRE_CALL_CONVENTION
(*pcre_compile2_real)(const char* pattern, int options, int* errorcodeptr,
    const char** errorptr, int* erroroffset, const unsigned char* tables);

[[deprecated]]
pcre* PCRE_CALL_CONVENTION
pcre_compile2_detour(const char* pattern, int options, int* errorcodeptr,
    const char** errorptr, int* erroroffset, const unsigned char* tables)
{
    if (!(modifiers & Modifier::RegEx)) {
        const_cast<char*>(pattern)[0] = termtext_initial;
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

    /*
    size_t len = strlen(pattern) + 1;
    // new and malloc will make the process crash under Debug config (although they are also in the process heap)
    //char* pat = new char[len];
    //char* pat = (char*)malloc(len);
    char* pat = (char*)HeapAlloc(GetProcessHeap(), 0, len);
    memcpy(pat, pattern, len);
    if constexpr (debug)
        DebugOStream() << (void*)pat << L'\n';
    return (pcre*)pat;
    */

    return (pcre*)compile((const char8_t*)pattern, 0, &config.pinyin_search.flags);
}
)*";

#pragma endregion


#pragma region pcre_fullinfo (not in use)

// pcre_fullinfo (not in use)

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

/*
int PCRE_CALL_CONVENTION
(*pcre_fullinfo_real)(const pcre* argument_re, const pcre_extra* extra_data,
    int what, void* where);

int PCRE_CALL_CONVENTION
pcre_fullinfo_detour(const pcre* argument_re, const pcre_extra* extra_data,
    int what, void* where)
{
    constexpr int PCRE_INFO_CAPTURECOUNT = 2;

    if constexpr (debug)
        DebugOStream() << L"pcre_fullinfo(" << argument_re << L", " << extra_data << L", " << what << L", " << where << L")\n";

    if (modifiers & Modifier::RegEx) {
        int result = pcre_fullinfo_real(argument_re, extra_data, what, where);
        if constexpr (debug)
            DebugOStream() << *(int*)where << L'\n';
        return result;
    }
    
    if (what == PCRE_INFO_CAPTURECOUNT) {
        *(int*)where = 0;
    }
    return 0;
}
*/

#pragma endregion


#pragma region regexec

/*************************************************
*              Match a regular expression        *
*************************************************/

/* Unfortunately, PCRE requires 3 ints of working space for each captured
substring, so we have to get and release working store instead of just using
the POSIX structures as was done in earlier releases when PCRE needed only 2
ints. However, if the number of possible capturing brackets is small, use a
block of store on the stack, to reduce the use of malloc/free. The threshold is
in a macro that can be changed at configure time.

If REG_NOSUB was specified at compile time, the PCRE_NO_AUTO_CAPTURE flag will
be set. When this is the case, the nmatch and pmatch arguments are ignored, and
the only result is yes/no/error. */

int PCRE_CALL_CONVENTION
(*regexec_real)(const regex_t* preg, const char* string, size_t nmatch,
    regmatch_t pmatch[], int eflags);

int PCRE_CALL_CONVENTION
regexec_detour(const regex_t* preg, const char* string, size_t nmatch,
    regmatch_t pmatch[], int eflags)
{
    if constexpr (debug) {
        do {
            thread_local const regex_t* last_preg = nullptr;
            thread_local size_t count = 0;
            if (preg == last_preg) {
                if (count > 1)
                    break;
                else
                    ++count;
            } else {
                last_preg = preg;
                count = 1;
            }

            //int length = eflags & REG_STARTEND ? pmatch[0].rm_eo : strlen(string);
            int length, start;
            if (eflags & REG_STARTEND) {
                // string is not zero-terminated
                start = pmatch[0].rm_so;
                string += start;
                length = pmatch[0].rm_eo - start;
            } else {
                start = 0;
                length = strlen(string);
            }

            std::wstring string_u16(length, L'\0');
            string_u16.resize(MultiByteToWideChar(CP_UTF8, 0, string, length, string_u16.data(), string_u16.size()));

            auto dout = DebugOStream();
            dout << LR"(regexec(")" << string_u16 << LR"(", )" << std::hex << eflags << std::dec;
            if (eflags & REG_STARTEND)
                dout << L", {" << pmatch[0].rm_so << L"," << pmatch[0].rm_eo << L"}";
            dout << L") -> ";

            int error;
            int rc;
            if (modifiers & Modifier::RegEx) {
                string -= start;
                error = regexec_real(preg, string, nmatch, pmatch, eflags);
                rc = 1 + preg->re_nsub;
            }
            else {
                rc = exec((Pattern*)preg->re_pcre, (const char8_t*)string, length, nmatch, (int*)pmatch, {
                    .not_begin_of_line = bool(eflags & REG_NOTBOL)
                    });
                if (rc == -1)
                    error = REG_NOMATCH;
                else
                    error = 0;
            }
            dout << rc;
            if (error == REG_NOMATCH) {
                dout << L"\n";
                return error;
            }
            
            for (int i = 0; i < rc; i++) {
                dout << L"{" << pmatch[i].rm_so << L"," << pmatch[i].rm_eo << L"}";
            }
            
            dout << L'\n';
            return 0;
        } while (false);
    }

    if (modifiers & Modifier::RegEx) {
        return regexec_real(preg, string, nmatch, pmatch, eflags);
    }

    int length, start;
    if (eflags & REG_STARTEND) {
        // string is not zero-terminated
        start = pmatch[0].rm_so;
        string += start;
        length = pmatch[0].rm_eo - start;
    } else {
        length = strlen(string);
    }

    int rc = exec((Pattern*)preg->re_pcre, (const char8_t*)string, length, nmatch, (int*)pmatch, {
        .not_begin_of_line = bool(eflags & REG_NOTBOL)
        });
    if (rc == -1) {
        return REG_NOMATCH;
    }

    // Everything removes this characteristic from PCRE regexec
    /*
    if (eflags & REG_STARTEND) {
        for (int i = 0; i < rc; i++) {
            pmatch[i].rm_so += start;
            pmatch[i].rm_eo += start;
        }
    }
    */

    return 0;
}

#pragma endregion


#pragma region pcre_exec (not in use)

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

[[deprecated]]
auto _comment1 = R"*(
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
            auto dout = DebugOStream();
            dout << L"pcre_exec(" << argument_re /* << L", " << extra_data */ << LR"(, ")" << subject_u16 << LR"(", )" << length << L", " << start_offset << L", " << std::hex << options << std::dec << L", " << offsets << L", " << offsetcount << L")"
                /* << std::hex << L" on " << GetCurrentThreadId() */;

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

            if (modifiers & Modifier::RegEx) {
                dout << L'\n';
                break;
            }

            int result = match((Pattern*)argument_re, (const char8_t*)subject, length, offsets, offsetcount);
            dout << L" -> " << result;
            if (result > 0)
                dout << L", " << offsets[0] << L", " << offsets[1];
            dout << L'\n';
            return result;
        } while (false);
    }

    if (modifiers & Modifier::RegEx) {
        return pcre_exec_real(argument_re, extra_data, subject, length, start_offset, options, offsets, offsetcount);
    }

    return match((Pattern*)argument_re, (const char8_t*)subject, length, offsets, offsetcount);
}
)*";

#pragma endregion


#pragma region free (not in use)

/*
bool (*regex_free_real)(void* result);
bool regex_free_detour(void* result) {
    // a2->result18, a2->result20
    if constexpr (debug)
        DebugOStream() << L"regex_free(" << result << L") on " << std::hex << GetCurrentThreadId() << L"\n";
    return regex_free_real(result);
}
*/

constexpr bool debug_HeapFree = false;

auto HeapFree_real = HeapFree;
_Success_(return != FALSE) BOOL WINAPI HeapFree_detour(
    _Inout_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    __drv_freesMem(Mem) _Frees_ptr_opt_ LPVOID lpMem)
{
    /*
    if constexpr (debug)
        DebugOStream() << L"HeapFree(" << lpMem << L")\n";  // multi threads
    */
    /*
    if ((uintptr_t)lpMem & 1) {
        if constexpr (debug)
            DebugOStream() << L"HeapFree(lpMem | 1)\n";
        lpMem = (LPVOID)((uintptr_t)lpMem & ~1);
    }
    */

    return HeapFree_real(hHeap, dwFlags, lpMem);
}

#pragma endregion


PinyinSearchPcre::PinyinSearchPcre() {
    bool support = true;
    ib::Addr Everything = ib::ModuleFactory::CurrentProcess().base;
    if (ipc_version.major == 1 && ipc_version.minor == 4 && ipc_version.revision == 1) {
        if (ipc_version.build == 1009) {
            regcomp_p3_14_real = Everything + 0x174C0;
            regcomp_p2_14_real = Everything + 0x5CB70;
            regcomp_real = Everything + 0x1A8E80;
            //pcre_compile2_real = Everything + 0x193340;
            //pcre_fullinfo_real = Everything + 0x1A7BF0;
            regexec_real = Everything + 0x1A8F60;
            //pcre_exec_real = Everything + 0x1A69E0;
            //regex_free_real = Everything + 0x5D990;
        }
        else if (ipc_version.build == 1015) {
            regcomp_p3_14_real = Everything + 0x175A0;
            regcomp_p2_14_real = Everything + 0x5CEC0;
            regcomp_real = Everything + 0x1A8FC0;
            regexec_real = Everything + 0x1A90A0;
        }
        else
            support = false;

        if (support) {
            IbDetourAttach(&regcomp_p3_14_real, regcomp_p3_14_detour);
            IbDetourAttach(&regcomp_p2_14_real, regcomp_p2_14_detour);
        }
    } else if (ipc_version.major == 1 && ipc_version.minor == 5 && ipc_version.revision == 0) {
        if (ipc_version.build == 1296) {
            regcomp_p3_15_real = Everything + 0x2D170;
            regcomp_p2_15_real = Everything + 0xB17A0;
            regcomp_real = Everything + 0x320800;
            regexec_real = Everything + 0x3208E0;
        }
        else
            support = false;

        if (support) {
            IbDetourAttach(&regcomp_p3_15_real, regcomp_p3_15_detour);
            IbDetourAttach(&regcomp_p2_15_real, regcomp_p2_15_detour);
        }
    }
    else
        support = false;
    if (!support)
        throw std::runtime_error("Unsupported Everything version");
        
    IbDetourAttach(&regcomp_real, regcomp_detour);
    //IbDetourAttach(&pcre_compile2_real, pcre_compile2_detour);
    //IbDetourAttach(&pcre_fullinfo_real, pcre_fullinfo_detour);
    IbDetourAttach(&regexec_real, regexec_detour);
    //IbDetourAttach(&pcre_exec_real, pcre_exec_detour);
    //IbDetourAttach(&regex_free_real, regex_free_detour);
    if constexpr (debug_HeapFree)
        IbDetourAttach(&HeapFree_real, HeapFree_detour);
}

PinyinSearchPcre::~PinyinSearchPcre() {
    if constexpr (debug_HeapFree)
        IbDetourDetach(&HeapFree_real, HeapFree_detour);
    //IbDetourDetach(&regex_free_real, regex_free_detour);
    //IbDetourDetach(&pcre_exec_real, pcre_exec_detour);
    IbDetourDetach(&regexec_real, regexec_detour);
    //IbDetourDetach(&pcre_fullinfo_real, pcre_fullinfo_detour);
    //IbDetourDetach(&pcre_compile2_real, pcre_compile2_detour);
    IbDetourDetach(&regcomp_real, regcomp_detour);
    if (ipc_version.major == 1 && ipc_version.minor == 4) {
        IbDetourDetach(&regcomp_p2_14_real, regcomp_p2_14_detour);
        IbDetourDetach(&regcomp_p3_14_real, regcomp_p3_14_detour);
    }
    else if (ipc_version.major == 1 && ipc_version.minor == 5) {
        IbDetourDetach(&regcomp_p2_15_real, regcomp_p2_15_detour);
        IbDetourDetach(&regcomp_p3_15_real, regcomp_p3_15_detour);
    }
}