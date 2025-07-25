use ib_matcher::pinyin::PinyinNotation;
use serde::{Deserialize, Serialize};

pub mod options;

#[derive(Serialize, Deserialize, Debug, Clone, Copy, Default)]
pub enum PinyinSearchMode {
    #[default]
    Auto,
    /// 默认模式
    Pcre2,
    /// - 不支持忽略 Unicode 大小写
    /// - 存在部分拼音匹配 bug (#56,#69,#77)
    Pcre,
    /// 版本兼容性好，但只支持简拼搜索，性能较低，且存在许多 bug
    Edit,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct PinyinSearchConfig {
    pub enable: bool,

    pub mode: PinyinSearchMode,

    pub allow_partial_match: Option<bool>,

    /// 简拼
    pub initial_letter: bool,
    /// 全拼
    pub pinyin_ascii: bool,
    /// 带声调全拼
    pub pinyin_ascii_digit: bool,
    /// 智能 ABC 双拼
    pub double_pinyin_abc: bool,
    /// 拼音加加双拼
    pub double_pinyin_jiajia: bool,
    /// 微软双拼
    pub double_pinyin_microsoft: bool,
    /// 华宇双拼（紫光双拼）
    pub double_pinyin_thunisoft: bool,
    /// 小鹤双拼
    pub double_pinyin_xiaohe: bool,
    /// 自然码双拼
    pub double_pinyin_zrm: bool,
}

impl Default for PinyinSearchConfig {
    fn default() -> Self {
        Self {
            enable: true,
            mode: Default::default(),
            allow_partial_match: None,
            initial_letter: true,
            pinyin_ascii: true,
            pinyin_ascii_digit: false,
            double_pinyin_abc: false,
            double_pinyin_jiajia: false,
            double_pinyin_microsoft: false,
            double_pinyin_thunisoft: false,
            double_pinyin_xiaohe: false,
            double_pinyin_zrm: false,
        }
    }
}

impl PinyinSearchConfig {
    pub fn notations(&self) -> PinyinNotation {
        let mut notations = PinyinNotation::empty();
        if self.initial_letter {
            notations |= PinyinNotation::AsciiFirstLetter;
        }
        if self.pinyin_ascii {
            notations |= PinyinNotation::Ascii;
        }
        if self.pinyin_ascii_digit {
            notations |= PinyinNotation::AsciiTone;
        }
        if self.double_pinyin_abc {
            notations |= PinyinNotation::DiletterAbc;
        }
        if self.double_pinyin_jiajia {
            notations |= PinyinNotation::DiletterJiajia;
        }
        if self.double_pinyin_microsoft {
            notations |= PinyinNotation::DiletterMicrosoft;
        }
        if self.double_pinyin_thunisoft {
            notations |= PinyinNotation::DiletterThunisoft;
        }
        if self.double_pinyin_xiaohe {
            notations |= PinyinNotation::DiletterXiaohe;
        }
        if self.double_pinyin_zrm {
            notations |= PinyinNotation::DiletterZrm;
        }
        notations
    }
}
