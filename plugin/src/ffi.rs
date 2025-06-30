use std::ffi::c_char;

unsafe extern "C" {
    pub fn start(yaml: *const c_char) -> bool;
    pub fn stop();
}
