# https://www.voidtools.com/downloads/
# https://github.com/Chaoses-Ib/IbEverythingExt/issues/85
# $version = "1.4.1.1028"
$version = "1.4.1.1026"

pushd target/publish

wget http://www.voidtools.com/Everything.lng.zip
Expand-Archive Everything.lng.zip -DestinationPath Everything

wget http://www.voidtools.com/Everything-$version.x64.zip
Expand-Archive -Force Everything-$version.x64.zip -DestinationPath .
mv Everything.exe Everything/Everything.exe
$v = Get-Content v
$zip = "Everything.v${version}_IbEverythingExt_v$v.zip"
zip -r $zip Everything
rm Everything/Everything.exe

"- [Everything v1.4 便携整合包](https://github.com/Chaoses-Ib/IbEverythingExt/releases/download/v$v/$zip)（[国内加速](https://gh-proxy.com/github.com/Chaoses-Ib/IbEverythingExt/releases/download/v$v/$zip)）" >> release.md

popd
