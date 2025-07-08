#![allow(non_snake_case)]

use everything_plugin::{
    PluginApp, PluginHandler, PluginHost,
    log::{debug, error},
    plugin_main,
    serde::{Deserialize, Serialize},
    ui::{OptionsPage, winio::spawn},
};

use crate::{home::UpdateConfig, pinyin::PinyinSearchConfig, quick_select::QuickSelectConfig};

mod ffi;
mod home;
mod pinyin;
mod quick_select;
pub mod search;

#[derive(Serialize, Deserialize, Debug, Default, Clone)]
pub struct Config {
    /// 拼音搜索
    pub pinyin_search: PinyinSearchConfig,
    /// 快速选择
    pub quick_select: QuickSelectConfig,
    /// 更新
    pub update: UpdateConfig,
}

pub struct App {
    config: Config,
    offsets: Option<sig::EverythingExeOffsets>,
}

impl PluginApp for App {
    type Config = Config;

    fn new(config: Option<Self::Config>) -> Self {
        Self {
            config: config.unwrap_or_default(),
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
            ipc_window: PluginHost::ipc_window_from_main_thread()
                .map(|w| w.hwnd())
                .unwrap_or_default(),
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
                .name("快速选择")
                .load(spawn::<quick_select::options::MainModel>)
                .build(),
        ])
        .build()
});

#[cfg(test)]
mod tests {
    #[test]
    fn test() {
        assert_eq!(env!("CARGO_CRATE_NAME"), "IbEverythingExt");
    }
}
