#![allow(unused_must_use)]
use everything_plugin::{log::debug, ui::winio::prelude::*};

use super::ShellConfig;
use crate::{App, HANDLER};

pub struct MainModel {
    window: Child<View>,

    open_file_in_workspace_vscode: Child<CheckBox>,
    link: Child<LinkLabel>,
}

#[derive(Debug)]
pub enum MainMessage {
    Noop,
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

        let mut link = Child::<LinkLabel>::init(&window).await?;
        link.set_text(t!("shell.link"));
        link.set_uri("https://github.com/Chaoses-Ib/ib-shell");

        // 加载当前配置
        HANDLER.with_app(|a| {
            let config = &a.config().shell;

            open_file_in_workspace_vscode.set_checked(config.open_file_in_workspace_vscode);
        });

        window.show();

        Ok(Self {
            window,
            open_file_in_workspace_vscode,
            link,
        })
    }

    async fn start(&mut self, sender: &ComponentSender<Self>) -> ! {
        start! {
            sender, default: MainMessage::Noop,
            self.open_file_in_workspace_vscode => {},
            self.link => {},
        }
    }

    async fn update_children(&mut self) -> Result<bool, Error> {
        update_children!(self.window, self.link)
    }

    async fn update(
        &mut self,
        message: Self::Message,
        sender: &ComponentSender<Self>,
    ) -> Result<bool, Self::Error> {
        Ok(match message {
            MainMessage::Noop => false,
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

        // 主布局
        let mut root_layout = layout! {
            Grid::from_str("1*", "auto,1*,auto").unwrap(),
            self.open_file_in_workspace_vscode => { column: 0, row: 0, margin: m },
            self.link => { column: 0, row: 2, margin: m },
        };

        root_layout.set_size(csize);
        Ok(())
    }
}
