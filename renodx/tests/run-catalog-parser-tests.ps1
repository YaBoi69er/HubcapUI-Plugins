param(
    # Also parse the current RenoDX wiki to catch structure changes upstream.
    [switch]$Live
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$output = Join-Path $root 'obj'
New-Item -ItemType Directory -Force -Path $output | Out-Null
$thirdParty = Join-Path $root 'third_party'
$objects = @('yyjson','miniz','miniz_tdef','miniz_tinfl','miniz_zip') | ForEach-Object { Join-Path $output "$_.obj" }
if ($objects | Where-Object { -not (Test-Path $_) }) { throw 'Run scripts/build.ps1 first to compile the third-party objects.' }

$wiki = ''
if ($Live) {
    $wiki = Join-Path $output 'Mods.md'
    Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/wiki/clshortfuse/renodx/Mods.md' -OutFile $wiki -UseBasicParsing
}

$vcvars = Get-ChildItem -Path @("${env:ProgramFiles}\Microsoft Visual Studio", "${env:ProgramFiles(x86)}\Microsoft Visual Studio") -Recurse -Filter vcvars64.bat -ErrorAction SilentlyContinue | Select-Object -First 1
$exe = Join-Path $output 'catalog-parser-test.exe'
$quotedObjects = ($objects | ForEach-Object { '"' + $_ + '"' }) -join ' '
$commands = @(
    "set `"PATH=%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem`"",
    "call `"$($vcvars.FullName)`" >NUL",
    "cl /nologo /EHsc /std:c++17 /utf-8 /DHUBCAP_UI_VERSION=\`"test\`" /I`"$(Join-Path $root 'sdk')`" /I`"$(Join-Path $root 'src')`" /I`"$thirdParty`" `"$(Join-Path $PSScriptRoot 'catalog-parser-test.cpp')`" $quotedObjects /Fo:`"$output\catalog-parser-test.obj`" /Fe:`"$exe`" dbghelp.lib winhttp.lib bcrypt.lib gdi32.lib shell32.lib shlwapi.lib user32.lib ole32.lib crypt32.lib",
    'if errorlevel 1 exit /b %errorlevel%',
    "`"$exe`" $(if ($wiki) { '"' + $wiki + '"' })"
)
$cmdFile = Join-Path $output 'catalog-parser-test.cmd'
Set-Content -LiteralPath $cmdFile -Value ($commands -join "`r`n") -Encoding ASCII
cmd /c "`"$cmdFile`""
if ($LASTEXITCODE -ne 0) { throw 'RenoDX catalog parser tests failed.' }
