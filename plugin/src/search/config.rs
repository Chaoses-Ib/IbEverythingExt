use serde::{Deserialize, Serialize};

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct SearchConfig {
    #[serde(default)]
    pub mix_lang: bool,
}

impl Default for SearchConfig {
    fn default() -> Self {
        Self { mix_lang: false }
    }
}
