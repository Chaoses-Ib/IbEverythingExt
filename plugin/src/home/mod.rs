use serde::{Deserialize, Serialize};

pub mod options;

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct UpdateConfig {
    /// 检查更新
    pub check: bool,
    /// 包括预览版
    pub prerelease: Option<bool>,
}

impl Default for UpdateConfig {
    fn default() -> Self {
        Self {
            check: true,
            prerelease: None,
        }
    }
}
