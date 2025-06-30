#![allow(non_snake_case)]

use everything_plugin::{
    PluginApp, PluginHandler,
    log::{debug, error},
    plugin_main,
    serde::{Deserialize, Serialize},
    ui::{self, OptionsPage},
};

// mod options;
mod ffi;

#[derive(Serialize, Deserialize, Debug, Default, Clone)]
pub struct Config {
    s: String,
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
        let instance_name = HANDLER
            .instance_name()
            .unwrap_or_default()
            .encode_utf16()
            .chain([0, 0])
            .collect::<Vec<_>>();
        let args = ffi::StartArgs {
            config: self.config.s.as_ptr() as *const _,
            ipc_window: HANDLER
                .host()
                .ipc_window_from_main_thread()
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
            // TODO
            // OptionsPage::builder()
            //     .name("Test Plugin")
            //     .load(ui::winio::spawn::<options::MainModel>)
            //     .build(),
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
