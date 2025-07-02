use std::env;

fn main() {
    let config = match env::var("PROFILE").unwrap().as_str() {
        "debug" => "Debug",
        "release" => "Release",
        _ => "Debug",
    };
    println!("cargo::rustc-link-lib=x64/{config}/EverythingExt");
    println!("cargo::rerun-if-changed=x64/{config}/EverythingExt.lib");

    println!("cargo::rustc-link-lib=external/build/_deps/ibwincpp-build/{config}/IbWinCpp");
    println!("cargo::rustc-link-lib=external/build/_deps/ibpinyin-build/{config}/IbPinyin");
    println!(
        "cargo::rustc-link-lib=external/build/_deps/ibeverything-build/everything-cpp/{config}/IbEverything"
    );

    // unsafe { env::set_var("VCPKGRS_TRIPLET", "x64-windows-static-release") };
    vcpkg::find_package("fmt").unwrap();
    vcpkg::find_package("detours").unwrap();
    vcpkg::find_package("yaml-cpp").unwrap();

    if cfg!(target_os = "windows") {
        let res = winres::WindowsResource::new();
        res.compile().unwrap();
    }
}
