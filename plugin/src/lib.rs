#![allow(non_snake_case)]

use everything_plugin::{
    PluginApp, PluginHandler, plugin_main,
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
}

impl PluginApp for App {
    type Config = Config;

    fn new(config: Option<Self::Config>) -> Self {
        unsafe { ffi::start(0 as _) };
        Self {
            config: config.unwrap_or_default(),
        }
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
