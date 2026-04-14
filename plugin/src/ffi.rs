use std::{
    ffi::{c_char, c_void},
    fs, thread,
};

use everything_plugin::{
    ipc::IpcWindow,
    log::debug,
    ui::winio::{prelude::*, winio::compio},
};

use crate::{Config, HANDLER};

pub fn read_config(default_if_nonexist: bool) -> Option<Config> {
    let config = std::env::current_exe()
        .unwrap()
        .parent()
        .unwrap()
        .join("Plugins/IbEverythingExt/config.yaml");
    match fs::read_to_string(config) {
        Ok(yaml) => match serde_yaml_ng::from_str(&yaml) {
            Ok(config) => Some(config),
            Err(e) => {
                // Without thread it will deadlock
                thread::spawn(move || {
                    compio::runtime::Runtime::new().unwrap().block_on(
                        MessageBox::new()
                            .title("IbEverythingExt")
                            .instruction("config.yaml 配置文件格式错误")
                            .message(format!("{:?}\n\nIbEverythingExt 将不会被启用", e))
                            .style(MessageBoxStyle::Error)
                            .show(()),
                    )
                });
                None
            }
        },
        Err(_) if default_if_nonexist => Some(Config::default()),
        Err(_) => {
            // Without thread it will deadlock
            thread::spawn(|| {
                compio::runtime::Runtime::new().unwrap().block_on(
                    MessageBox::new()
                        .title("IbEverythingExt")
                        .instruction(r"配置文件 Plugins\IbEverythingExt\config.yaml 不存在")
                        .message("IbEverythingExt 将不会被启用")
                        .style(MessageBoxStyle::Error)
                        .show(()),
                )
            });
            None
        }
    }
}

/// Everything v1.5 will load `WindowsCodecs.dll` even for sub instances (`load image lists`).
/// We use `wait_start()` to avoid load `WindowsCodecs.dll` unnecessarily.
#[unsafe(no_mangle)]
extern "C" fn plugin_start() {
    if !unsafe { start_on_create } {
        unsafe { wait_start() };
    } else {
        // plugin_start_inner() will trigger CreateWindowExW
        unsafe { start_on_create = false };
        plugin_start_inner();
    }
}

fn plugin_start_inner() {
    match read_config(false) {
        Some(config) => HANDLER.init_start_with_config(config),
        None => {
            // HANDLER.init_start()
        }
    }
}

/**
Everything will call ExitProcess() on exit, which breaks TLS of tracing.
It's just [`crate::App::drop()`] anyway, we can skip it.

```
IbEverythingExt.dll!std::panicking::panic_with_hook() Line 822
IbEverythingExt.dll!std::panicking::panic_handler::closure$0() Line 707
IbEverythingExt.dll!std::sys::backtrace::__rust_end_short_backtrace<std::panicking::panic_handler::closure_env$0,never$>() Line 174
IbEverythingExt.dll!std::panicking::panic_handler() Line 698
IbEverythingExt.dll!core::panicking::panic_fmt() Line 80
IbEverythingExt.dll!std::thread::local::panic_access_error() Line 430
IbEverythingExt.dll!std::thread::local::LocalKey<core::cell::RefCell<alloc::string::String>>::with<core::cell::RefCell<alloc::string::String>,tracing_subscriber::fmt::fmt_layer::impl$11::on_event::closure_env$0<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>,tuple$<>>(tracing_subscriber::fmt::fmt_layer::impl$11::on_event::closure_env$0<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()> self) Line 476
IbEverythingExt.dll!tracing_subscriber::fmt::fmt_layer::impl$11::on_event<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>(tracing_subscriber::fmt::fmt_layer::Layer<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()> * self, tracing_core::event::Event * event, tracing_subscriber::layer::context::Context<tracing_subscriber::registry::sharded::Registry>) Line 1014
IbEverythingExt.dll!tracing_subscriber::layer::layered::impl$1::event<tracing_subscriber::fmt::fmt_layer::Layer<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>,tracing_subscriber::registry::sharded::Registry>(tracing_subscriber::layer::layered::Layered<tracing_subscriber::fmt::fmt_layer::Layer<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>,tracing_subscriber::registry::sharded::Registry,tracing_subscriber::registry::sharded::Registry> * self, tracing_core::event::Event * event) Line 154
IbEverythingExt.dll!tracing_subscriber::layer::layered::impl$1::event<tracing_core::metadata::LevelFilter,tracing_subscriber::layer::layered::Layered<tracing_subscriber::fmt::fmt_layer::Layer<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>,tracing_subscriber::registry::sharded::Registry,tracing_subscriber::registry::sharded::Registry>>(tracing_subscriber::layer::layered::Layered<tracing_core::metadata::LevelFilter,tracing_subscriber::layer::layered::Layered<tracing_subscriber::fmt::fmt_layer::Layer<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>,tracing_subscriber::registry::sharded::Registry,tracing_subscriber::registry::sharded::Registry>,tracing_subscriber::layer::layered::Layered<tracing_subscriber::fmt::fmt_layer::Layer<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>,tracing_subscriber::registry::sharded::Registry,tracing_subscriber::registry::sharded::Registry>> * self, tracing_core::event::Event * event) Line 152
IbEverythingExt.dll!tracing_subscriber::fmt::impl$2::event<tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,tracing_core::metadata::LevelFilter,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>(tracing_subscriber::fmt::Subscriber<tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,tracing_core::metadata::LevelFilter,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()> * self, tracing_core::event::Event * event) Line 409
IbEverythingExt.dll!tracing_core::dispatcher::Dispatch::event(tracing_core::event::Event * self) Line 612
IbEverythingExt.dll!tracing_core::event::impl$0::dispatch::closure$0(tracing_core::event::impl$0::dispatch::closure_env$0 * current, tracing_core::dispatcher::Dispatch *) Line 36
IbEverythingExt.dll!tracing_core::dispatcher::get_default<tuple$<>,tracing_core::event::impl$0::dispatch::closure_env$0>(tracing_core::event::impl$0::dispatch::closure_env$0 f) Line 388
IbEverythingExt.dll!tracing_core::event::Event::dispatch(tracing_core::field::ValueSet * metadata) Line 37
IbEverythingExt.dll!tracing_panic::panic_hook::closure$1(tracing_panic::panic_hook::closure_env$1 *, tracing_core::field::ValueSet value_set) Line 884
IbEverythingExt.dll!tracing_panic::panic_hook(std::panic::PanicHookInfo * panic_info) Line 82
IbEverythingExt.dll!everything_plugin::log::tracing_try_init::closure$0(everything_plugin::log::tracing_try_init::closure_env$0 *, std::panic::PanicHookInfo * info) Line 26
IbEverythingExt.dll!std::panicking::panic_with_hook() Line 844
IbEverythingExt.dll!std::panicking::panic_handler::closure$0() Line 707
IbEverythingExt.dll!std::sys::backtrace::__rust_end_short_backtrace<std::panicking::panic_handler::closure_env$0,never$>() Line 174
IbEverythingExt.dll!std::panicking::panic_handler() Line 698
IbEverythingExt.dll!core::panicking::panic_fmt() Line 80
IbEverythingExt.dll!std::thread::local::panic_access_error() Line 430
IbEverythingExt.dll!std::thread::local::LocalKey<core::cell::RefCell<alloc::string::String>>::with<core::cell::RefCell<alloc::string::String>,tracing_subscriber::fmt::fmt_layer::impl$11::on_event::closure_env$0<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>,tuple$<>>(tracing_subscriber::fmt::fmt_layer::impl$11::on_event::closure_env$0<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()> self) Line 476
IbEverythingExt.dll!tracing_subscriber::fmt::fmt_layer::impl$11::on_event<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>(tracing_subscriber::fmt::fmt_layer::Layer<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()> * self, tracing_core::event::Event * event, tracing_subscriber::layer::context::Context<tracing_subscriber::registry::sharded::Registry>) Line 1014
IbEverythingExt.dll!tracing_subscriber::layer::layered::impl$1::event<tracing_subscriber::fmt::fmt_layer::Layer<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>,tracing_subscriber::registry::sharded::Registry>(tracing_subscriber::layer::layered::Layered<tracing_subscriber::fmt::fmt_layer::Layer<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>,tracing_subscriber::registry::sharded::Registry,tracing_subscriber::registry::sharded::Registry> * self, tracing_core::event::Event * event) Line 154
IbEverythingExt.dll!tracing_subscriber::layer::layered::impl$1::event<tracing_core::metadata::LevelFilter,tracing_subscriber::layer::layered::Layered<tracing_subscriber::fmt::fmt_layer::Layer<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>,tracing_subscriber::registry::sharded::Registry,tracing_subscriber::registry::sharded::Registry>>(tracing_subscriber::layer::layered::Layered<tracing_core::metadata::LevelFilter,tracing_subscriber::layer::layered::Layered<tracing_subscriber::fmt::fmt_layer::Layer<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>,tracing_subscriber::registry::sharded::Registry,tracing_subscriber::registry::sharded::Registry>,tracing_subscriber::layer::layered::Layered<tracing_subscriber::fmt::fmt_layer::Layer<tracing_subscriber::registry::sharded::Registry,tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>,tracing_subscriber::registry::sharded::Registry,tracing_subscriber::registry::sharded::Registry>> * self, tracing_core::event::Event * event) Line 152
IbEverythingExt.dll!tracing_subscriber::fmt::impl$2::event<tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,tracing_core::metadata::LevelFilter,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()>(tracing_subscriber::fmt::Subscriber<tracing_subscriber::fmt::format::DefaultFields,tracing_subscriber::fmt::format::Format<tracing_subscriber::fmt::format::Full,tracing_subscriber::fmt::time::SystemTime>,tracing_core::metadata::LevelFilter,anstream::auto::AutoStream<std::io::stdio::Stderr> (*)()> * self, tracing_core::event::Event * event) Line 409
IbEverythingExt.dll!tracing_core::dispatcher::Dispatch::event(tracing_core::event::Event * self) Line 612
IbEverythingExt.dll!tracing_core::event::impl$0::dispatch::closure$0(tracing_core::event::impl$0::dispatch::closure_env$0 * current, tracing_core::dispatcher::Dispatch *) Line 36
IbEverythingExt.dll!tracing_core::dispatcher::get_default<tuple$<>,tracing_core::event::impl$0::dispatch::closure_env$0>(tracing_core::event::impl$0::dispatch::closure_env$0 f) Line 388
IbEverythingExt.dll!tracing_core::event::Event::dispatch(tracing_core::field::ValueSet * metadata) Line 37
IbEverythingExt.dll!everything_plugin::impl$2::handle::closure$9<IbEverythingExt::App>(everything_plugin::impl$2::handle::closure_env$9<IbEverythingExt::App> *, tracing_core::field::ValueSet value_set) Line 884
IbEverythingExt.dll!everything_plugin::PluginHandler<IbEverythingExt::App>::handle<IbEverythingExt::App>(unsigned int self, core::ffi::c_void * msg) Line 361
IbEverythingExt.dll!everything_plugin::PluginHandler<IbEverythingExt::App>::stop_kill<IbEverythingExt::App>() Line 242
IbEverythingExt.dll!IbEverythingExt::ffi::plugin_stop() Line 68
WindowsCodecs.dll!DllMain(HINSTANCE__ * hModule, unsigned long ul_reason_for_call, void * lpReserved) Line 30
WindowsCodecs.dll!dllmain_dispatch(HINSTANCE__ * const instance, const unsigned long reason, void * const reserved) Line 281
ntdll.dll!LdrpCallInitRoutineInternal()
ntdll.dll!LdrpCallInitRoutine()
ntdll.dll!LdrShutdownProcess()
ntdll.dll!RtlExitUserProcess()
kernel32.dll!ExitProcessImplementation()
Everything.exe!00007ff699c16c11()
Everything.exe!00007ff699c15ce6()
user32.dll!UserCallWinProcCheckWow(struct _ACTIVATION_CONTEXT *,__int64 (*)(struct tagWND *,unsigned int,unsigned __int64,__int64),struct HWND__ *,enum _WM_VALUE,unsigned __int64,__int64,void *,int)
user32.dll!DispatchMessageWorker()
Everything.exe!00007ff699a4aac5()
Everything.exe!00007ff699c94201()
kernel32.dll!BaseThreadInitThunk()
ntdll.dll!RtlUserThreadStart()
```
*/
#[cfg(test)]
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
    pub fn wait_start();
    static mut start_on_create: bool;

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

#[unsafe(no_mangle)]
extern "C" fn on_search_edit_created(hwnd: *mut c_void) {
    HANDLER.with_app(|a| {
        if a.config.search.ime_default_off() {
            ib_ime::hook::ImeHookConfig::default_off().hook_window(hwnd);
        }
    })
}
