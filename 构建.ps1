param(
    [ValidateSet('Release','Debug')][string]$Configuration = 'Release',
    [switch]$Test
)
$ErrorActionPreference = 'Stop'
$workspacePath = $PSScriptRoot
$projectPath = Join-Path $workspacePath '源码\NanjingMetro'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw '未找到Visual Studio Installer，请安装VS 2022的C++桌面开发与MFC组件。' }
$msbuild = @(& $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe') | Select-Object -First 1
if (-not $msbuild) { throw '未找到MSBuild，请检查Visual Studio安装。' }
New-Item -ItemType Directory -Path (Join-Path $workspacePath '临时文件') -Force | Out-Null
function Build-Project([string]$name, [string]$output, [string]$cache) {
    $logPath = Join-Path $workspacePath "临时文件\${name}_${Configuration}.log"
    & $msbuild (Join-Path $projectPath ($name+'.vcxproj')) "/p:Configuration=$Configuration" '/p:Platform=x64' "/p:OutDir=$output\" "/p:IntDir=$cache\" /m /v:normal /nologo | Tee-Object -FilePath $logPath | Select-Object -Last 9
    if ($LASTEXITCODE -ne 0) { throw "构建失败，详见 $logPath" }
}
$outputPath = if ($Configuration -eq 'Release') { Join-Path $workspacePath '可运行程序' } else { Join-Path $workspacePath '构建产物\Debug' }
Build-Project 'NanjingMetro' $outputPath (Join-Path $workspacePath "构建缓存\NanjingMetro\$Configuration")
if ($Test) {
    $testOutput = Join-Path $workspacePath "构建产物\Tests\$Configuration"
    Build-Project 'IntegrationTests' $testOutput (Join-Path $workspacePath "构建缓存\IntegrationTests\$Configuration")
    Push-Location -LiteralPath $testOutput
    try {
        & (Join-Path $testOutput 'IntegrationTests.exe') | Tee-Object -FilePath (Join-Path $workspacePath "临时文件\测试结果_${Configuration}.txt")
        if ($LASTEXITCODE -ne 0) { throw '集成测试失败。' }
    } finally { Pop-Location }
}
Write-Output "程序输出：$outputPath"
