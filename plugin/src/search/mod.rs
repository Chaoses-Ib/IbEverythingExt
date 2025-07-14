//! i.e. PCRE 2 mode
//!
//! https://www.pcre.org/original/doc/html/pcreposix.html

use core::str;
use std::{
    borrow::Cow,
    ffi::{CStr, c_char, c_void},
    fmt::Debug,
    slice,
};

use bitflags::bitflags;
use everything_plugin::{ipc::Version, log::*};
use ib_matcher::matcher::{IbMatcher, PinyinMatchConfig, input::Input};

use crate::HANDLER;

#[derive(Clone, Copy, PartialEq, Eq)]
#[repr(transparent)]
pub struct Modifiers(u32);
bitflags! {
    impl Modifiers: u32 {
        /// "Match Case" or `case:`
        const Case = 0x1;

        /// "Match Whole Word", `wholeword:` or `ww:`
        const WholeWord = 0x2;

        /// "Match Path" or `path:`
        const Path = 0x4;

        /// "Match Diacritics" or `diacritics:`
        const Diacritics = 0x8;

        /// `file:`
        const File = 0x10;
        /// `folder:`
        const Folder = 0x20;

        /// `ascii:` (`utf8:` to disable)
        const v4_FastAsciiSearch = 0x40;
        /// `startwith:`
        ///
        /// Even `StartWith` and `EndWith` are both used, `WholeFilename` won't be set.
        ///
        /// v1.4 just doesn't go through `regcomp_p2`.
        const v5_StartWith = 0x40;
        /// `endwith:`
        ///
        /// Even `StartWith` and `EndWith` are both used, `WholeFilename` won't be set.
        ///
        /// v1.4 just doesn't go through `regcomp_p2`.
        const v5_EndWith = 0x80;

        /// `wholefilename:` or `wfn:`
        const WholeFilename = 0x200;

        /// "Enable Regex" or `regex:`
        const RegEx = 0x800;
        /// "Match whole filename when using wildcards"
        const WildcardWholeFilename = 0x1000;
        const Alphanumeric = 0x2000;
        /// `wildcards:`
        const WildcardEx = 0x4000;

        /// "Match Prefix" or `prefix:`
        ///
        /// If `Prefix` and `Suffix` are both used, only `WholeWord` will be set.
        const Prefix = 0x20000;
        /// "Match Suffix" or `suffix:`
        ///
        /// If `Prefix` and `Suffix` are both used, only `WholeWord` will be set.
        const Suffix = 0x40000;
    }
}

impl Debug for Modifiers {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "0x{:x}", self.0)
    }
}

/// See https://www.pcre.org/original/doc/html/pcre_compile.html
///
/// Usually 0x441
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(transparent)]
pub struct PcreFlags(u32);
bitflags! {
    impl PcreFlags: u32 {
        /// Maps to PCRE_CASELESS
        const REG_ICASE = 0x0001;
        /// Maps to PCRE_MULTILINE
        const REG_NEWLINE = 0x0002;
        /// Maps to PCRE_NOTBOL
        const REG_NOTBOL = 0x0004;
        /// Maps to PCRE_NOTEOL
        const REG_NOTEOL = 0x0008;
        /// NOT defined by POSIX; maps to PCRE_DOTALL
        const REG_DOTALL = 0x0010;
        /// Maps to PCRE_NO_AUTO_CAPTURE
        const REG_NOSUB = 0x0020;
        /// NOT defined by POSIX; maps to PCRE_UTF8
        const REG_UTF8 = 0x0040;
        /// BSD feature: pass subject string by so,eo
        const REG_STARTEND = 0x0080;
        /// NOT defined by POSIX; maps to PCRE_NOTEMPTY
        const REG_NOTEMPTY = 0x0100;
        /// NOT defined by POSIX; maps to PCRE_UNGREEDY
        const REG_UNGREEDY = 0x0200;
        /// NOT defined by POSIX; maps to PCRE_UCP
        const REG_UCP = 0x0400;
    }
}

#[unsafe(no_mangle)]
extern "C" fn search_compile(
    pattern: *const c_char,
    cflags: PcreFlags,
    mut modifiers: Modifiers,
) -> *const c_void {
    let pattern = unsafe { CStr::from_ptr(pattern) }.to_string_lossy();
    debug!(?pattern, ?cflags, ?modifiers, "Compiling IbMatcher");
    let app = unsafe { HANDLER.app() };
    if app.version() < Version::new(1, 5, 0, 0) {
        modifiers.remove(Modifiers::v5_StartWith | Modifiers::v5_EndWith);
    }
    let matcher = IbMatcher::builder(pattern.as_ref())
        .case_insensitive(cflags.contains(PcreFlags::REG_ICASE))
        .starts_with(modifiers.intersects(Modifiers::v5_StartWith | Modifiers::WholeFilename))
        .ends_with(modifiers.intersects(Modifiers::v5_EndWith | Modifiers::WholeFilename))
        // TODO
        .is_pattern_partial(true)
        .pinyin(
            PinyinMatchConfig::builder(app.config.pinyin_search.notations())
                .data(&app.pinyin_data)
                // TODO
                // .case_insensitive(app.config.pinyin_search.)
                .allow_partial_pattern(false)
                .build(),
        )
        .maybe_romaji(app.romaji.clone())
        .analyze(true)
        .build();
    let r = Box::new(matcher);
    Box::into_raw(r) as _
}

#[allow(non_camel_case_types)]
type regoff_t = i32;

/// The structure in which a captured offset is returned.
#[allow(non_camel_case_types)]
#[repr(C)]
struct regmatch_t {
    rm_so: regoff_t,
    rm_eo: regoff_t,
}

#[unsafe(no_mangle)]
extern "C" fn search_exec(
    matcher: *const c_void,
    haystack: *const c_char,
    length: u32,
    nmatch: usize,
    pmatch: *mut regmatch_t,
    eflags: PcreFlags,
) -> i32 {
    let matcher = unsafe { &*(matcher as *const IbMatcher) };

    let haystack = unsafe { slice::from_raw_parts(haystack as _, length as usize) };
    let buf;
    let haystack = if cfg!(debug_assertions) {
        buf = String::from_utf8_lossy(haystack);
        if let Cow::Owned(_) = &buf {
            error!(?haystack, ?buf, "haystack invalid utf8");
        }
        buf.as_ref()
    } else {
        // TODO: Optimization
        // buf = String::from_utf8_lossy(haystack);
        // buf.as_ref()

        unsafe { str::from_utf8_unchecked(haystack) }
    };

    if let Some(m) = matcher.find(
        Input::builder(haystack)
            .no_start(eflags.contains(PcreFlags::REG_NOTBOL))
            .build(),
    ) {
        if nmatch > 0 {
            unsafe {
                (*pmatch).rm_so = m.start() as _;
                (*pmatch).rm_eo = m.end() as _;
            }
            1
        } else {
            0
        }
    } else {
        -1
    }

    // pysse

    // found 1 files with 24 threads in 0.021186 seconds
    // found 0 folders with 24 threads in 0.002326 seconds

    // c++
    // found 2 files with 24 threads in 0.292198 seconds
    // found 0 folders with 24 threads in 0.032536 seconds
    // found 2 files with 24 threads in 0.036560 seconds
    // found 0 folders with 24 threads in 0.003925 seconds
    // found 2 files with 24 threads in 0.036331 seconds
    // found 0 folders with 24 threads in 0.003879 seconds
    // found 2 files with 24 threads in 0.036273 seconds
    // found 0 folders with 24 threads in 0.003875 seconds

    // pcre2
    // found 2 files with 24 threads in 0.298361 seconds
    // found 0 folders with 24 threads in 0.033580 seconds
    // found 2 files with 24 threads in 0.282134 seconds
    // found 0 folders with 24 threads in 0.031068 seconds

    // length
    // found 2 files with 24 threads in 0.172370 seconds
    // found 0 folders with 24 threads in 0.015033 seconds
    // found 2 files with 24 threads in 0.175381 seconds
    // found 0 folders with 24 threads in 0.015673 seconds
    // debug
    // found 2 files with 24 threads in 0.937618 seconds
    // found 0 folders with 24 threads in 0.077921 seconds

    // Box dyn
    // found 2 files with 24 threads in 0.136043 seconds
    // found 0 folders with 24 threads in 0.012813 seconds
    // found 2 files with 24 threads in 0.133395 seconds
    // found 0 folders with 24 threads in 0.012782 seconds

    // str::from_utf8_unchecked
    // found 2 files with 24 threads in 0.126010 seconds
    // found 0 folders with 24 threads in 0.011448 seconds

    // min_haystack_len
    // found 2 files with 24 threads in 0.130524 seconds
    // found 0 folders with 24 threads in 0.011800 seconds

    // sub_test is_ascii fast fail optimization

    // ac
    // found 2 files with 24 threads in 0.025432 seconds
    // found 0 folders with 24 threads in 0.003713 seconds

    // REG_STARTEND
    // found 2 files with 24 threads in 0.014172 seconds
    // found 0 folders with 24 threads in 0.001881 seconds
    // found 2 files with 24 threads in 0.013749 seconds
    // found 0 folders with 24 threads in 0.001856 seconds
    // np:
    // found 1 files with 24 threads in 0.008721 seconds
    // found 0 folders with 24 threads in 0.001185 seconds
}

#[unsafe(no_mangle)]
extern "C" fn search_free(matcher: *mut c_void) {
    drop(unsafe { Box::from_raw(matcher as *mut IbMatcher) });
}
