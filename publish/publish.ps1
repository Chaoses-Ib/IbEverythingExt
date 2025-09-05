# Bump version
# Build VS EverythingExt, loader, Updater

cargo build -p plugin -r
if (!$?) {
    Write-Host "Build failed"
    exit 1
}

rm -r target/publish

mkdir target/publish/Everything/Plugins
cp target/release/IbEverythingExt.dll target/publish/Everything/Plugins/IbEverythingExt.dll

cp -r publish/IbEverythingExt target/publish/Everything/Plugins
cp x64/Release/Updater.exe target/publish/Everything/Plugins/IbEverythingExt/Updater.exe

cp x64/Release/WindowsCodecs.dll target/publish/Everything/WindowsCodecs.dll

# Read version from Cargo.toml
$cargoToml = Get-Content "Cargo.toml" -Raw
$version = ($cargoToml | Select-String 'version = "([^"]+)"').Matches[0].Groups[1].Value

# Save version to target/publish/v
$version | Out-File -FilePath "target/publish/v" -Encoding utf8 -NoNewline

pushd target/publish
zip -r "IbEverythingExt.v$version.zip" Everything
"### 便携整合包 (Portable packages)" >> release.md
popd

.\publish\publish-14.ps1
.\publish\publish-15.ps1
