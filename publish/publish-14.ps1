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
zip -r "Everything v${version}_IbEverythingExt_v$v.zip" Everything
rm Everything/Everything.exe

popd
