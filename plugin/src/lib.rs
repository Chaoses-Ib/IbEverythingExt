#![allow(non_snake_case)]

use std::sync::OnceLock;

use everything_plugin::{
    PluginApp, PluginHandler, PluginHost,
    ipc::{IpcWindow, Version},
    log::{debug, error},
    plugin_main,
    serde::{Deserialize, Serialize},
    ui::{OptionsPage, winio::spawn},
};
use ib_matcher::{matcher::RomajiMatchConfig, pinyin::PinyinData};

use crate::{
    home::UpdateConfig, pinyin::PinyinSearchConfig, quick_select::QuickSelectConfig,
    romaji::RomajiSearchConfig,
};

mod ffi;
mod home;
mod pinyin;
mod quick_select;
pub mod romaji;
pub mod search;

#[derive(Serialize, Deserialize, Debug, Default, Clone)]
pub struct Config {
    /// 拼音搜索
    pub pinyin_search: PinyinSearchConfig,
    #[serde(default)]
    pub romaji_search: RomajiSearchConfig,
    /// 快速选择
    pub quick_select: QuickSelectConfig,
    /// 更新
    pub update: UpdateConfig,
}

pub struct App {
    config: Config,
    ipc: OnceLock<(IpcWindow, Version)>,
    offsets: Option<sig::EverythingExeOffsets>,
    pinyin_data: PinyinData,
    romaji: Option<RomajiMatchConfig<'static>>,
}

impl PluginApp for App {
    type Config = Config;

    fn new(config: Option<Self::Config>) -> Self {
        let config = config.unwrap_or_default();

        let ipc = match PluginHost::ipc_window_from_main_thread() {
            Some(ipc) => {
                let version = ipc.get_version();
                debug!(ipc_version = ?version);
                (ipc, version).into()
            }
            None => OnceLock::new(),
        };

        let romaji = &config.romaji_search;
        let romaji = if romaji.enable {
            Some(
                RomajiMatchConfig::builder()
                    .allow_partial_pattern(romaji.allow_partial_match)
                    .build(),
            )
        } else {
            None
        };

        Self {
            ipc,
            pinyin_data: PinyinData::new(config.pinyin_search.notations()),
            romaji,
            config,
            offsets: match sig::EverythingExeOffsets::from_current_exe() {
                Ok(offsets) => {
                    debug!(?offsets);
                    Some(offsets)
                }
                Err(e) => {
                    error!(%e, "Failed to get offsets from current exe");
                    None
                }
            },
        }
    }

    fn start(&self) {
        use pinyin::PinyinSearchMode;

        let host = HANDLER.get_host().is_some();

        // Dirty fixes
        let mut config = self.config.clone();
        config.pinyin_search.mode = match self.config.pinyin_search.mode {
            PinyinSearchMode::Auto => PinyinSearchMode::Pcre2,
            mode => mode,
        };
        config.quick_select.result_list.terminal = self
            .config
            .quick_select
            .result_list
            .terminal_command()
            .into();
        let config = serde_json::to_string(&config).unwrap();

        let instance_name = HANDLER
            .instance_name()
            .unwrap_or_default()
            .encode_utf16()
            .chain([0, 0])
            .collect::<Vec<_>>();

        let args = ffi::StartArgs {
            host,
            config: if host {
                config.as_ptr() as *const _
            } else {
                0 as _
            },
            ipc_window: self.ipc.get().map(|w| w.0.hwnd()).unwrap_or_default(),
            instance_name: instance_name.as_ptr(),
        };
        unsafe { ffi::start(&args) };
    }

    fn config(&self) -> &Self::Config {
        &self.config
    }

    fn into_config(self) -> Self::Config {
        self.config.clone()
    }
}

impl App {
    /// Avaliable after:
    /// - v1.4: `on_ipc_window_created()`
    /// - v1.5: `new()`
    pub fn version(&self) -> Version {
        unsafe { self.ipc.get().unwrap_unchecked() }.1
    }
}

impl Drop for App {
    fn drop(&mut self) {
        unsafe { ffi::stop() };
    }
}

plugin_main!(App, {
    PluginHandler::builder()
        .name(env!("CARGO_CRATE_NAME"))
        .description(env!("CARGO_PKG_DESCRIPTION"))
        .author(env!("CARGO_PKG_AUTHORS"))
        .version(env!("CARGO_PKG_VERSION"))
        .link(env!("CARGO_PKG_HOMEPAGE"))
        .options_pages(vec![
            OptionsPage::builder()
                .name("IbEverythingExt")
                .load(spawn::<home::options::MainModel>)
                .build(),
            OptionsPage::builder()
                .name("拼音搜索")
                .load(spawn::<pinyin::options::MainModel>)
                .build(),
            OptionsPage::builder()
                .name("ローマ字検索")
                .load(spawn::<romaji::options::MainModel>)
                .build(),
            OptionsPage::builder()
                .name("快速选择")
                .load(spawn::<quick_select::options::MainModel>)
                .build(),
        ])
        .build()
});

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test() {
        assert_eq!(env!("CARGO_CRATE_NAME"), "IbEverythingExt");
    }

    #[test]
    fn yaml() {
        let yaml = include_str!("../../publish/IbEverythingExt/config.yaml");
        let config: Config = serde_yaml_ng::from_str(yaml).unwrap();
        assert_eq!(
            format!("{:?}", config),
            format!("{:?}", {
                let mut config = Config::default();
                config.quick_select.result_list.terminal = "wt -d ${fileDirname}".into();
                config.update.prerelease = Some(false);
                config
            })
        );
    }
}
