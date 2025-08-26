use serde::{Deserialize, Serialize};

pub mod options;

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct RomajiSearchConfig {
    pub enable: Option<bool>,
    pub allow_partial_match: bool,
}

impl Default for RomajiSearchConfig {
    fn default() -> Self {
        Self {
            enable: None,
            allow_partial_match: true,
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
