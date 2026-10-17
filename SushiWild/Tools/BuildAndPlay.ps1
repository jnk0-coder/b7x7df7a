# 스시 와일드: 엔진 찾기 → 프로젝트 버전 맞추기 → 빌드 → 실행을 한 번에.
# BuildAndPlay.bat(게임 바로 실행) 또는 OpenEditor.bat(에디터로 열기)을 더블클릭하면 이 스크립트가 돈다.
param(
    [switch]$Editor
)

$ProjectDir = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $ProjectDir 'SushiWild.uproject'
$Log = Join-Path $ProjectDir 'BuildLog.txt'

function Say([string]$Text, [string]$Color = 'White') { Write-Host $Text -ForegroundColor $Color }

Say '=== 스시 와일드 자동 빌드 & 실행 ===' Cyan

if (-not (Test-Path $Project)) {
    Say "SushiWild.uproject 를 못 찾았슈: $Project" Red
    exit 1
}
if ($ProjectDir -match '[^\x00-\x7F]') {
    Say '주의: 폴더 경로에 한글 같은 영어 아닌 글자가 있슈. 빌드가 실패하면 C:\Games\SushiWild 같은 곳으로 옮겨 주셔유.' Yellow
}

# 1) 설치된 언리얼 엔진 찾기: Epic 런처 설치 목록 → 기본 설치 폴더 순서
$Engines = @()
$Manifest = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
if (Test-Path $Manifest) {
    $Installed = (Get-Content $Manifest -Raw | ConvertFrom-Json).InstallationList
    foreach ($Item in $Installed) {
        if ($Item.AppName -match '^UE_(\d+\.\d+)$') {
            $Engines += [pscustomobject]@{ Version = [version]$Matches[1]; Path = $Item.InstallLocation }
        }
    }
}
if ($Engines.Count -eq 0) {
    foreach ($Dir in Get-ChildItem (Join-Path $env:ProgramFiles 'Epic Games') -Directory -Filter 'UE_*' -ErrorAction SilentlyContinue) {
        if ($Dir.Name -match '^UE_(\d+\.\d+)$') {
            $Engines += [pscustomobject]@{ Version = [version]$Matches[1]; Path = $Dir.FullName }
        }
    }
}

$Engine = $Engines |
    Where-Object { $_.Version -ge [version]'5.5' -and (Test-Path (Join-Path $_.Path 'Engine\Build\BatchFiles\Build.bat')) } |
    Sort-Object Version |
    Select-Object -Last 1
if (-not $Engine) {
    Say '언리얼 엔진 5.5 이상을 못 찾았슈. Epic 런처 라이브러리에 설치돼 있는지 확인해 주셔유.' Red
    exit 1
}
$EngineVersion = '{0}.{1}' -f $Engine.Version.Major, $Engine.Version.Minor
Say "엔진 찾음: UE $EngineVersion  ($($Engine.Path))" Green

# 2) 프로젝트의 엔진 버전을 설치된 버전으로 맞추기 (Switch Unreal Engine version 메뉴 대신)
$Text = [IO.File]::ReadAllText($Project)
$NewText = [regex]::Replace($Text, '"EngineAssociation"\s*:\s*"[^"]*"', ('"EngineAssociation": "{0}"' -f $EngineVersion))
if ($NewText -ne $Text) {
    [IO.File]::WriteAllText($Project, $NewText)
    Say "프로젝트 엔진 버전을 $EngineVersion 으로 맞췄슈." Green
}

# 3) 빌드 (Visual Studio 2022 의 'C++를 사용한 게임 개발' 이 필요)
Say '빌드 시작… 처음엔 5~15분 걸려유. 커피 한 잔 하셔유.' Cyan
$BuildBat = Join-Path $Engine.Path 'Engine\Build\BatchFiles\Build.bat'
& $BuildBat SushiWildEditor Win64 Development "-Project=$Project" -WaitMutex 2>&1 | Tee-Object -FilePath $Log
if ($LASTEXITCODE -ne 0) {
    Say ''
    Say '빌드가 실패했슈.' Red
    Say '아래 BuildLog.txt 파일을 티모시(Claude)한테 보내 주시면 고쳐 드릴게유:' Yellow
    Say "  $Log" Yellow
    exit 1
}
Say '빌드 성공!' Green

# 4) 실행
$EditorExe = Join-Path $Engine.Path 'Engine\Binaries\Win64\UnrealEditor.exe'
if ($Editor) {
    Start-Process $EditorExe -ArgumentList ('"{0}"' -f $Project)
    Say '에디터를 여는 중이에유. 뜨면 위쪽 Play(▶) 를 누르셔유.' Green
}
else {
    Start-Process $EditorExe -ArgumentList ('"{0}" -game -windowed -ResX=1600 -ResY=900' -f $Project)
    Say '게임을 바로 켜는 중이에유. 첫 실행은 셰이더 준비 때문에 몇 분 걸릴 수 있슈!' Green
}
