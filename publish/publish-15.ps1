# https://www.voidtools.com/forum/viewtopic.php?t=9787
$version = "1.5.0.1396a"
# https://www.voidtools.com/forum/viewtopic.php?f=12&t=9799
$httpVersion = "1.0.3.4"
$etpVersion = "1.0.1.4"
$everythingServerVersion = "1.0.1.2"

pushd target/publish

wget -O Everything.zip http://www.voidtools.com/Everything-$version.x64.zip
Expand-Archive Everything.zip -DestinationPath Everything

ni Everything/No_Alpha_Instance

wget -O Everything-HTTP-Server.zip http://www.voidtools.com/Everything-HTTP-Server-$httpVersion.x64.zip
Expand-Archive Everything-HTTP-Server.zip -DestinationPath Everything/Plugins

wget http://www.voidtools.com/Everything-ETP-Server-$etpVersion.x64.zip
Expand-Archive Everything-ETP-Server-$etpVersion.x64.zip -DestinationPath Everything/Plugins

wget http://www.voidtools.com/Everything-Server-$everythingServerVersion.x64.zip
Expand-Archive Everything-Server-$everythingServerVersion.x64.zip -DestinationPath Everything/Plugins

$v = Get-Content v
$zip = "Everything.v${version}_IbEverythingExt_v$v.zip"
zip -r $zip Everything

"- [Everything v1.5 便携整合包](https://github.com/Chaoses-Ib/IbEverythingExt/releases/download/v$v/$zip)（[国内加速](https://gh-proxy.com/github.com/Chaoses-Ib/IbEverythingExt/releases/download/v$v/$zip)）" >> release.md

popd
