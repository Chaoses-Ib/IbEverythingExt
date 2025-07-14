use everything_plugin::{log::debug, ui::winio::prelude::*};

use super::UpdateConfig;
use crate::{App, HANDLER};

pub struct MainModel {
    window: Child<Window>,

    // 更新设置
    check: Child<CheckBox>,
    prerelease: Child<CheckBox>,
}

#[derive(Debug)]
pub enum MainMessage {
    Noop,
    Close,
    Redraw,
    CheckClick,
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
        window.set_size(Size::new(400.0, 200.0));

        // 更新设置
        let mut check = Child::<CheckBox>::init(&window);
        check.set_text("启用自动检查更新");

        let mut prerelease = Child::<CheckBox>::init(&window);
        prerelease.set_text("包括预览版");

        // 加载当前配置
        HANDLER.with_app(|a| {
            let config = &a.config().update;

            check.set_checked(config.check);
            prerelease.set_checked(config.prerelease.unwrap_or(false));
        });

        sender.post(MainMessage::CheckClick);

        window.show();

        Self {
            window,
            check,
            prerelease,
        }
    }

    async fn start(&mut self, sender: &ComponentSender<Self>) -> ! {
        start! {
            sender, default: MainMessage::Noop,
            self.window => {
                WindowEvent::Close => MainMessage::Close,
                WindowEvent::Resize => MainMessage::Redraw,
            },
            self.check => {
                CheckBoxEvent::Click => MainMessage::CheckClick
            },
            self.prerelease => {}
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
            MainMessage::CheckClick => {
                let is_enabled = self.check.is_checked();

                // 启用/禁用预览版选项
                self.prerelease
                    .set_enabled(env!("CARGO_PKG_VERSION_PRE").is_empty() && is_enabled);

                false
            }
            MainMessage::OptionsPage(m) => {
                debug!(?m, "Options page message");
                match m {
                    OptionsPageMessage::Save(config, tx) => {
                        // 保存更新配置
                        config.update = UpdateConfig {
                            check: self.check.is_checked(),
                            prerelease: if self.check.is_checked() {
                                Some(self.prerelease.is_checked())
                            } else {
                                None
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

        // 主布局
        let mut root_layout = layout! {
            Grid::from_str("1*", "auto,auto,1*").unwrap(),
            self.check => { column: 0, row: 0, margin: m },
            self.prerelease => { column: 0, row: 1, margin: m },
        };

        root_layout.set_size(csize);
    }
}
