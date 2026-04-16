use everything_plugin::log::error;
use ib_hook::process::module::Module;
use ib_shell::{
    app::ShellApp,
    hook::{HookConfig, inject::ShellInjector},
    item::{
        self,
        hook::{
            folder::FolderHookConfig,
            prop::{PropertyHookConfig, system::PropertySystemHookConfig},
        },
    },
};
use ib_shell_verb::hook;
use serde::{Deserialize, Serialize};

pub mod options;

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ShellConfig {
    pub open_file_in_workspace_vscode: bool,
    pub inject: Option<bool>,
    pub inject_explorer: Option<bool>,
    pub size_no_alwayskb: Option<bool>,
    pub size_max_bar: Option<bool>,
}

impl Default for ShellConfig {
    fn default() -> Self {
        Self {
            open_file_in_workspace_vscode: false,
            inject: None,
            inject_explorer: None,
            size_no_alwayskb: None,
            size_max_bar: None,
        }
    }
}

impl ShellConfig {
    pub fn inject(&self) -> bool {
        self.inject.unwrap_or(true)
    }

    pub fn inject_explorer(&self) -> bool {
        self.inject_explorer.unwrap_or(true)
    }

    pub fn size_no_alwayskb(&self) -> bool {
        self.size_no_alwayskb.unwrap_or(true)
    }

    pub fn size_max_bar(&self) -> bool {
        self.size_max_bar.unwrap_or(true)
    }

    pub fn apps(&self) -> Vec<ShellApp> {
        use ib_shell::app;

        let mut apps = Vec::new();
        if self.inject_explorer() {
            apps.push(app::EXPLORER);
        }
        apps
    }

    pub fn injector(&self) -> Option<ShellInjector> {
        if !self.inject() {
            return None;
        }
        let config = HookConfig::builder()
            .item(
                item::hook::HookConfig::builder()
                    .enabled(true)
                    .folder(
                        FolderHookConfig::builder()
                            .compare_size_from_everything(true)
                            .build(),
                    )
                    .property({
                        PropertyHookConfig::builder()
                            .size_from_everything(true)
                            .system(
                                PropertySystemHookConfig::builder()
                                    .size_no_alwayskb(self.size_no_alwayskb())
                                    .size_max_bar(self.size_max_bar())
                                    .build(),
                            )
                            .build()
                    })
                    .build(),
            )
            .build();
        ShellInjector::builder()
            .dll_path(Module::current().get_path())
            .config(config)
            .apps(self.apps())
            .build()
            .inspect_err(|e| error!(?e, "ShellInjector"))
            .ok()
    }

    pub fn start(&self) {
        let mut verbs: Vec<Box<dyn ib_shell_verb::OpenVerb>> = Vec::new();
        if self.open_file_in_workspace_vscode {
            verbs.push(Box::new(
                ib_shell_verb::workspace::OpenFileInWorkspace::builder()
                    .parent_as_workspace(false)
                    .vscode(Default::default())
                    .build(),
            ));
        }
        hook::set_hook(Some(hook::HookConfig { verbs }));
    }

    pub fn stop(&self) {
        hook::set_hook(None);
    }
}

pub use ib_shell::hook::dll;
