use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct SearchConfig {
    #[serde(default)]
    pub mix_lang: bool,
    pub wildcard_complement_separator_as_star: Option<bool>,
    pub wildcard_two_separator_as_star: Option<bool>,
}

impl Default for SearchConfig {
    fn default() -> Self {
        Self {
            mix_lang: false,
            wildcard_complement_separator_as_star: None,
            wildcard_two_separator_as_star: None,
        }
    }
}

impl SearchConfig {
    pub fn wildcard_complement_separator_as_star(&self) -> bool {
        self.wildcard_complement_separator_as_star.unwrap_or(true)
    }

    pub fn wildcard_two_separator_as_star(&self) -> bool {
        self.wildcard_two_separator_as_star.unwrap_or(true)
    }
}
