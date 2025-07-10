use serde::{Deserialize, Serialize};

pub mod options;

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct RomajiSearchConfig {
    pub enable: bool,
    pub allow_partial_match: bool,
}

impl Default for RomajiSearchConfig {
    fn default() -> Self {
        Self {
            enable: false,
            allow_partial_match: true,
        }
    }
}
