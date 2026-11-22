# Dev setup for piano-tutor on Windows. Safe to re-run.
# Installs with winget: Python 3.12, gitleaks, Espressif's ESP-IDF Installation Manager (EIM);
# then ESP-IDF v5.5.5 for the ESP32-P4 (Tab5) into C:\Espressif, turns on the gitleaks
# pre-commit hook, and builds the firmware.
#   powershell -ExecutionPolicy Bypass -File scripts\setup-windows.ps1 [-NoBuild]
param([switch]$NoBuild)
$ErrorActionPreference = 'Stop'
$IdfVersion = 'v5.5.5'
$Repo = Split-Path -Parent $PSScriptRoot

function Step($text) { Write-Host "`n==> $text" -ForegroundColor Cyan }
function Refresh-Path {
	$env:Path = [Environment]::GetEnvironmentVariable('Path', 'User') + ';' + [Environment]::GetEnvironmentVariable('Path', 'Machine')
}
function Find-IdfProfile {
	Get-ChildItem 'C:\Espressif\tools' -Filter "*$IdfVersion*PowerShell_profile.ps1" -ErrorAction SilentlyContinue | Select-Object -First 1
}

Step 'Installing Python 3.12, gitleaks and the ESP-IDF installer (winget)'
$wingetArgs = @('--silent', '--accept-package-agreements', '--accept-source-agreements', '--disable-interactivity')
foreach ($id in 'Python.Python.3.12', 'Gitleaks.Gitleaks', 'Espressif.EIM-CLI') {
	$scope = if ($id -eq 'Python.Python.3.12') { @('--scope', 'user') } else { @() }
	winget list --id $id --exact *> $null
	if ($LASTEXITCODE -eq 0) { Write-Host "$id already installed"; continue }
	winget install --id $id --exact @scope @wingetArgs
	if ($LASTEXITCODE -ne 0) { throw "winget could not install $id" }
}
Refresh-Path

Step 'Enabling the gitleaks pre-commit hook'
git -C $Repo config core.hooksPath .githooks

Step "Installing ESP-IDF $IdfVersion for esp32p4 (large download, takes a while)"
if (Find-IdfProfile) {
	Write-Host 'already installed'
} else {
	# Source goes to C:\esp\<version>\esp-idf, tools to C:\Espressif; keep eim's config file out of the repo
	eim install -i $IdfVersion -t esp32p4 -n true -a true --do-not-track true --cleanup true `
		--config-file-save-path (Join-Path $env:TEMP 'eim_config.toml')
	if ($LASTEXITCODE -ne 0) { throw 'eim install failed (log: %LOCALAPPDATA%\eim\logs)' }
}
$profileScript = Find-IdfProfile
if (-not $profileScript) { throw "ESP-IDF $IdfVersion activation script not found in C:\Espressif\tools" }

if (-not $NoBuild) {
	Step 'Building the firmware'
	. $profileScript.FullName
	Push-Location (Join-Path $Repo 'firmware')
	try {
		idf.py set-target esp32p4
		if ($LASTEXITCODE -ne 0) { throw 'idf.py set-target failed' }
		idf.py build
		if ($LASTEXITCODE -ne 0) { throw 'idf.py build failed' }
	} finally { Pop-Location }
}

Step 'Done'
Write-Host "In a new PowerShell, activate ESP-IDF before building or flashing:"
Write-Host "  . '$($profileScript.FullName)'"
Write-Host "  cd '$Repo\firmware'; idf.py build"
Write-Host "  idf.py -p COM5 flash monitor    # Tab5 on its USB-C port; check the COM number in Device Manager"
