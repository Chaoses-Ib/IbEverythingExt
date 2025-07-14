#include "pch.h"
#include "PinyinSearchPcre.hpp"
#include <fmt/core.h>
#include <fmt/xchar.h>
#include <Psapi.h>
#include <sigmatch/sigmatch.hpp>
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
* free
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
    
    regcomp_p3_common(ib::Addr(a1)[PinyinSearchPcre::offsets.regcomp_p3_search], ib::Addr(a1)[PinyinSearchPcre::offsets.regcomp_p3_filter]);
    return regcomp_p3_15_real(a1);
}

// uint64_t regcomp_p3_15_0_1318_detour(void* a1) {
//     regcomp_p3_common(ib::Addr(a1).offset<uint64_t>(393)[0], ib::Addr(a1).offset<uint64_t>(395)[0]);
//     return regcomp_p3_15_real(a1);
// }

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

    // return if termtext is an absolute path
    if (pattern.size() > 1 && pattern[1] == u8':' && 'A' <= ib::toupper(pattern[0]) && ib::toupper(pattern[0]) <= 'Z')
        return;
    
    // "Match path when a search term contains a path separator"
    if (!(modifiers & Modifier::Path) && std::find(pattern.begin(), pattern.end(), u8'\\') != pattern.end()) {
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
    char8_t* termtext = ib::Addr(a1[1]) + PinyinSearchPcre::offsets.regcomp_p2_termtext_1;
    size_t* termtext_len = ib::Addr(a1[1]) + PinyinSearchPcre::offsets.regcomp_p2_termtext_0;
    assert(termtext[*termtext_len] == u8'\0');

    regcomp_p2_common(ib::Addr(a1[1]) + PinyinSearchPcre::offsets.regcomp_p2_modifiers, termtext, termtext_len);
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

//using regoff_t = int;
//
//struct regmatch_t {
//    regoff_t rm_so;
//    regoff_t rm_eo;
//};


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
    
    if (config.pinyin_search.mode == PinyinSearchMode::Pcre2) {
        const void* matcher = search_compile(pattern, cflags, modifiers);
        if (!matcher) {
            return regcomp_real(preg, pattern, cflags);
        }
        preg->re_pcre = (void*)((uintptr_t)matcher | 1);
    } else {
        Pattern* compiled_pattern = compile((const char8_t*)pattern, {
            .match_at_start = bool(modifiers & Modifier::WholeFilename),
            .match_at_end = bool(modifiers & Modifier::WholeFilename)
            }, &config.pinyin_search.flags);
        preg->re_pcre = (void*)((uintptr_t)compiled_pattern | 1);
    }

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
    auto do_exec = [&](const char* string, int length) {
        const void* matcher = (const void*)((uintptr_t)preg->re_pcre & ~1);
        if (config.pinyin_search.mode == PinyinSearchMode::Pcre2) {
            return search_exec(matcher, string, length, nmatch, pmatch, eflags);
        } else {
            return exec((Pattern*)matcher, (const char8_t*)string, length, nmatch, (int*)pmatch, {
                .not_begin_of_line = bool(eflags & REG_NOTBOL)
                });
        }
    };  

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
            if (modifiers & Modifier::RegEx || ((uintptr_t)preg->re_pcre & 1) == 0) {
                string -= start;
                error = regexec_real(preg, string, nmatch, pmatch, eflags);
                rc = 1 + preg->re_nsub;
            }
            else {
                rc = do_exec(string, length);
                if (rc == -1)
                    error = REG_NOMATCH;
                else
                    error = 0;

                if (eflags & REG_STARTEND) {
                    for (int i = 0; i < rc; i++) {
                        pmatch[i].rm_so += start;
                        pmatch[i].rm_eo += start;
                    }
                }
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

    if (modifiers & Modifier::RegEx || ((uintptr_t)preg->re_pcre & 1) == 0) {
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

    int rc = do_exec(string, length);
    if (rc == -1) {
        return REG_NOMATCH;
    }

    // `regexec()` conforms to `REG_STARTEND` after v1.5.0.1359
    if (eflags & REG_STARTEND) {
        for (int i = 0; i < rc; i++) {
            pmatch[i].rm_so += start;
            pmatch[i].rm_eo += start;
        }
    }

    return 0;
}

int PCRE_CALL_CONVENTION
regexec_15_0_1359_detour(const regex_t* preg, const char* string, size_t nmatch,
    regmatch_t pmatch[], int eflags)
{
    if (modifiers & Modifier::RegEx || ((uintptr_t)preg->re_pcre & 1) == 0) {
        return regexec_real(preg, string, nmatch, pmatch, eflags);
    }

    int start = 0;
    if (eflags & REG_STARTEND) {
        start = pmatch[0].rm_so;
    }
    int r = regexec_detour(preg, string, nmatch, pmatch, eflags);

    // Everything removes this characteristic from PCRE regexec
    if (r == 0 && eflags & REG_STARTEND) {
        int rc = 1 + preg->re_nsub;
        for (int i = 0; i < rc; i++) {
            pmatch[i].rm_so -= start;
            pmatch[i].rm_eo -= start;
        }
    }

    return r;
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


#pragma region free

/*
bool (*regex_free_real)(void* result);
bool regex_free_detour(void* result) {
    // a2->result18, a2->result20
    if constexpr (debug)
        DebugOStream() << L"regex_free(" << result << L") on " << std::hex << GetCurrentThreadId() << L"\n";
    return regex_free_real(result);
}
*/

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
    if ((uintptr_t)lpMem & 1) {
        if constexpr (debug)
            DebugOStream() << L"HeapFree(" << lpMem << L")\n";
        auto ptr = (uintptr_t)lpMem & ~1;
        if (config.pinyin_search.mode == PinyinSearchMode::Pcre2) {
            search_free((void*)ptr);
        } else {
            delete (Pattern*)ptr;
        }
        return true;
    }

    return HeapFree_real(hHeap, dwFlags, lpMem);
}

#pragma endregion

bool try_match_signatures(int version) {
    using namespace sigmatch_literals;

    /*
    ## regcomp_p3
    E9 ?? ?? ?? ?? 48 8B 8F ?? ?? ?? ?? 33 D2
    - v1.4.1.1018: 0
    - v1.5.0.1296: 1
    - v1.5.0.1315: 1
    - v1.5.0.1318: 1

    ... CC
    v1.4: ~0x175C0
    4C 8B DC                             mov     r11, rsp
    v1.5: ~0x2DB30
    40 56                                push    rsi
    v1.5.0.1318:
    41 54                                push    r12
    41 57                                push    r15

    0xBD:
    49 8B 87 F0 0C 00 00                 mov     rax, [r15+0CF0h]
    0F B7 08                             movzx   ecx, word ptr [rax]
    41 8B 87 F8 0C 00 00                 mov     eax, [r15+0CF8h]
    4D 8B 8F C0 0C 00 00                 mov     r9, [r15+0CC0h]
    4D 8B 87 B0 0C 00 00                 mov     r8, [r15+0CB0h]
    89 44 24 28                          mov     dword ptr [rsp+4B8h+var_490], eax
    89 4C 24 20                          mov     dword ptr [rsp+4B8h+var_498], ecx
    48 8D 15 FD A1 30 00                 lea     rdx, aSearchSFilterS ; "search '%s' filter '%s' sort %d ascendi"...
    B9 00 FF 00 FF                       mov     ecx, 0FF00FF00h
    E8 BB 6F 0F 00                       call    @log
    

    0x3A7:
    41 80 F8 65                          cmp     r8b, 65h ; 'e'
    48 8D 4F 30                          lea     rcx, [rdi+30h]
    48 8D 15 62 DE 19 00                 lea     rdx, aEmpty     ; "empty:"
    41 80 F8 65 48 8D 4F ?? 48 8D 15 ?? ?? ?? ??
    - v1.4.1.1017: 1
    - v1.5.0.1315: 0

    0x4D5:
    40 38 35 F0 78 3D 00                 cmp     cs:byte_14040590A, sil
    74 05                                jz      short loc_14002E021
    41 0F BA E9 1E                       bts     r9d, 1Eh
    0F BA EB 1E
    - v1.4.1.1017: 0
    - v1.5.0.1315: 1
    40 38 35 ?? ?? ?? ?? 74 05 41 0F BA E9 1E
    - v1.5.0.1296: 0
    - v1.5.0.1315: 1

    0x1630:
    41 F7 C3 02 00 1C 00                 test    r11d, 1C0002h
    74 43                                jz      short loc_14002F242
    41 F7 C3 02 00 1C 00 74 ??
    - v1.4.1.1017: 0
    - v1.5.0.1296: 1
    - v1.5.0.1315: 1


    ## regcomp_p2
    ... CC
    v1.4: ~0x5CEF0
    48 89 74 24 18                       mov     [rsp+arg_10], rsi
    48 89 74 24 ??

    v1.5: ~0xB7630
    4C 8B DC                             mov     r11, rsp

    4C 8B DC 53 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 84 24 ?? ?? ?? ?? 48 8B 41 08
    - v1.4.1.1018: 0
    - v1.5.0.1296: 1
    - v1.5.0.1315: 1
    - v1.5.0.1318: 1

    0xD:
    8B 52 28                             mov     edx, [rdx+28h]
    48 8B F9                             mov     rdi, rcx
    0F BA E2 0B                          bt      edx, 0Bh
    0F 83 53 06 00 00                    jnb     loc_14005D560
    44 0F B6 46 30                       movzx   r8d, byte ptr [rsi+48]
    8B 52 28 48 8B F9 0F BA E2 0B 0F 83 ?? ?? ?? ?? 44 0F B6 46 30
    - v1.4.1.1017: 1

    0x24:
    0F BA A0 18 01 00 00 0B              bt      dword ptr [rax+280], 0Bh
    - v1.5.0.1315: 1


    ## regcomp
    ... CC
    v1.4: ~0x1A8FC0, v1.5: ~0x348820
    40 53                                push    rbx

    40 53 48 83 EC 40 45 33 C9
    - v1.4.1.1018: 1
    - v1.5.1.1296: 1
    - v1.5.1.1315: 1
    - v1.5.1.1318: 1

    0x49:
    41 0F BA E0 0A                       bt      r8d, 10
    73 04                                jnb     short loc_1401A9014
    0F BA EA 1D                          bts     edx, 1Dh
    41 0F BA E0 0A
    - v1.4.1.1017: 25
    - v1.5.0.1315: 2
    0F BA EA 1D
    - v1.4.1.1017: 1
    - v1.5.0.1315: 1
    41 0F BA E0 0A 73 04 0F BA EA 1D
    - v1.4.1.1017: 1
    - v1.5.0.1315: 1

    41 0F BA E0 09                       bt      r8d, 9
    - v1.4.1.1017: 4
    - v1.5.0.1315: 1

    83 F8 57                             cmp     eax, 87
    - v1.5.0.1315: 4


    ## regexec
    E8 ? ? ? ? 85 C0 0F 94 C3
    - v1.4.1.1018: 1
    - v1.5.0.1296: 1
    - v1.5.0.1318: 1

    ... CC
    v1.4: ~0x1A90A0, v1.5: ~0x348900
    48 89 5C 24 18                       mov     [rsp+arg_10], rbx
    48 89 5C 24 ??

    0x91, 0x97:
    49 81 F8 AA AA AA 0A                 cmp     r8, 0AAAAAAAh
    76 0A                                jbe     short loc_1403489A4
    B8 0E 00 00 00                       mov     eax, 0Eh        ; jumptable 0000000140310291 cases -8,-6
    E9 32 01 00 00                       jmp     loc_140348AD6
    49 81 F8 AA AA AA 0A 76 0A B8 0E 00 00 00 E9 ?? ?? ?? ??
    - v1.4.1.1017: 1
    - v1.5.0.1315: 1
    - v1.5.0.1318: 1
    */

    MODULEINFO info;
    if (!GetModuleInformation(GetCurrentProcess(), ib::ModuleFactory::current_process().handle, &info, sizeof info))
        return false;
    
    sigmatch::this_process_target target;
    sigmatch::search_context context = target.in_range({ info.lpBaseOfDll, info.SizeOfImage });

    /*
    static auto find_entry_CC = [](const std::byte* p_) {
        ib::Addr p = (void*)p_;
        while (p.read<uint16_t>() != 0xCCCC)
            p -= 1;
        return p + 2;
    };
    */
    
    auto search = [&target, &context](sigmatch::signature sig) -> ib::Addr {
        std::vector<const std::byte*> matches = context.search(sig).matches();
        if (matches.empty()) {
            if constexpr (debug)
                DebugOStream() << L"search: Found zero result\n";
            return nullptr;
        }
        if constexpr (debug)
            if (matches.size() > 1)
                DebugOStream() << L"search: Found more than one results\n";
        return (void*)matches[0];
    };
    auto search_entry = [&target, &context, &search](sigmatch::signature sig, sigmatch::signature entry_sig, size_t entry_range) -> ib::Addr {
        ib::Addr code = search(sig);
        if (!code)
            return nullptr;
        
        // not stable
        //return find_entry_CC(matches[0]);

        std::vector<const std::byte*> matches = target.in_range({ code - entry_range, entry_range }).search(entry_sig).matches();
        if (matches.empty())
            return nullptr;

        return (void*)matches.back();
    };
    auto search_call = [&target, &context, &search]<typename offset>(sigmatch::signature sig, size_t call_offset) -> ib::Addr {
        ib::Addr code = search(sig);
        if (!code)
            return nullptr;

        ib::Addr offset_addr = code + call_offset;
        ib::Addr eip = offset_addr + sizeof(offset);
        return eip + *(offset*)offset_addr;
    };

    switch (version) {
    case 4:
        if (!(regcomp_p3_14_real = search_entry("41 80 F8 65 48 8D 4F ?? 48 8D 15"_sig, "4C 8B DC"_sig, 0xA00)))
            return false;
        if (!(regcomp_p2_14_real = search_entry("8B 52 28 48 8B F9 0F BA E2 0B 0F 83 ?? ?? ?? ?? 44 0F B6 46 30"_sig, "48 89 74 24"_sig, 0x30)))
            return false;
        break;
    case 5:
        if (!(regcomp_p3_15_real = search_call.template operator()<int32_t>("E9 ?? ?? ?? ?? 48 8B 8F ?? ?? ?? ?? 33 D2"_sig, 1))) {
            if (!(regcomp_p3_15_real = search_entry("41 F7 C3 02 00 1C 00 74"_sig, "40 56"_sig, 0x3000)))  // < v1.5.0.1318
                return false;
        }
        if (!(regcomp_p2_15_real = search("4C 8B DC 53 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 84 24 ?? ?? ?? ?? 48 8B 41 08"_sig))) {
            if (!(regcomp_p2_15_real = search_entry("0F BA A0 18 01 00 00 0B"_sig, "4C 8B DC"_sig, 0x50)))
                return false;
        }
        break;
    }
    
    if (!(regcomp_real = search("40 53 48 83 EC 40 45 33 C9"_sig))) {
        if (!(regcomp_real = search_entry("41 0F BA E0 0A 73 04 0F BA EA 1D"_sig, "40 53"_sig, 0xA0)))
            return false;
    }
    if (!(regexec_real = search_entry("49 81 F8 AA AA AA 0A 76 0A B8 0E 00 00 00 E9"_sig, "48 89 5C 24"_sig, 0x140))) {
        if (!(regexec_real = search_call.template operator()<int32_t>("E8 ?? ?? ?? ?? 85 C0 0F 94 C3"_sig, 1)))
            return false;
    }

    if constexpr (debug) {
        switch (version) {
        case 4:
            DebugOStream() << fmt::format(
LR"(regcomp_p3_14_real = Everything + 0x{:X};
regcomp_p2_14_real = Everything + 0x{:X};
regcomp_real = Everything + 0x{:X};
regexec_real = Everything + 0x{:X};
)", ib::Addr(regcomp_p3_14_real) - info.lpBaseOfDll,
    ib::Addr(regcomp_p2_14_real) - info.lpBaseOfDll,
    ib::Addr(regcomp_real) - info.lpBaseOfDll,
    ib::Addr(regexec_real) - info.lpBaseOfDll);
            break;
        case 5:
            DebugOStream() << fmt::format(
LR"(regcomp_p3_15_real = Everything + 0x{:X};
regcomp_p2_15_real = Everything + 0x{:X};
regcomp_real = Everything + 0x{:X};
regexec_real = Everything + 0x{:X};
)", ib::Addr(regcomp_p3_15_real) - info.lpBaseOfDll,
    ib::Addr(regcomp_p2_15_real) - info.lpBaseOfDll,
    ib::Addr(regcomp_real) - info.lpBaseOfDll,
    ib::Addr(regexec_real) - info.lpBaseOfDll);
        }
    }

    return true;
}


PinyinSearchPcre::PinyinSearchPcre() {
    if constexpr (debug)
        DebugOStream() << "PinyinSearchPcre()\n";

    ib::Addr Everything = ib::ModuleFactory::current_process().base;

    offsets = get_everything_exe_offsets();
    regcomp_p3_14_real = offsets.regcomp_p3 ? Everything + offsets.regcomp_p3 : nullptr;
    regcomp_p3_15_real = offsets.regcomp_p3 ? Everything + offsets.regcomp_p3 : nullptr;
    regcomp_p2_14_real = offsets.regcomp_p2 ? Everything + offsets.regcomp_p2 : nullptr;
    regcomp_p2_15_real = offsets.regcomp_p2 ? Everything + offsets.regcomp_p2 : nullptr;
    regcomp_real = offsets.regcomp ? Everything + offsets.regcomp : nullptr;
    regexec_real = offsets.regexec ? Everything + offsets.regexec : nullptr;

    bool support = regcomp_p3_14_real && regcomp_p2_14_real && regcomp_real && regexec_real;
    if constexpr (debug)
        DebugOStream() << "support: " << support;

    if (ipc_version.major == 1 && ipc_version.minor == 4) {
        if (ipc_version.revision == 1) {
            if (ipc_version.build == 1009) {
                regcomp_p3_14_real = Everything + 0x174C0;
                regcomp_p2_14_real = Everything + 0x5CB70;
                regcomp_real = Everything + 0x1A8E80;
                //pcre_compile2_real = Everything + 0x193340;
                //pcre_fullinfo_real = Everything + 0x1A7BF0;
                regexec_real = Everything + 0x1A8F60;
                //pcre_exec_real = Everything + 0x1A69E0;
                //regex_free_real = Everything + 0x5D990;
                support = true;
            }
            else if (ipc_version.build == 1015) {
                regcomp_p3_14_real = Everything + 0x175A0;
                regcomp_p2_14_real = Everything + 0x5CEC0;
                regcomp_real = Everything + 0x1A8FC0;
                regexec_real = Everything + 0x1A90A0;
                support = true;
            }
            else if (ipc_version.build == 1017) {
                regcomp_p3_14_real = Everything + 0x175C0;
                regcomp_p2_14_real = Everything + 0x5CEF0;
                regcomp_real = Everything + 0x1A8FC0;
                regexec_real = Everything + 0x1A90A0;
                support = true;
            }
            else if (ipc_version.build == 1018) {
                regcomp_p3_14_real = Everything + 0x175A0;
                regcomp_p2_14_real = Everything + 0x5CEB0;
                regcomp_real = Everything + 0x1A8E80;
                regexec_real = Everything + 0x1A8F60;
                support = true;
            }
            else if (ipc_version.build == 1020) {
                regcomp_p3_14_real = Everything + 0x175A0;
                regcomp_p2_14_real = Everything + 0x5CEB0;
                regcomp_real = Everything + 0x1A8E80;
                regexec_real = Everything + 0x1A8F60;
                support = true;
            }
        }

        if (!support)
            support = try_match_signatures(4);

        if (support) {
            IbDetourAttach(&regcomp_p3_14_real, regcomp_p3_14_detour);
            IbDetourAttach(&regcomp_p2_14_real, regcomp_p2_14_detour);

            IbDetourAttach(&regexec_real, regexec_15_0_1359_detour);
        }
    } else if (ipc_version.major == 1 && ipc_version.minor >= 5) {
        if (ipc_version.minor == 5 && ipc_version.revision == 0) {
            if (ipc_version.build == 1296) {
                regcomp_p3_15_real = Everything + 0x2D170;
                regcomp_p2_15_real = Everything + 0xB17A0;
                regcomp_real = Everything + 0x320800;
                regexec_real = Everything + 0x3208E0;
                support = true;
            }
            else if (ipc_version.build == 1305) {
                regcomp_p3_15_real = Everything + 0x2D920;
                regcomp_p2_15_real = Everything + 0xB44D0;
                regcomp_real = Everything + 0x336880;
                regexec_real = Everything + 0x336960;
                support = true;
            }
            else if (ipc_version.build == 1315) {
                regcomp_p3_15_real = Everything + 0x2DB30;
                regcomp_p2_15_real = Everything + 0xB7630;
                regcomp_real = Everything + 0x348820;
                regexec_real = Everything + 0x348900;
                support = true;
            }
            else if (ipc_version.build == 1318) {
                regcomp_p3_15_real = Everything + 0x319C0;
                regcomp_p2_15_real = Everything + 0xC1F00;
                regcomp_real = Everything + 0x35AA30;
                regexec_real = Everything + 0x35AB10;
                support = true;
            }
        }

        if (!support)
            support = try_match_signatures(5);

        if (support) {
            //if (ipc_version.revision == 0 && ipc_version.build < 1318) {
            //    IbDetourAttach(&regcomp_p3_15_real, regcomp_p3_15_detour);
            //}
            //else {
            //    IbDetourAttach(&regcomp_p3_15_real, regcomp_p3_15_0_1318_detour);
            //}
            IbDetourAttach(&regcomp_p3_15_real, regcomp_p3_15_detour);
            
            IbDetourAttach(&regcomp_p2_15_real, regcomp_p2_15_detour);

            if (ipc_version.minor == 5 && ipc_version.revision == 0 && ipc_version.build <= 1359) {
                IbDetourAttach(&regexec_real, regexec_15_0_1359_detour);
            } else {
                IbDetourAttach(&regexec_real, regexec_detour);
            }
        }
    }
    if (!support)
        throw std::runtime_error("Unsupported Everything version");

    IbDetourAttach(&regcomp_real, regcomp_detour);
    //IbDetourAttach(&pcre_compile2_real, pcre_compile2_detour);
    //IbDetourAttach(&pcre_fullinfo_real, pcre_fullinfo_detour);
    //IbDetourAttach(&regexec_real, regexec_detour);
    //IbDetourAttach(&pcre_exec_real, pcre_exec_detour);
    //IbDetourAttach(&regex_free_real, regex_free_detour);
    IbDetourAttach(&HeapFree_real, HeapFree_detour);
}

PinyinSearchPcre::~PinyinSearchPcre() {
    IbDetourDetach(&HeapFree_real, HeapFree_detour);
    //IbDetourDetach(&regex_free_real, regex_free_detour);
    //IbDetourDetach(&pcre_exec_real, pcre_exec_detour);
    //IbDetourDetach(&regexec_real, regexec_detour);
    //IbDetourDetach(&pcre_fullinfo_real, pcre_fullinfo_detour);
    //IbDetourDetach(&pcre_compile2_real, pcre_compile2_detour);
    IbDetourDetach(&regcomp_real, regcomp_detour);
    if (ipc_version.major == 1 && ipc_version.minor == 4) {
        IbDetourDetach(&regcomp_p2_14_real, regcomp_p2_14_detour);
        IbDetourDetach(&regcomp_p3_14_real, regcomp_p3_14_detour);

        IbDetourAttach(&regexec_real, regexec_15_0_1359_detour);
    }
    else if (ipc_version.major == 1 && ipc_version.minor >= 5) {
        IbDetourDetach(&regcomp_p2_15_real, regcomp_p2_15_detour);
        IbDetourDetach(&regcomp_p3_15_real, regcomp_p3_15_detour);

        if (ipc_version.minor == 5 && ipc_version.revision == 0 && ipc_version.build <= 1359) {
            IbDetourDetach(&regexec_real, regexec_15_0_1359_detour);
        } else {
            IbDetourDetach(&regexec_real, regexec_detour);
        }
    }
}