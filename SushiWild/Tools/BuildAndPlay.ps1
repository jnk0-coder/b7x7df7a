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

# 3) 빌드 (Visual Studio 의 'C++를 사용한 게임 개발' + Windows 10/11 SDK 가 필요)
$BuildBat = Join-Path $Engine.Path 'Engine\Build\BatchFiles\Build.bat'

function Invoke-SushiBuild {
    Say '빌드 시작… 처음엔 5~15분 걸려유. 커피 한 잔 하셔유.' Cyan
    # Out-Host: 빌드 출력이 함수의 반환값에 섞이지 않게 화면으로만 보낸다
    & $BuildBat SushiWildEditor Win64 Development "-Project=$Project" -WaitMutex 2>&1 | Tee-Object -FilePath $Log | Out-Host
    return $LASTEXITCODE
}

# C++ 공구 상자(Visual Studio C++ 도구 + Windows SDK)가 없을 때 대신 설치해 준다.
function Install-BuildTools {
    $Components = @(
        'Microsoft.VisualStudio.Workload.NativeGame',          # C++를 사용한 게임 개발
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',   # MSVC 컴파일러
        'Microsoft.VisualStudio.Component.Windows11SDK.22621'  # Windows SDK (10.0.19041 이상이면 됨)
    )
    $AddArgs = ($Components | ForEach-Object { "--add $_" }) -join ' '
    $InstallerDir = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer'
    $VsWhere = Join-Path $InstallerDir 'vswhere.exe'
    $VsPath = $null
    if (Test-Path $VsWhere) {
        $VsPath = & $VsWhere -latest -products * -property installationPath
    }

    Say '관리자 권한을 묻는 창이 뜨면 "예"를 눌러 주셔유. 설치는 10~30분 걸려유.' Yellow
    if ($VsPath) {
        # 이미 깔린 Visual Studio 에 빠진 부품만 추가
        $Setup = Join-Path $InstallerDir 'setup.exe'
        $Proc = Start-Process $Setup -ArgumentList "modify --installPath `"$VsPath`" $AddArgs --includeRecommended --passive --norestart" -Verb RunAs -Wait -PassThru
    }
    elseif (Get-Command winget -ErrorAction SilentlyContinue) {
        # Visual Studio 가 아예 없으면 Community 2022 를 필요한 부품과 함께 설치
        $Proc = Start-Process winget -ArgumentList "install --id Microsoft.VisualStudio.2022.Community -e --accept-package-agreements --accept-source-agreements --override `"$AddArgs --includeRecommended --passive --wait --norestart`"" -Verb RunAs -Wait -PassThru
    }
    else {
        Say 'winget 이 없어서 자동 설치를 못 하겠슈. visualstudio.microsoft.com 에서 Visual Studio 2022 Community 를 받아' Red
        Say '"C++를 사용한 게임 개발" 과 "Windows 11 SDK" 를 체크해서 설치해 주셔유.' Red
        return $false
    }
    # 3010 = 설치는 됐고 재부팅 권장
    return ($Proc.ExitCode -eq 0 -or $Proc.ExitCode -eq 3010)
}

$Result = Invoke-SushiBuild
if ($Result -ne 0) {
    $LogText = Get-Content $Log -Raw -ErrorAction SilentlyContinue
    $MissingTools = $LogText -match 'SDK validation failed|not a valid platform to build|Visual Studio .* (must be|is not) installed|No valid Visual C\+\+|Unable to find valid .*toolchain'
    if ($MissingTools) {
        Say ''
        Say '빌드에 필요한 C++ 공구 상자(Windows SDK / Visual Studio C++)가 없슈.' Yellow
        $Answer = Read-Host '지가 대신 설치하고 다시 빌드할까유? (Y/N, 그냥 엔터 = Y)'
        if ($Answer -eq '' -or $Answer -match '^[Yy예ㅇ]') {
            if (Install-BuildTools) {
                Say '설치 끝! 다시 빌드해유.' Green
                $Result = Invoke-SushiBuild
            }
            else {
                Say '설치가 끝나지 않았거나 실패했슈. 컴퓨터를 한 번 재시작한 뒤 BuildAndPlay.bat 을 다시 실행해 주셔유.' Red
            }
        }
    }
}
if ($Result -ne 0) {
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
