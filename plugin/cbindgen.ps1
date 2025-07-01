$header = "../EverythingExt/ffi.h"
cbindgen > $header

# $content = Get-Content $header -Raw
# rm $header
# $content = $content -replace 'extern bool start\(', 'extern __declspec(dllexport) bool start('
# $content = $content -replace 'extern void stop\(', 'extern __declspec(dllexport) void stop('
# Set-Content $header $content
