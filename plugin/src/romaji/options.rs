#![allow(unused_must_use)]
use everything_plugin::{log::debug, ui::winio::prelude::*};

use super::RomajiSearchConfig;
use crate::{App, HANDLER};

pub struct MainModel {
    window: Child<View>,

    enabled: Child<CheckBox>,

    allow_partial_match: Child<CheckBox>,

    system_label: Child<Label>,
    system_hepburn: Child<CheckBox>,
}

#[derive(Debug)]
pub enum MainMessage {
    Noop,
    EnabledClick,
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
        // window.set_size(Size::new(500.0, 600.0));

        let mut enabled = Child::<CheckBox>::init(&window).await?;
        enabled.set_text("ローマ字検索を有効にする");

        let mut allow_partial_match = Child::<CheckBox>::init(&window).await?;
        allow_partial_match.set_text("キーワード部分一致検索");

        let mut options_label = Child::<Label>::init(&window).await?;
        options_label.set_text("ローマ字の種別：");

        let mut system_hepburn = Child::<CheckBox>::init(&window).await?;
        system_hepburn.set_text("ヘボン式（Hepburn）");
        // TODO
        system_hepburn.set_checked(true);
        system_hepburn.set_enabled(false);

        // 加载当前配置
        HANDLER.with_app(|a| {
            let config = &a.config().romaji_search;

            enabled.set_checked(config.enable());

            allow_partial_match.set_checked(config.allow_partial_match);
        });

        sender.post(MainMessage::EnabledClick);

        window.show();

        Ok(Self {
            window,
            enabled,
            allow_partial_match,
            system_label: options_label,
            system_hepburn,
        })
    }

    async fn start(&mut self, sender: &ComponentSender<Self>) -> ! {
        start! {
            sender, default: MainMessage::Noop,
            self.enabled => {
                CheckBoxEvent::Click => MainMessage::EnabledClick
            },
            self.allow_partial_match => {},
            self.system_hepburn => {},
        }
    }

    async fn update(
        &mut self,
        message: Self::Message,
        sender: &ComponentSender<Self>,
    ) -> Result<bool, Self::Error> {
        self.window.update().await;
        Ok(match message {
            MainMessage::Noop => false,
            MainMessage::EnabledClick => {
                let is_enabled = self.enabled.is_checked()?;

                // 启用/禁用所有子控件
                self.allow_partial_match.set_enabled(is_enabled);

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
                        // 保存拼音搜索配置
                        config.romaji_search = RomajiSearchConfig {
                            enable: Some(self.enabled.is_checked()?),
                            allow_partial_match: self.allow_partial_match.is_checked()?,
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
            Grid::from_str("1*", "auto,auto,auto,auto,1*").unwrap(),
            self.enabled => { column: 0, row: 0, margin: m },
            self.allow_partial_match => { column: 0, row: 1, margin: m },
            self.system_label => { column: 0, row: 2, margin: m },
            self.system_hepburn => { column: 0, row: 3, margin: m },
        };

        root_layout.set_size(csize);
        Ok(())
    }
}
