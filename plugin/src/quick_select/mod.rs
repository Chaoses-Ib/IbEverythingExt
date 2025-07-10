use serde::{Deserialize, Serialize};
use serde_repr::{Deserialize_repr, Serialize_repr};

pub mod options;

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct QuickSelectConfig {
    pub enable: bool,
    /// 搜索编辑框
    pub search_edit: SearchEditConfig,
    /// 结果列表
    pub result_list: ResultListConfig,
    /// 打开或定位文件后关闭窗口（不对 Everything 默认热键生效）
    ///
    /// 如果想要默认 Enter 热键也关闭窗口，可在 Everything 快捷键选项中将“打开选中对象，并退出 Everything”设置为 Enter
    pub close_everything: bool,
    /// 输入模拟模式
    pub input_mode: InputMode,
}

impl Default for QuickSelectConfig {
    fn default() -> Self {
        Self {
            enable: true,
            search_edit: Default::default(),
            result_list: Default::default(),
            close_everything: true,
            input_mode: Default::default(),
        }
    }
}

/// Alt 组合键范围
#[derive(Serialize_repr, Deserialize_repr, Debug, Clone)]
#[repr(u8)]
pub enum AltKind {
    /// 禁用
    None = 0,
    /// Alt+0~9
    Alt09 = 10,
    /// Alt+[0-9A-Z]
    /// - 原本的 `Alt+A~Z` 访问菜单功能可以通过先单击 `Alt` 键再按 `A~Z` 实现
    /// - 默认的 `Alt+1~4` 调整窗口大小、`Alt+P` 预览和 `Alt+D` 聚焦搜索编辑框则无法使用，可以通过更改 Everything 选项来绑定到其它热键上（其中 `Alt+D` 也可使用相同功能的 `Ctrl+F` 和 `F3` 来代替）
    Alt09AZ = 36,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct SearchEditConfig {
    pub alt: AltKind,
}

impl Default for SearchEditConfig {
    fn default() -> Self {
        Self {
            alt: AltKind::Alt09,
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub enum TerminalKind {
    None,
    #[default]
    WindowsTerminal,
    WindowsConsole,
    Custom,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ResultListConfig {
    pub alt: AltKind,
    /// `[0-9A-Z]` 选中项目
    pub select: bool,
    #[serde(default)]
    pub terminal_kind: TerminalKind,
    /// 终端 (v1.5a)
    /// - Windows Terminal："wt -d ${fileDirname}"
    /// - Windows Console："conhost"（不支持以管理员身份启动）
    /// - 禁用：""
    pub terminal: String,
}

impl ResultListConfig {
    pub fn terminal_command(&self) -> &str {
        match self.terminal_kind {
            TerminalKind::None => "",
            TerminalKind::WindowsTerminal => "wt -d ${fileDirname}",
            TerminalKind::WindowsConsole => "conhost",
            TerminalKind::Custom => &self.terminal,
        }
    }
}

impl Default for ResultListConfig {
    fn default() -> Self {
        Self {
            alt: AltKind::None,
            select: true,
            terminal_kind: Default::default(),
            terminal: Default::default(),
        }
    }
}

#[derive(Serialize, Deserialize, Debug, Clone, Default)]
pub enum InputMode {
    /// v1.5a→WmKey，v1.4→SendInput
    #[default]
    Auto,
    WmKey,
    SendInput,
}
