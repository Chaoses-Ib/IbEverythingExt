use serde::{Deserialize, Serialize};

pub mod options;

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct RomajiSearchConfig {
    pub enable: Option<bool>,
    /// Historical shit
    #[serde(rename = "allow_partial_match")]
    pub partial_word: bool,
    #[serde(rename = "partial_kana", default)]
    pub allow_partial_match: bool,
}

impl Default for RomajiSearchConfig {
    fn default() -> Self {
        Self {
            enable: None,
            partial_word: true,
            allow_partial_match: false,
        }
    }
}

impl RomajiSearchConfig {
    /// Locale-dependent default config
    pub fn enable(&self) -> bool {
        self.enable
            .unwrap_or_else(|| rust_i18n::locale().starts_with("ja-"))
    }
}
