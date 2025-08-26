use everything_plugin::{log::debug, ui::winio::prelude::*};

use super::{PinyinSearchConfig, PinyinSearchMode};
use crate::{App, HANDLER};

pub struct MainModel {
    window: Child<Window>,

    // 基本设置
    enabled: Child<CheckBox>,
    mode_label: Child<Label>,
    mode: Child<ComboBox>,
    allow_partial_match: Child<CheckBox>,

    // 拼音编码选项
    notation_label: Child<Label>,
    initial_letter: Child<CheckBox>,
    pinyin_ascii: Child<CheckBox>,
    pinyin_ascii_digit: Child<CheckBox>,

    // 双拼选项
    double_pinyin_abc: Child<CheckBox>,
    double_pinyin_jiajia: Child<CheckBox>,
    double_pinyin_microsoft: Child<CheckBox>,
    double_pinyin_thunisoft: Child<CheckBox>,
    double_pinyin_xiaohe: Child<CheckBox>,
    double_pinyin_zrm: Child<CheckBox>,
}

#[derive(Debug)]
pub enum MainMessage {
    Noop,
    Close,
    Redraw,
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

    fn init(mut init: Self::Init<'_>, sender: &ComponentSender<Self>) -> Self {
        let mut window = init.window(sender);
        // window.set_size(Size::new(500.0, 600.0));

        // 基本设置
        let mut enabled = Child::<CheckBox>::init(&window);
        enabled.set_text("启用拼音搜索");

        let mut mode_label = Child::<Label>::init(&window);
        mode_label.set_text("模式：");
        let mut mode = Child::<ComboBox>::init(&window);
        mode.insert(0, "自动");
        mode.insert(1, "PCRE 2（默认）");
        mode.insert(2, "PCRE");
        mode.insert(3, "Edit（兼容）");

        let mut allow_partial_match = Child::<CheckBox>::init(&window);
        allow_partial_match.set_text("允许关键词末尾拼音部分匹配");

        // 拼音模式选项标签
        let mut options_label = Child::<Label>::init(&window);
        options_label.set_text("拼音编码：");

        let mut initial_letter = Child::<CheckBox>::init(&window);
        initial_letter.set_text("简拼");

        let mut pinyin_ascii = Child::<CheckBox>::init(&window);
        pinyin_ascii.set_text("全拼");

        let mut pinyin_ascii_digit = Child::<CheckBox>::init(&window);
        pinyin_ascii_digit.set_text("带声调全拼");

        let mut double_pinyin_abc = Child::<CheckBox>::init(&window);
        double_pinyin_abc.set_text("智能 ABC 双拼");

        let mut double_pinyin_jiajia = Child::<CheckBox>::init(&window);
        double_pinyin_jiajia.set_text("拼音加加双拼");

        let mut double_pinyin_microsoft = Child::<CheckBox>::init(&window);
        double_pinyin_microsoft.set_text("微软双拼");

        let mut double_pinyin_thunisoft = Child::<CheckBox>::init(&window);
        double_pinyin_thunisoft.set_text("华宇双拼（紫光双拼）");

        let mut double_pinyin_xiaohe = Child::<CheckBox>::init(&window);
        double_pinyin_xiaohe.set_text("小鹤双拼");

        let mut double_pinyin_zrm = Child::<CheckBox>::init(&window);
        double_pinyin_zrm.set_text("自然码双拼");

        // 加载当前配置
        HANDLER.with_app(|a| {
            let config = &a.config().pinyin_search;

            enabled.set_checked(config.enable());

            mode.set_selection(Some(match config.mode {
                PinyinSearchMode::Auto => 0,
                PinyinSearchMode::Pcre2 => 1,
                PinyinSearchMode::Pcre => 2,
                PinyinSearchMode::Edit => 3,
            }));

            allow_partial_match.set_checked(config.allow_partial_match.unwrap_or(false));

            initial_letter.set_checked(config.initial_letter);
            pinyin_ascii.set_checked(config.pinyin_ascii);
            pinyin_ascii_digit.set_checked(config.pinyin_ascii_digit);
            double_pinyin_abc.set_checked(config.double_pinyin_abc);
            double_pinyin_jiajia.set_checked(config.double_pinyin_jiajia);
            double_pinyin_microsoft.set_checked(config.double_pinyin_microsoft);
            double_pinyin_thunisoft.set_checked(config.double_pinyin_thunisoft);
            double_pinyin_xiaohe.set_checked(config.double_pinyin_xiaohe);
            double_pinyin_zrm.set_checked(config.double_pinyin_zrm);
        });

        sender.post(MainMessage::EnabledClick);

        window.show();

        Self {
            window,
            enabled,
            mode_label,
            mode,
            allow_partial_match,
            notation_label: options_label,
            initial_letter,
            pinyin_ascii,
            pinyin_ascii_digit,
            double_pinyin_abc,
            double_pinyin_jiajia,
            double_pinyin_microsoft,
            double_pinyin_thunisoft,
            double_pinyin_xiaohe,
            double_pinyin_zrm,
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
            self.allow_partial_match => {},
            self.initial_letter => {},
            self.pinyin_ascii => {},
            self.pinyin_ascii_digit => {},
            self.double_pinyin_abc => {},
            self.double_pinyin_jiajia => {},
            self.double_pinyin_microsoft => {},
            self.double_pinyin_thunisoft => {},
            self.double_pinyin_xiaohe => {},
            self.double_pinyin_zrm => {},
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
                self.mode.set_enabled(is_enabled);
                self.allow_partial_match.set_enabled(is_enabled);
                self.initial_letter.set_enabled(is_enabled);
                self.pinyin_ascii.set_enabled(is_enabled);
                self.pinyin_ascii_digit.set_enabled(is_enabled);
                self.double_pinyin_abc.set_enabled(is_enabled);
                self.double_pinyin_jiajia.set_enabled(is_enabled);
                self.double_pinyin_microsoft.set_enabled(is_enabled);
                self.double_pinyin_thunisoft.set_enabled(is_enabled);
                self.double_pinyin_xiaohe.set_enabled(is_enabled);
                self.double_pinyin_zrm.set_enabled(is_enabled);

                false
            }
            MainMessage::OptionsPage(m) => {
                debug!(?m, "Options page message");
                match m {
                    OptionsPageMessage::Save(config, tx) => {
                        // 保存拼音搜索配置
                        config.pinyin_search = PinyinSearchConfig {
                            enable: Some(self.enabled.is_checked()),
                            mode: match self.mode.selection() {
                                Some(0) => PinyinSearchMode::Auto,
                                Some(1) => PinyinSearchMode::Pcre2,
                                Some(2) => PinyinSearchMode::Pcre,
                                Some(3) => PinyinSearchMode::Edit,
                                _ => Default::default(),
                            },
                            allow_partial_match: Some(self.allow_partial_match.is_checked()),
                            initial_letter: self.initial_letter.is_checked(),
                            pinyin_ascii: self.pinyin_ascii.is_checked(),
                            pinyin_ascii_digit: self.pinyin_ascii_digit.is_checked(),
                            double_pinyin_abc: self.double_pinyin_abc.is_checked(),
                            double_pinyin_jiajia: self.double_pinyin_jiajia.is_checked(),
                            double_pinyin_microsoft: self.double_pinyin_microsoft.is_checked(),
                            double_pinyin_thunisoft: self.double_pinyin_thunisoft.is_checked(),
                            double_pinyin_xiaohe: self.double_pinyin_xiaohe.is_checked(),
                            double_pinyin_zrm: self.double_pinyin_zrm.is_checked(),
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

        // 模式选择布局
        let mut mode_layout = layout! {
            Grid::from_str("auto,1*", "auto").unwrap(),
            // width for workaroud winio bug
            self.mode_label => { column: 0, row: 0, width: 50.0, margin: m_label, valign: VAlign::Center },
            self.mode => { column: 1, row: 0, margin: m },
        };

        // 主布局
        let mut root_layout = layout! {
            Grid::from_str("1*", "auto,auto,auto,auto,auto,auto,auto,auto,auto,auto,auto,auto,auto,auto,1*").unwrap(),
            self.enabled => { column: 0, row: 0, margin: m },
            mode_layout => { column: 0, row: 1, margin: m },
            self.allow_partial_match => { column: 0, row: 2, margin: m },
            self.notation_label => { column: 0, row: 3, margin: m },
            self.initial_letter => { column: 0, row: 4, margin: m },
            self.pinyin_ascii => { column: 0, row: 5, margin: m },
            self.pinyin_ascii_digit => { column: 0, row: 6, margin: m },
            self.double_pinyin_abc => { column: 0, row: 8, margin: m },
            self.double_pinyin_jiajia => { column: 0, row: 9, margin: m },
            self.double_pinyin_microsoft => { column: 0, row: 10, margin: m },
            self.double_pinyin_thunisoft => { column: 0, row: 11, margin: m },
            self.double_pinyin_xiaohe => { column: 0, row: 12, margin: m },
            self.double_pinyin_zrm => { column: 0, row: 13, margin: m },
        };

        root_layout.set_size(csize);
    }
}
