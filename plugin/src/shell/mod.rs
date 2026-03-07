use ib_shell_verb::hook;
use serde::{Deserialize, Serialize};

pub mod options;

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ShellConfig {
    pub open_file_in_workspace_vscode: bool,
}

impl Default for ShellConfig {
    fn default() -> Self {
        Self {
            open_file_in_workspace_vscode: false,
        }
    }
}

impl ShellConfig {
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
