use everything_plugin::{log::debug, ui::winio::prelude::*};

use super::{InputMode, QuickSelectConfig, ResultListConfig, SearchEditConfig, TerminalKind};
use crate::{App, HANDLER, quick_select::AltKind};

pub struct MainModel {
    window: Child<Window>,

    enabled: Child<CheckBox>,
    description: Child<TextBox>,
    close_everything: Child<ToolTip<CheckBox>>,
    search_edit_label: Child<Label>,
    search_edit_alt_label: Child<ToolTip<Label>>,
    search_edit_alt: Child<ComboBox>,
    result_list_label: Child<Label>,
    result_list_alt_label: Child<ToolTip<Label>>,
    result_list_alt: Child<ComboBox>,
    result_list_select: Child<CheckBox>,
    terminal_kind_label: Child<ToolTip<Label>>,
    terminal_kind: Child<ComboBox>,
    terminal_label: Child<Label>,
    terminal: Child<Edit>,
    input_mode_label: Child<Label>,
    input_mode: Child<ComboBox>,
}

#[derive(Debug)]
pub enum MainMessage {
    Noop,
    Close,
    Redraw,
    EnabledClick,
    TerminalKindSelect,
    OptionsPage(OptionsPageMessage<App>),
}

impl From<OptionsPageMessage<App>> for MainMessage {
    fn from(value: OptionsPageMessage<App>) -> Self {
        Self::OptionsPage(value)
    }
}

impl Component for MainModel {
    type Event = ();
    type Init<'a> = OptionsPageInit<'a, App>;
    type Message = MainMessage;

    fn init(mut init: Self::Init<'_>, sender: &ComponentSender<Self>) -> Self {
        let mut window = init.window(sender);
        // window.set_size(Size::new(500.0, 600.0));

        // 基本设置
        let mut enabled = Child::<CheckBox>::init(&window);
        enabled.set_text(t!("quick.enabled"));

        let mut description = Child::<TextBox>::init(&window);
        description.set_text(t!("quick.description"));

        let mut close_everything = Child::<ToolTip<CheckBox>>::init(&window);
        close_everything.set_text(t!("quick.close_everything"));
        close_everything.set_tooltip(t!("quick.close_everything.tooltip"));

        let mut search_edit_label = Child::<Label>::init(&window);
        search_edit_label.set_text(t!("quick.search_edit"));

        let mut search_edit_alt_label = Child::<ToolTip<Label>>::init(&window);
        search_edit_alt_label.set_text(t!("quick.alt"));
        search_edit_alt_label.set_tooltip(t!("quick.alt.tooltip"));
        let mut search_edit_alt = Child::<ComboBox>::init(&window);
        search_edit_alt.insert(0, t!("disabled"));
        search_edit_alt.insert(1, "Alt+0~9");
        search_edit_alt.insert(2, "Alt+[0-9A-Z]");

        let mut result_list_label = Child::<Label>::init(&window);
        result_list_label.set_text(t!("quick.result_list"));

        let mut result_list_alt_label = Child::<ToolTip<Label>>::init(&window);
        result_list_alt_label.set_text(t!("quick.alt"));
        result_list_alt_label.set_tooltip(t!("quick.alt.tooltip"));
        let mut result_list_alt = Child::<ComboBox>::init(&window);
        result_list_alt.insert(0, t!("disabled"));
        result_list_alt.insert(1, "Alt+0~9");
        result_list_alt.insert(2, "Alt+[0-9A-Z]");

        let mut result_list_select = Child::<CheckBox>::init(&window);
        result_list_select.set_text(t!("quick.result_list_select"));

        let mut terminal_kind_label = Child::<ToolTip<Label>>::init(&window);
        terminal_kind_label.set_text(t!("quick.terminal"));
        terminal_kind_label.set_tooltip(t!("quick.terminal.tooltip"));
        let mut terminal_kind = Child::<ComboBox>::init(&window);
        terminal_kind.insert(0, t!("disabled"));
        terminal_kind.insert(1, "Windows Terminal (wt -d ${fileDirname})");
        terminal_kind.insert(2, "Windows Console (conhost)");
        terminal_kind.insert(3, t!("custom"));

        let mut terminal_label = Child::<Label>::init(&window);
        terminal_label.set_text(t!("quick.terminal.command"));
        let mut terminal = Child::<Edit>::init(&window);

        let mut input_mode_label = Child::<Label>::init(&window);
        input_mode_label.set_text(t!("quick.input_mode"));
        let mut input_mode = Child::<ComboBox>::init(&window);
        input_mode.insert(0, t!("auto"));
        input_mode.insert(1, "WmKey");
        input_mode.insert(2, "SendInput");

        // 加载当前配置
        HANDLER.with_app(|a| {
            let config = &a.config().quick_select;

            enabled.set_checked(config.enable);
            close_everything.set_checked(config.close_everything);

            search_edit_alt.set_selection(Some(match config.search_edit.alt {
                AltKind::None => 0,
                AltKind::Alt09 => 1,
                AltKind::Alt09AZ => 2,
            }));

            result_list_alt.set_selection(Some(match config.result_list.alt {
                AltKind::None => 0,
                AltKind::Alt09 => 1,
                AltKind::Alt09AZ => 2,
            }));
            result_list_select.set_checked(config.result_list.select);

            terminal_kind.set_selection(Some(match config.result_list.terminal_kind {
                TerminalKind::None => 0,
                TerminalKind::WindowsTerminal => 1,
                TerminalKind::WindowsConsole => 2,
                TerminalKind::Custom => 3,
            }));
            terminal.set_text(&config.result_list.terminal);

            input_mode.set_selection(Some(match config.input_mode {
                InputMode::Auto => 0,
                InputMode::WmKey => 1,
                InputMode::SendInput => 2,
            }));
        });

        sender.post(MainMessage::EnabledClick);
        sender.post(MainMessage::TerminalKindSelect);

        window.show();

        Self {
            window,
            enabled,
            description,
            close_everything,
            search_edit_label,
            search_edit_alt_label,
            search_edit_alt,
            result_list_label,
            result_list_alt_label,
            result_list_alt,
            result_list_select,
            terminal_kind_label,
            terminal_kind,
            terminal_label,
            terminal,
            input_mode_label,
            input_mode,
        }
    }

    async fn start(&mut self, sender: &ComponentSender<Self>) -> ! {
        start! {
            sender, default: MainMessage::Noop,
            self.window => {
                WindowEvent::Close => MainMessage::Close,
                WindowEvent::Resize => MainMessage::Redraw,
            },
            self.enabled => {
                CheckBoxEvent::Click => MainMessage::EnabledClick
            },
            self.result_list_select => {},
            self.close_everything => {},
            self.terminal_kind => {
                ComboBoxEvent::Select => MainMessage::TerminalKindSelect
            },
        }
    }

    async fn update(&mut self, message: Self::Message, sender: &ComponentSender<Self>) -> bool {
        self.window.update().await;
        match message {
            MainMessage::Noop => false,
            MainMessage::Close => {
                sender.output(());
                false
            }
            MainMessage::Redraw => true,
            MainMessage::EnabledClick => {
                let is_enabled = self.enabled.is_checked();

                // 启用/禁用所有子控件
                self.description.set_enabled(is_enabled);
                self.close_everything.set_enabled(is_enabled);
                self.search_edit_alt.set_enabled(is_enabled);
                self.result_list_alt.set_enabled(is_enabled);
                self.result_list_select.set_enabled(is_enabled);
                self.terminal_kind.set_enabled(is_enabled);
                self.terminal
                    .set_enabled(is_enabled && self.terminal_kind.selection() == Some(3));
                self.input_mode.set_enabled(is_enabled);

                false
            }
            MainMessage::TerminalKindSelect => {
                let is_enabled = self.enabled.is_checked();
                let is_custom = self.terminal_kind.selection() == Some(3);
                self.terminal.set_enabled(is_enabled && is_custom);
                false
            }
            MainMessage::OptionsPage(m) => {
                debug!(?m, "Options page message");
                match m {
                    OptionsPageMessage::Save(config, tx) => {
                        config.quick_select = QuickSelectConfig {
                            enable: self.enabled.is_checked(),
                            close_everything: self.close_everything.is_checked(),
                            search_edit: SearchEditConfig {
                                alt: match self.search_edit_alt.selection() {
                                    Some(0) => AltKind::None,
                                    Some(1) => AltKind::Alt09,
                                    Some(2) => AltKind::Alt09AZ,
                                    _ => SearchEditConfig::default().alt,
                                },
                            },
                            result_list: ResultListConfig {
                                alt: match self.result_list_alt.selection() {
                                    Some(0) => AltKind::None,
                                    Some(1) => AltKind::Alt09,
                                    Some(2) => AltKind::Alt09AZ,
                                    _ => ResultListConfig::default().alt,
                                },
                                select: self.result_list_select.is_checked(),
                                terminal_kind: match self.terminal_kind.selection() {
                                    Some(0) => TerminalKind::None,
                                    Some(1) => TerminalKind::WindowsTerminal,
                                    Some(2) => TerminalKind::WindowsConsole,
                                    Some(3) => TerminalKind::Custom,
                                    _ => Default::default(),
                                },
                                terminal: self.terminal.text(),
                            },
                            input_mode: match self.input_mode.selection() {
                                Some(0) => InputMode::Auto,
                                Some(1) => InputMode::WmKey,
                                Some(2) => InputMode::SendInput,
                                _ => Default::default(),
                            },
                        };
                        tx.send(config).unwrap()
                    }
                }
                false
            }
        }
    }

    fn render(&mut self, _sender: &ComponentSender<Self>) {
        self.window.render();

        let csize = self.window.client_size();
        let m = Margin::new(4., 0., 4., 0.);
        let m_label = Margin::new(0., 4., 0., 0.);
        let m_group = Margin::new(0., 0., 8., 16.);

        // 搜索编辑框 Alt 组合键布局
        let mut search_edit_layout = layout! {
            Grid::from_str("auto,1*", "auto").unwrap(),
            self.search_edit_alt_label => { column: 0, row: 0, width: 120.0, margin: m_label, valign: VAlign::Center },
            self.search_edit_alt => { column: 1, row: 0, margin: m },
        };

        // 结果列表 Alt 组合键布局
        let mut result_list_alt_layout = layout! {
            Grid::from_str("auto,1*", "auto").unwrap(),
            self.result_list_alt_label => { column: 0, row: 0, width: 120.0, margin: m_label, valign: VAlign::Center },
            self.result_list_alt => { column: 1, row: 0, margin: m },
        };

        // 终端命令布局
        let mut result_list_terminal_layout = layout! {
            Grid::from_str("auto,1*", "auto,auto").unwrap(),
            self.terminal_kind_label => { column: 0, row: 0, width: 120.0, margin: m_label, valign: VAlign::Center },
            self.terminal_kind => { column: 1, row: 0, margin: m },
            self.terminal_label => { column: 0, row: 1, width: 120.0, margin: m_label, valign: VAlign::Center },
            self.terminal => { column: 1, row: 1, height: 32.0, margin: m },
        };

        let mut result_list_layout = layout! {
            Grid::from_str("1*", "auto,auto,auto").unwrap(),
            result_list_alt_layout => { column: 0, row: 0, margin: m },
            self.result_list_select => { column: 0, row: 1, margin: m },
            result_list_terminal_layout => { column: 0, row: 2, margin: m },
        };

        // 输入模式布局
        let mut input_mode_layout = layout! {
            Grid::from_str("auto,1*", "auto").unwrap(),
            self.input_mode_label => { column: 0, row: 0, width: 100.0, margin: m_label, valign: VAlign::Center },
            self.input_mode => { column: 1, row: 0, margin: m },
        };

        // 主布局
        let mut root_layout = layout! {
            Grid::from_str("1*", "auto,auto,auto,auto,auto,auto,auto,auto,1*").unwrap(),
            self.enabled => { column: 0, row: 0, margin: m },

            // self.description => { column: 0, row: 1, margin: m, height: 100.0 },

            self.search_edit_label => { column: 0, row: 2, margin: m },
            search_edit_layout => { column: 0, row: 3, margin: m_group },

            self.result_list_label => { column: 0, row: 4, margin: m },
            result_list_layout => { column: 0, row: 5, margin: m_group },

            self.close_everything => { column: 0, row: 6, margin: m },

            input_mode_layout => { column: 0, row: 7, margin: m },

            // This should be put after enabled, but Winio sets its min size too large
            self.description => { column: 0, row: 8, margin: m },
        };
        root_layout.set_size(csize);
    }
}
