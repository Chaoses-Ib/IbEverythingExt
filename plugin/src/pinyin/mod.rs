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
