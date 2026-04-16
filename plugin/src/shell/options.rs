#![allow(unused_must_use)]
use everything_plugin::{log::debug, ui::winio::prelude::*};

use super::ShellConfig;
use crate::{App, HANDLER};

pub struct MainModel {
    window: Child<View>,

    open_file_in_workspace_vscode: Child<CheckBox>,
    inject: Child<CheckBox>,
    inject_explorer: Child<CheckBox>,

    size_no_alwayskb: Child<CheckBox>,
    size_max_bar: Child<CheckBox>,

    link: Child<LinkLabel>,
}

#[derive(Debug)]
pub enum MainMessage {
    Noop,
    InjectClick,
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
    type Error = Error;

    async fn init(mut init: Self::Init<'_>, sender: &ComponentSender<Self>) -> Result<Self, Error> {
        let mut window = init.window(sender).await?;

        let mut open_file_in_workspace_vscode = Child::<CheckBox>::init(&window).await?;
        open_file_in_workspace_vscode.set_text(t!("shell.open_file_in_workspace_vscode"));

        let mut inject = Child::<CheckBox>::init(&window).await?;
        inject.set_text(t!("shell.inject"));
        inject.set_tooltip(t!("shell.inject.tooltip"));

        let mut inject_explorer = Child::<CheckBox>::init(&window).await?;
        inject_explorer.set_text(t!("shell.inject_explorer"));

        let mut size_no_alwayskb = Child::<CheckBox>::init(&window).await?;
        size_no_alwayskb.set_text(t!("shell.size_no_alwayskb"));

        let mut size_max_bar = Child::<CheckBox>::init(&window).await?;
        size_max_bar.set_text(t!("shell.size_max_bar"));

        let mut link = Child::<LinkLabel>::init(&window).await?;
        link.set_text(t!("shell.link"));
        link.set_uri("https://github.com/Chaoses-Ib/ib-shell");

        // 加载当前配置
        HANDLER.with_app(|a| {
            let config = &a.config().shell;

            open_file_in_workspace_vscode.set_checked(config.open_file_in_workspace_vscode);
            inject.set_checked(config.inject());
            inject_explorer.set_checked(config.inject_explorer());
            size_no_alwayskb.set_checked(config.size_no_alwayskb());
            size_max_bar.set_checked(config.size_max_bar());
        });

        sender.post(MainMessage::InjectClick);

        window.show();

        Ok(Self {
            window,
            open_file_in_workspace_vscode,
            inject,
            inject_explorer,
            size_no_alwayskb,
            size_max_bar,
            link,
        })
    }

    async fn start(&mut self, sender: &ComponentSender<Self>) -> ! {
        start! {
            sender, default: MainMessage::Noop,
            self.open_file_in_workspace_vscode => {},
            self.inject => {
                CheckBoxEvent::Click => MainMessage::InjectClick
            },
            self.inject_explorer => {},
            self.size_no_alwayskb => {},
            self.size_max_bar => {},
            self.link => {},
        }
    }

    async fn update_children(&mut self) -> Result<bool, Error> {
        update_children!(
            self.window,
            self.link,
            self.inject,
            self.inject_explorer,
            self.size_no_alwayskb,
            self.size_max_bar
        )
    }

    async fn update(
        &mut self,
        message: Self::Message,
        sender: &ComponentSender<Self>,
    ) -> Result<bool, Self::Error> {
        Ok(match message {
            MainMessage::Noop => false,
            MainMessage::InjectClick => {
                let is_inject = self.inject.is_checked()?;

                // 未勾选 inject 时禁用 inject_explorer 及 size_* 选项
                self.inject_explorer.set_enabled(is_inject);
                self.size_no_alwayskb.set_enabled(is_inject);
                self.size_max_bar.set_enabled(is_inject);

                false
            }
            MainMessage::OptionsPage(m) => {
                debug!(?m, "Options page message");
                match m {
                    OptionsPageMessage::Redraw => true,
                    OptionsPageMessage::Close => {
                        sender.output(());
                        false
                    }
                    OptionsPageMessage::Save(config, tx) => {
                        // 保存配置
                        config.shell = ShellConfig {
                            open_file_in_workspace_vscode: self
                                .open_file_in_workspace_vscode
                                .is_checked()?,
                            inject: Some(self.inject.is_checked()?),
                            inject_explorer: Some(self.inject_explorer.is_checked()?),
                            size_no_alwayskb: Some(self.size_no_alwayskb.is_checked()?),
                            size_max_bar: Some(self.size_max_bar.is_checked()?),
                        };
                        tx.send(config).unwrap();
                        false
                    }
                }
            }
        })
    }

    fn render(&mut self, _sender: &ComponentSender<Self>) -> Result<(), Self::Error> {
        self.window.render();

        let csize = self.window.size()?;
        let m = Margin::new(6., 0., 6., 0.);
        let m_group = Margin::new(0., 0., 8., 16.);

        // 主布局
        let mut root_layout = layout! {
            Grid::from_str("1*", "auto,auto,auto,auto,auto,1*,auto").unwrap(),
            self.open_file_in_workspace_vscode => { column: 0, row: 0, margin: m },
            self.inject => { column: 0, row: 1, margin: m },
            self.inject_explorer => { column: 0, row: 2, margin: m_group },
            self.size_no_alwayskb => { column: 0, row: 3, margin: m_group },
            self.size_max_bar => { column: 0, row: 4, margin: m_group },
            self.link => { column: 0, row: 6, margin: m },
        };

        root_layout.set_size(csize);
        Ok(())
    }
}
