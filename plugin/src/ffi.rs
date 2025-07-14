use std::{
    ffi::{c_char, c_void},
    fs,
};

use everything_plugin::{ipc::IpcWindow, log::debug};

use crate::HANDLER;

#[unsafe(no_mangle)]
extern "C" fn plugin_start() {
    let config = std::env::current_exe()
        .unwrap()
        .parent()
        .unwrap()
        .join("plugins/IbEverythingExt/config.yaml");
    match fs::read_to_string(config)
        .ok()
        .and_then(|yaml| serde_yaml_ng::from_str(&yaml).ok())
    {
        Some(config) => HANDLER.init_start_with_config(config),
        None => HANDLER.init_start(),
    }
}

#[unsafe(no_mangle)]
extern "C" fn plugin_stop() {
    HANDLER.stop_kill();
}

#[repr(C)]
pub struct StartArgs {
    pub host: bool,
    pub config: *const c_char,
    pub ipc_window: *const c_void,
    pub instance_name: *const u16,
}

unsafe extern "C" {
    pub fn start(args: *const StartArgs) -> bool;
    pub fn stop();
}

#[unsafe(no_mangle)]
extern "C" fn on_ipc_window_created() {
    HANDLER.with_app(|a| {
        a.ipc.get_or_init(|| {
            let ipc = IpcWindow::from_current_thread().unwrap();
            let version = ipc.get_version();
            debug!(ipc_version = ?version);
            (ipc, version)
        });
    });
}

#[repr(C)]
pub struct EverythingExeOffsets {
    pub regcomp_p3: u32,
    pub regcomp_p3_search: usize,
    pub regcomp_p3_filter: usize,
    pub regcomp_p: u32,
    pub regcomp_p2: u32,
    pub regcomp_p2_termtext_0: usize,
    pub regcomp_p2_termtext_1: usize,
    pub regcomp_p2_modifiers: usize,
    pub regcomp: u32,
    pub regexec: u32,
}

impl From<Option<&sig::EverythingExeOffsets>> for EverythingExeOffsets {
    fn from(offsets: Option<&sig::EverythingExeOffsets>) -> Self {
        EverythingExeOffsets {
            regcomp_p3: offsets.and_then(|o| o.regcomp_p3).unwrap_or_default(),
            regcomp_p3_search: offsets
                .and_then(|o| o.regcomp_p3_search)
                .unwrap_or_default(),
            regcomp_p3_filter: offsets
                .and_then(|o| o.regcomp_p3_filter)
                .unwrap_or_default(),
            regcomp_p: offsets.and_then(|o| o.regcomp_p).unwrap_or_default(),
            regcomp_p2: offsets.and_then(|o| o.regcomp_p2).unwrap_or_default(),
            regcomp_p2_termtext_0: offsets
                .and_then(|o| o.regcomp_p2_termtext)
                .map(|(first, _)| first)
                .unwrap_or_default(),
            regcomp_p2_termtext_1: offsets
                .and_then(|o| o.regcomp_p2_termtext)
                .map(|(_, second)| second)
                .unwrap_or_default(),
            regcomp_p2_modifiers: offsets
                .and_then(|o| o.regcomp_p2_modifiers)
                .unwrap_or_default(),
            regcomp: offsets.and_then(|o| o.regcomp).unwrap_or_default(),
            regexec: offsets.and_then(|o| o.regexec).unwrap_or_default(),
        }
    }
}

#[unsafe(no_mangle)]
extern "C" fn get_everything_exe_offsets() -> EverythingExeOffsets {
    // match LazyLock::get(&HANDLER) {
    //     Some(handler) => handler.with_app(|a| a.offsets.as_ref().into()),
    //     None => sig::EverythingExeOffsets::from_current_exe()
    //         .ok()
    //         .as_ref()
    //         .into(),
    // }
    HANDLER.with_app(|a| a.offsets.as_ref().into())
}
