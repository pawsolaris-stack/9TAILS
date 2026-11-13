# Pre-logon furry warning. Runs before the desktop appears.
param()
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
. "C:\FurryTest\Config.ps1"

$form = New-Object Windows.Forms.Form
$form.FormBorderStyle = 'None'
$form.WindowState = 'Maximized'
$form.TopMost = $true
$form.StartPosition = 'Manual'
$form.Bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen

$pic = New-Object Windows.Forms.PictureBox
$pic.Dock = 'Fill'
$pic.SizeMode = 'Zoom'
if (Test-Path $Global:BootImage) { $pic.Image = [Drawing.Image]::FromFile($Global:BootImage) }
$form.Controls.Add($pic)

$label = New-Object Windows.Forms.Label
$label.Text = $Global:BootText
$label.ForeColor = 'White'
$label.BackColor = [Drawing.Color]::Transparent
$label.Font = New-Object Drawing.Font('Comic Sans MS', 28, [Drawing.FontStyle]::Bold)
$label.Dock = 'Bottom'
$label.TextAlign = 'MiddleCenter'
$label.Height = 120
$form.Controls.Add($label)

if (Test-Path $Global:BootAudio) {
    $player = New-Object System.Media.SoundPlayer $Global:BootAudio
    $player.Play()
}

$form.Show()
$t = [System.Windows.Forms.Application]
$end = (Get-Date).AddSeconds($Global:BootSeconds)
while ((Get-Date) -lt $end) { $t::DoEvents(); Start-Sleep -Milliseconds 50 }
$form.Close()