use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct SearchConfig {
    pub ime_default_off: Option<bool>,
    #[serde(default)]
    pub mix_lang: bool,
    pub wildcard_complement_separator_as_star: Option<bool>,
    pub wildcard_two_separator_as_star: Option<bool>,
}

impl Default for SearchConfig {
    fn default() -> Self {
        Self {
            ime_default_off: None,
            mix_lang: false,
            wildcard_complement_separator_as_star: None,
            wildcard_two_separator_as_star: None,
        }
    }
}

impl SearchConfig {
    /// Locale-dependent default config
    pub fn ime_default_off(&self) -> bool {
        self.ime_default_off.unwrap_or_else(|| {
            let locale = rust_i18n::locale();
            locale.starts_with("zh-") || locale.starts_with("ja-")
        })
    }

    pub fn wildcard_complement_separator_as_star(&self) -> bool {
        self.wildcard_complement_separator_as_star.unwrap_or(true)
    }

    pub fn wildcard_two_separator_as_star(&self) -> bool {
        self.wildcard_two_separator_as_star.unwrap_or(true)
    }
}
