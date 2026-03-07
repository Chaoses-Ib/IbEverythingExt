#![allow(unused_must_use)]
use everything_plugin::{log::debug, ui::winio::prelude::*};

use super::UpdateConfig;
use crate::{App, HANDLER, search::config::SearchConfig};

pub struct MainModel {
    window: Child<View>,

    // 更新设置
    check: Child<CheckBox>,
    prerelease: Child<CheckBox>,

    ime_default_off: Child<CheckBox>,
    search_mix_lang: Child<CheckBox>,
    wildcard_complement_separator_as_star: Child<CheckBox>,
    wildcard_two_separator_as_star: Child<CheckBox>,

    search_syntax: Child<TextBox>,
}

#[derive(Debug)]
pub enum MainMessage {
    Noop,
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
    type Error = Error;

    async fn init(mut init: Self::Init<'_>, sender: &ComponentSender<Self>) -> Result<Self, Error> {
        let mut window = init.window(sender).await?;
        // window.set_size(Size::new(400.0, 200.0));

        // 更新设置
        let mut check = Child::<CheckBox>::init(&window).await?;
        check.set_text(t!("update.check"));

        let mut prerelease = Child::<CheckBox>::init(&window).await?;
        prerelease.set_text(t!("update.prerelease"));

        let mut ime_default_off = Child::<CheckBox>::init(&window).await?;
        ime_default_off.set_text(t!("search.ime_default_off"));

        let mut search_mix_lang = Child::<CheckBox>::init(&window).await?;
        search_mix_lang.set_text("允许混合匹配拼音和ローマ字（开启简拼时误匹配率较高）");

        let mut wildcard_complement_separator_as_star = Child::<CheckBox>::init(&window).await?;
        wildcard_complement_separator_as_star.set_text(t!("wildcard_complement_separator_as_star"));

        let mut wildcard_two_separator_as_star = Child::<CheckBox>::init(&window).await?;
        wildcard_two_separator_as_star.set_text(t!("wildcard_two_separator_as_star"));

        let mut search_syntax = Child::<TextBox>::init(&window).await?;
        search_syntax.set_text(t!("search.syntax"));
        // TODO: Readonly

        // 加载当前配置
        HANDLER.with_app(|a| {
            let config = &a.config().update;

            check.set_checked(config.check);
            prerelease.set_checked(config.prerelease.unwrap_or(false));

            let config = &a.config().search;
            ime_default_off.set_checked(config.ime_default_off());
            search_mix_lang.set_checked(config.mix_lang);
            wildcard_complement_separator_as_star
                .set_checked(config.wildcard_complement_separator_as_star());
            wildcard_two_separator_as_star.set_checked(config.wildcard_two_separator_as_star());
        });

        sender.post(MainMessage::CheckClick);

        window.show();

        Ok(Self {
            window,
            check,
            prerelease,
            ime_default_off,
            search_mix_lang,
            wildcard_complement_separator_as_star,
            wildcard_two_separator_as_star,
            search_syntax,
        })
    }

    async fn start(&mut self, sender: &ComponentSender<Self>) -> ! {
        start! {
            sender, default: MainMessage::Noop,
            self.check => {
                CheckBoxEvent::Click => MainMessage::CheckClick
            },
            self.prerelease => {},
            self.search_mix_lang => {},
            self.wildcard_complement_separator_as_star => {},
            self.wildcard_two_separator_as_star => {},
            self.ime_default_off => {},
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
            MainMessage::CheckClick => {
                let is_enabled = self.check.is_checked()?;

                // 启用/禁用预览版选项
                self.prerelease
                    .set_enabled(env!("CARGO_PKG_VERSION_PRE").is_empty() && is_enabled);

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
                        // 保存更新配置
                        config.update = UpdateConfig {
                            check: self.check.is_checked()?,
                            prerelease: if self.check.is_checked()? {
                                Some(self.prerelease.is_checked()?)
                            } else {
                                None
                            },
                        };
                        config.search = SearchConfig {
                            ime_default_off: self.ime_default_off.is_checked()?.into(),
                            mix_lang: self.search_mix_lang.is_checked()?,
                            wildcard_complement_separator_as_star: Some(
                                self.wildcard_complement_separator_as_star.is_checked()?,
                            ),
                            wildcard_two_separator_as_star: Some(
                                self.wildcard_two_separator_as_star.is_checked()?,
                            ),
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
        let m = Margin::new(4., 0., 4., 0.);

        let mut search_layout = layout! {
            Grid::from_str("1*", "auto,auto,auto,auto,1*").unwrap(),
            self.ime_default_off => { column: 0, row: 0, margin: m },
            self.search_mix_lang => { column: 0, row: 1, margin: m },
            self.wildcard_complement_separator_as_star => { column: 0, row: 2, margin: m },
            self.wildcard_two_separator_as_star => { column: 0, row: 3, margin: m },
            self.search_syntax => { column: 0, row: 4, margin: m },
        };

        // 主布局
        let mut root_layout = layout! {
            Grid::from_str("1*", "auto,auto,1*").unwrap(),
            self.check => { column: 0, row: 0, margin: m },
            self.prerelease => { column: 0, row: 1, margin: m },
            search_layout => { column: 0, row: 2, margin: m },
        };

        root_layout.set_size(csize);
        Ok(())
    }
}
