use std::{ops::Range, path::Path};

use bon::bon;
use pelite::{
    pattern::Atom::{self, *},
    pe::Rva,
};

#[allow(unused_imports)]
use std::{
    eprintln as error, eprintln as warn, eprintln as info, eprintln as debug, eprintln as trace,
};

/// TODO: Cache
pub struct EverythingExe<'a> {
    pe: pelite::PeFile<'a>,
}

#[bon]
impl<'a> EverythingExe<'a> {
    pub fn from_bytes(bytes: &'a [u8]) -> Result<Self, pelite::Error> {
        let pe = pelite::PeFile::from_bytes(bytes)?;
        Ok(Self { pe })
    }

    fn exception(
        &'a self,
    ) -> Option<pelite::pe64::exception::Exception<'a, pelite::pe64::PeFile<'a>>> {
        let e = self.pe.exception().ok()?;
        match e {
            pelite::Wrap::T32(_) => unimplemented!(),
            pelite::Wrap::T64(e) => Some(e),
        }
    }

    fn exception_next(&self, f: Rva) -> Option<Rva> {
        let e = self.exception()?;
        // TODO
        // let i = e.index_of(f).ok()?;
        let i = e
            .functions()
            .position(|fun| fun.image().BeginAddress == f)?;
        Some(e.functions().nth(i + 1).unwrap().image().BeginAddress)
    }

    fn filter_one(rvas: Vec<Rva>) -> Option<Rva> {
        if rvas.len() == 1 {
            Some(rvas[0])
        } else {
            warn!("None or too many matches: {rvas:?}");
            None
        }
    }

    /// Can avoid overlap.
    #[builder]
    fn match_first_with_save(
        &self,
        #[builder(start_fn)] pattern: &[Atom],
        save: &mut [u32],
        range: Option<Range<Rva>>,
    ) -> Option<Rva> {
        if false {
            dbg!(self.match_many_with_save(pattern).save(save).call());
        }
        let mut matches = self.pe.scanner().matches(
            pattern,
            range.unwrap_or_else(|| self.pe.headers().code_range()),
        );
        if matches.next(save) {
            debug!("match: {save:X?}");
            Some(save[0])
        } else {
            None
        }
    }

    #[builder]
    fn match_many_with_save(
        &self,
        #[builder(start_fn)] pattern: &[Atom],
        save: &mut [u32],
    ) -> Vec<Rva> {
        let mut matches = self.pe.scanner().matches_code(pattern);
        let mut rvas = Vec::new();
        while matches.next(save) {
            debug!("match: {save:X?}");
            let rva = save[0];
            rvas.push(rva);
        }
        rvas
    }

    fn match_one_with_save(&self, pattern: &[Atom], save: &mut [u32]) -> Option<Rva> {
        Self::filter_one(self.match_many_with_save(pattern).save(save).call())
    }

    fn match_one(&self, pattern: &[Atom]) -> Option<Rva> {
        let mut save = [0];
        self.match_one_with_save(pattern, &mut save)
    }

    pub fn regcomp_p3(&self) -> Option<Rva> {
        let mut save = [0];
        // Or: callee of regcomp_p2
        self.match_first_with_save(&[
            Aligned(4),
            Save(0),
            // v1.4.1.1009_x64
            // 4C 8B DC  mov     r11, rsp
            // v1.5.0.1315_x64
            // 40 56     push    rsi
            // v1.5.0.1384_x64
            // 41 55     push    r13
            Case(4),
            Byte(0x4C),
            Byte(0x8B),
            Byte(0xDC),
            Break(4),
            Fuzzy(0xFE),
            Byte(0x40),
            Fuzzy(0xFC),
            Byte(0x56),
            // v1.4.1.1009_x64
            // 12
            // 48 83 EC 40           sub     rsp, 40h
            // v1.5.0.1315_x64
            // 4
            // 48 81 EC B8 04 00 00  sub     rsp, 4B8h
            Many(16),
            Byte(0x48),
            Skip(1),
            Byte(0xEC),
            // v1.4.1.1009_x64
            // 83
            // v1.5.0.1315_x64
            // 227
            Rangext(2),
            Many(0),
            Jump4,
            // "search '%s' filter '%s' sort %d
            Byte(b's'),
            Byte(b'e'),
            Byte(b'a'),
            Byte(b'r'),
            Byte(b'c'),
            Byte(b'h'),
            Byte(b' '),
            Byte(b'\''),
            Byte(b'%'),
            Byte(b's'),
            Byte(b'\''),
            Byte(b' '),
            Byte(b'f'),
            Byte(b'i'),
            Byte(b'l'),
            Byte(b't'),
            Byte(b'e'),
            Byte(b'r'),
            Byte(b' '),
            Byte(b'\''),
            Byte(b'%'),
            Byte(b's'),
            Byte(b'\''),
            Byte(b' '),
            Byte(b's'),
            Byte(b'o'),
            Byte(b'r'),
            Byte(b't'),
            Byte(b' '),
            Byte(b'%'),
            Byte(b'd'),
        ])
        .save(&mut save)
        .call()
    }

    pub fn regcomp_p3_search(&self) -> Option<usize> {
        let mut save = [0, 0];
        let regcomp_p3 = self.regcomp_p3()?;
        self.match_first_with_save(&[
            // v1.5.0.1315_x64
            // 49 39 B7 B0 0C 00 00                    cmp     [r15+3248], rsi
            // 41 89 B7 6C 0D 00 00                    mov     [r15+3436], esi
            // v1.5.0.1318_x64
            // 4D 39 A7 48 0C 00 00                 cmp     [r15+3144], r12
            // 45 89 A7 04 0D 00 00                 mov     [r15+3332], r12d
            // v1.5.0.1384_x64
            // 4D 39 BE 68 0C 00 00                    cmp     [r14+3176], r15
            // 45 89 AE 58 1D 00 00                    mov     [r14+7512], r13d
            // 45 89 BE 30 0D 00 00                    mov     [r14+3376], r15d
            Fuzzy(0x80),
            Byte(0x49),
            Byte(0x39),
            Fuzzy(0xE0),
            Byte(0xB7),
            ReadU32(0),
            // Byte(0x41),
            // Byte(0x89),
            // Byte(0xB7),
            // ReadU32(1),
        ])
        .save(&mut save)
        .range(regcomp_p3..regcomp_p3 + 256)
        .call()?;
        Some(save[0] as usize)
    }

    pub fn regcomp_p3_filter(&self) -> Option<usize> {
        self.regcomp_p3_search().map(|search| search + 16)
    }

    pub fn regcomp_p(&self) -> Option<Rva> {
        let mut save = [0, self.regcomp()?];
        self.match_many_with_save(&[
            Aligned(2),
            Save(0),
            // v1.4.1.1009_x64
            // 1A8E80
            // 48 89 5C 24 10  mov     [rsp+arg_8], rbx
            // 1.5.0.1315_x64
            // 48 89 5C 24 08  mov     [rsp+arg_0], rbx
            Byte(0x48),
            Byte(0x89),
            Byte(0x5C),
            Byte(0x24),
            // 66
            Many(80),
            // v1.4.1.1009_x64
            // regcomp_p_similar 5CA30
            // B9 20 00 00 00     mov     ecx, 20h ; ' '
            // Byte(0xB9),
            // Byte(0x20),
            // Byte(0x00),
            // Byte(0x00),
            // Byte(0x00),

            // B8 41 04 00 00     mov     eax, 441h
            // 41 B8 40 04 00 00  mov     r8d, 440h
            Case(10),
            Byte(0x41),
            Byte(0x04),
            Byte(0x00),
            Byte(0x00),
            Many(16),
            Byte(0x40),
            Byte(0x04),
            Byte(0x00),
            Byte(0x00),
            // v1.5.0.1315_x64
            // BB 40 04 00 00         mov     ebx, 440h
            // 48 C7 00 01 00 00 00   mov     qword ptr [rax], 1
            // 48 8B F8               mov     rdi, rax
            // B8 41 04 00 00         mov     eax, 441h
            Break(9),
            Byte(0x40),
            Byte(0x04),
            Byte(0x00),
            Byte(0x00),
            Many(16),
            Byte(0x41),
            Byte(0x04),
            Byte(0x00),
            Byte(0x00),
            // 41
            Many(60),
            Byte(0xE8),
            Jump4,
            Check(1),
        ])
        .save(&mut save)
        .call()
        // regcomp_p_similar
        .last()
        .cloned()
    }

    pub fn regcomp_p2(&self) -> Option<Rva> {
        let mut save = [0, self.regcomp_p()?];
        self.match_one_with_save(
            &[
                Aligned(4),
                Save(0),
                // v1.4.1.1009_x64
                // 48 89 74 24 18  mov     [rsp+arg_10], rsi
                // v1.5.0.1384_x64
                // 4C 8B DC        mov     r11, rsp
                Case(4),
                Byte(0x4C),
                Byte(0x8B),
                Byte(0xDC),
                Break(5),
                Byte(0x48),
                Byte(0x89),
                Byte(0x74),
                Byte(0x24),
                Byte(0x18),
                // v1.4.1.1009_x64
                // 6, 1
                // 48 83 EC 30           sub     rsp, 30h
                // v1.5.0.1315_x64
                // 4, 1
                // 48 81 EC E0 03 00 00  sub     rsp, 3E0h
                Many(6),
                Byte(0x48),
                Skip(1),
                Byte(0xEC),
                // v1.4.1.1009_x64
                // 19, 9
                // 0F BA E2 0B              bt      edx, 0Bh
                // v1.5.0.1315_x64
                // 36, 25
                // 0F BA A0 18 01 00 00 0B  bt      dword ptr [rax+280], 0Bh
                Many(64),
                Byte(0x0F),
                Byte(0xBA),
                Many(6),
                Byte(0x0B),
                // 378
                // 1516
                // v1.5.0.1384_x64: 1604
                // TODO: Relatively slow
                Rangext((4096u32 >> 8) as u8),
                Many(0),
                Byte(0xE8),
                Jump4,
                Check(1),
            ],
            &mut save,
        )
    }

    pub fn regcomp_p2_modifiers(&self) -> Option<usize> {
        Some(280)
    }

    pub fn regcomp_p2_termtext(&self) -> Option<(usize, usize)> {
        let mut save = [0, 0];
        self.match_first_with_save(&[
            // v1.4.1.1026_x64
            // 4C 8D 47 30           lea     r8, [rdi+48]
            // 48 8D 15 DD DF 19 00  lea     rdx, aTermtextS ; "termtext %s\n"

            // v1.5.0.1315_x64
            // 4C 8B 8F 00 01 00 00  mov     r9, [rdi+256]
            // 4C 8D 87 20 01 00 00  lea     r8, [rdi+288]
            // 48 8D 15 A6 13 33 00  lea     rdx, aTermtextT ; "termtext %t\n"
            // v1.5.0.1384_x64
            // 4C 8B 8F 00 01 00 00  mov     r9, [rdi+256]
            // 4C 8D 87 28 01 00 00  lea     r8, [rdi+296]
            // 48 8D 15 22 B5 41 00  lea     rdx, aTermtextT ; "termtext %t\n"
            // v1.5.0.1403_x64 (LTCG)
            // 4C 8B 8E 00 01 00 00  mov     r9, [rsi+256]
            // 4C 8D 86 28 01 00 00  lea     r8, [rsi+296]
            Byte(0x4C),
            Byte(0x8B),
            Fuzzy(0xF0),
            Byte(0x8F),
            ReadU32(0),
            Byte(0x4C),
            Byte(0x8D),
            Fuzzy(0xF0),
            Byte(0x87),
            ReadU32(1),
            Byte(0x48),
            Byte(0x8D),
            Byte(0x15),
            Jump4,
            // "termtext %t\n"
            Byte(b't'),
            Byte(b'e'),
            Byte(b'r'),
            Byte(b'm'),
            Byte(b't'),
            Byte(b'e'),
            Byte(b'x'),
            Byte(b't'),
            Byte(b' '),
            Byte(b'%'),
            Byte(b't'),
            Byte(b'\n'),
        ])
        .save(&mut save)
        .call()?;
        Some((save[0] as usize, save[1] as usize))
    }

    pub fn regcomp(&self) -> Option<Rva> {
        self.match_one(&[
            Save(0),
            // 40 53        push    rbx
            // 48 83 EC 40  sub     rsp, 40h
            // 45 33 C9     xor     r9d, r9d
            Byte(0x40),
            Byte(0x53),
            Byte(0x48),
            Byte(0x83),
            Byte(0xEC),
            Byte(0x40),
            Byte(0x45),
            Byte(0x33),
            Byte(0xC9),
        ])
    }

    pub fn regexec(&self) -> Option<Rva> {
        // v1.5.0.1403+ (LTCG)
        if let Some(regcomp) = self.regcomp() {
            if let Some(regexec) = self.exception_next(regcomp) {
                return Some(regexec);
            }

            /*
            let mut save = [0];
            if let Some(regexec) = self
                .match_first_with_save(&[
                    Aligned(2),
                    Save(0),
                    // TODO
                    // 48 89 5C 24 18  mov     [rsp+arg_10], rbx
                    Byte(0x48),
                    Byte(0x89),
                    Byte(0x5C),
                    Byte(0x24),
                    Byte(0x18),
                ])
                .save(&mut save)
                .range(regcomp..regcomp + 256)
                .call()
            {
                return Some(regexec);
            }
            */
        }

        self.match_one(&[
            Aligned(2),
            Save(0),
            // 48 89 5C 24 18  mov     [rsp+arg_10], rbx
            Byte(0x48),
            Byte(0x89),
            Byte(0x5C),
            Byte(0x24),
            Byte(0x18),
            // 53
            Many(70),
            // options = 128
            // B8 80 00 00 00  mov     eax, 80h ; '€'
            Byte(0xB8),
            Byte(0x80),
            Byte(0x00),
            Byte(0x00),
            Byte(0x00),
            // re->options & PCRE_NO_AUTO_CAPTURE
            // v1.4.1.1009_x64
            // 41 C1 EE 0C  shr     r14d, 0Ch
            // v1.5.0.1315_x64
            // 41 C1 ED 0C  shr     r13d, 0Ch
            Byte(0x41),
            Byte(0xC1),
            Skip(1),
            Byte(0x0C),
        ])
    }
}

#[derive(Debug, PartialEq, Eq)]
#[repr(C)]
pub struct EverythingExeOffsets {
    pub regcomp_p3: Option<Rva>,
    pub regcomp_p3_search: Option<usize>,
    pub regcomp_p3_filter: Option<usize>,
    pub regcomp_p: Option<Rva>,
    pub regcomp_p2: Option<Rva>,
    pub regcomp_p2_termtext: Option<(usize, usize)>,
    pub regcomp_p2_modifiers: Option<usize>,
    pub regcomp: Option<Rva>,
    pub regexec: Option<Rva>,
}

impl EverythingExe<'_> {
    pub fn offsets(&self) -> EverythingExeOffsets {
        EverythingExeOffsets {
            regcomp_p3: self.regcomp_p3(),
            regcomp_p3_search: self.regcomp_p3_search(),
            regcomp_p3_filter: self.regcomp_p3_filter(),
            regcomp_p: self.regcomp_p(),
            regcomp_p2: self.regcomp_p2(),
            regcomp_p2_termtext: self.regcomp_p2_termtext(),
            regcomp_p2_modifiers: self.regcomp_p2_modifiers(),
            regcomp: self.regcomp(),
            regexec: self.regexec(),
        }
    }
}

impl EverythingExeOffsets {
    pub fn from_path(path: &Path) -> Result<Self, anyhow::Error> {
        let map = pelite::FileMap::open(path)?;
        let exe = EverythingExe::from_bytes(map.as_ref())?;
        Ok(exe.offsets())
    }

    pub fn from_current_exe() -> Result<Self, anyhow::Error> {
        Self::from_path(&std::env::current_exe()?)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use std::{fs, path::Path, time};

    fn everything_exe_bytes(v: &str) -> Vec<u8> {
        let dir = Path::new("../tests/everything").join(v);
        let mut exe = dir.join("Everything64.exe");
        if !exe.exists() {
            exe = dir.join("Everything.exe");
        }
        fs::read(exe).unwrap()
    }

    fn everything_exe(v: &str) -> EverythingExe<'static> {
        let bytes = everything_exe_bytes(v).leak();
        EverythingExe::from_bytes(bytes).unwrap()
    }

    /// cargo test -r --package sig --lib -- tests::dump --exact --show-output --ignored
    #[ignore]
    #[test]
    fn dump() {
        let exe = everything_exe("v1.5.0.1396_x64");
        let t = time::Instant::now();
        println!("regcomp_p3: {:X?}", exe.regcomp_p3());
        println!("Time taken: {:?}", t.elapsed());
        println!("regcomp_p3_search: {:?}", exe.regcomp_p3_search());
        println!("Time taken: {:?}", t.elapsed());
        println!("regcomp_p3_filter: {:?}", exe.regcomp_p3_filter());
        println!("Time taken: {:?}", t.elapsed());

        println!("regcomp_p: {:X?}", exe.regcomp_p());
        println!("Time taken: {:?}", t.elapsed());

        println!("regcomp_p2: {:X?}", exe.regcomp_p2());
        println!("Time taken: {:?}", t.elapsed());
        println!("regcomp_p2_termtext: {:?}", exe.regcomp_p2_termtext());
        println!("Time taken: {:?}", t.elapsed());
        println!("regcomp_p2_modifiers: {:?}", exe.regcomp_p2_modifiers());
        println!("Time taken: {:?}", t.elapsed());

        println!("regcomp: {:X?}", exe.regcomp());
        println!("Time taken: {:?}", t.elapsed());

        println!("regexec: {:X?}", exe.regexec());
        println!("Time taken: {:?}", t.elapsed());
    }

    #[test]
    fn regcomp_p3() {
        assert_eq!(
            everything_exe("v1.4.1.1009_x64").regcomp_p3(),
            Some(0x174C0)
        );
        assert_eq!(
            everything_exe("v1.4.1.1015_x64").regcomp_p3(),
            Some(0x175A0)
        );
        assert_eq!(
            everything_exe("v1.4.1.1017_x64").regcomp_p3(),
            Some(0x175C0)
        );
        assert_eq!(
            everything_exe("v1.4.1.1018_x64").regcomp_p3(),
            Some(0x175A0)
        );
        assert_eq!(
            everything_exe("v1.4.1.1020_x64").regcomp_p3(),
            Some(0x175A0)
        );
        assert_eq!(
            everything_exe("v1.5.0.1296_x64").regcomp_p3(),
            Some(0x2D170)
        );
        assert_eq!(
            everything_exe("v1.5.0.1305_x64").regcomp_p3(),
            Some(0x2D920)
        );
        assert_eq!(
            everything_exe("v1.5.0.1315_x64").regcomp_p3(),
            Some(0x2DB30)
        );
        // assert_eq!(
        //     everything_exe("v1.5.0.1318_x64").regcomp_p3(),
        //     Some(0x319C0)
        // );
        assert_eq!(
            everything_exe("v1.5.0.1384_x64").regcomp_p3(),
            Some(0x3C220)
        );
    }

    #[test]
    fn regcomp_p3_search() {
        assert_eq!(
            everything_exe("v1.5.0.1315_x64").regcomp_p3_search(),
            Some(406 * 8)
        );
        assert_eq!(
            everything_exe("v1.5.0.1318_x64").regcomp_p3_search(),
            Some(393 * 8)
        );
        assert_eq!(
            everything_exe("v1.5.0.1384_x64").regcomp_p3_search(),
            Some(397 * 8)
        );
        assert_eq!(
            everything_exe("v1.5.0.1391_x64").regcomp_p3_search(),
            Some(397 * 8)
        );
    }

    #[test]
    fn regcomp_p3_filter() {
        assert_eq!(
            everything_exe("v1.5.0.1315_x64").regcomp_p3_filter(),
            Some(408 * 8)
        );
        assert_eq!(
            everything_exe("v1.5.0.1318_x64").regcomp_p3_filter(),
            Some(395 * 8)
        );
        assert_eq!(
            everything_exe("v1.5.0.1384_x64").regcomp_p3_filter(),
            Some(399 * 8)
        );
        assert_eq!(
            everything_exe("v1.5.0.1391_x64").regcomp_p3_filter(),
            Some(399 * 8)
        );
    }

    #[test]
    fn regcomp_p() {
        assert_eq!(everything_exe("v1.4.1.1009_x64").regcomp_p(), Some(0x5CA30));
        assert_eq!(everything_exe("v1.5.0.1315_x64").regcomp_p(), Some(0x9EDD0));
        assert_eq!(everything_exe("v1.5.0.1384_x64").regcomp_p(), Some(0xD4060));
    }

    #[test]
    fn regcomp_p2() {
        assert_eq!(
            everything_exe("v1.4.1.1009_x64").regcomp_p2(),
            Some(0x5CB70)
        );
        assert_eq!(
            everything_exe("v1.4.1.1015_x64").regcomp_p2(),
            Some(0x5CEC0)
        );
        assert_eq!(
            everything_exe("v1.4.1.1017_x64").regcomp_p2(),
            Some(0x5CEF0)
        );
        assert_eq!(
            everything_exe("v1.4.1.1018_x64").regcomp_p2(),
            Some(0x5CEB0)
        );
        assert_eq!(
            everything_exe("v1.4.1.1020_x64").regcomp_p2(),
            Some(0x5CEB0)
        );
        assert_eq!(
            everything_exe("v1.5.0.1296_x64").regcomp_p2(),
            Some(0xB17A0)
        );
        assert_eq!(
            everything_exe("v1.5.0.1305_x64").regcomp_p2(),
            Some(0xB44D0)
        );
        assert_eq!(
            everything_exe("v1.5.0.1315_x64").regcomp_p2(),
            Some(0xB7630)
        );
        assert_eq!(
            everything_exe("v1.5.0.1318_x64").regcomp_p2(),
            Some(0xC1F00)
        );
        assert_eq!(
            everything_exe("v1.5.0.1384_x64").regcomp_p2(),
            Some(0xF5380)
        );
    }

    #[test]
    fn regcomp_p2_termtext() {
        assert_eq!(
            everything_exe("v1.5.0.1315_x64").regcomp_p2_termtext(),
            Some((256, 288))
        );
        assert_eq!(
            everything_exe("v1.5.0.1384_x64").regcomp_p2_termtext(),
            Some((256, 296))
        );
        assert_eq!(
            everything_exe("v1.5.0.1391_x64").regcomp_p2_termtext(),
            Some((256, 296))
        );
        assert_eq!(
            everything_exe("v1.5.0.1403_x64").regcomp_p2_termtext(),
            Some((256, 296))
        );
    }

    #[test]
    fn regcomp_p2_modifiers() {
        assert_eq!(
            everything_exe("v1.5.0.1315_x64").regcomp_p2_modifiers(),
            Some(280)
        );
        assert_eq!(
            everything_exe("v1.5.0.1384_x64").regcomp_p2_modifiers(),
            Some(280)
        );
        assert_eq!(
            everything_exe("v1.5.0.1391_x64").regcomp_p2_modifiers(),
            Some(280)
        );
    }

    #[test]
    fn regcomp() {
        assert_eq!(everything_exe("v1.4.1.1009_x64").regcomp(), Some(0x1A8E80));
        assert_eq!(everything_exe("v1.4.1.1015_x64").regcomp(), Some(0x1A8FC0));
        assert_eq!(everything_exe("v1.4.1.1017_x64").regcomp(), Some(0x1A8FC0));
        assert_eq!(everything_exe("v1.4.1.1018_x64").regcomp(), Some(0x1A8E80));
        assert_eq!(everything_exe("v1.4.1.1020_x64").regcomp(), Some(0x1A8E80));
        assert_eq!(everything_exe("v1.5.0.1296_x64").regcomp(), Some(0x320800));
        assert_eq!(everything_exe("v1.5.0.1305_x64").regcomp(), Some(0x336880));
        assert_eq!(everything_exe("v1.5.0.1315_x64").regcomp(), Some(0x348820));
        assert_eq!(everything_exe("v1.5.0.1318_x64").regcomp(), Some(0x35AA30));
        assert_eq!(everything_exe("v1.5.0.1403_x64").regcomp(), Some(0x3AB3F0));
    }

    #[test]
    fn regexec() {
        assert_eq!(everything_exe("v1.4.1.1009_x64").regexec(), Some(0x1A8F60));
        assert_eq!(everything_exe("v1.4.1.1015_x64").regexec(), Some(0x1A90A0));
        assert_eq!(everything_exe("v1.4.1.1017_x64").regexec(), Some(0x1A90A0));
        assert_eq!(everything_exe("v1.4.1.1018_x64").regexec(), Some(0x1A8F60));
        assert_eq!(everything_exe("v1.4.1.1020_x64").regexec(), Some(0x1A8F60));
        assert_eq!(everything_exe("v1.5.0.1296_x64").regexec(), Some(0x3208E0));
        assert_eq!(everything_exe("v1.5.0.1305_x64").regexec(), Some(0x336960));
        assert_eq!(everything_exe("v1.5.0.1315_x64").regexec(), Some(0x348900));
        assert_eq!(everything_exe("v1.5.0.1318_x64").regexec(), Some(0x35AB10));
        assert_eq!(everything_exe("v1.5.0.1346_x64").regexec(), Some(0x3A1310));
        assert_eq!(everything_exe("v1.5.0.1367_x64").regexec(), Some(0x3D3590));
        assert_eq!(everything_exe("v1.5.0.1384_x64").regexec(), Some(0x42AB70));
        assert_eq!(everything_exe("v1.5.0.1403_x64").regexec(), Some(0x3AB4D0));
    }
}
