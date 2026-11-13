# ===== Furry Doom Simulation - Config =====

# Popup stage
$Global:PopupCount      = 25            # how many popups
$Global:PopupMessages   = @(
    "Your computer is DOOMED!! *soft gasp* 🐾",
    "Windows will NOT start again!! ...maybe O.O",
    "A very serious furry has detected your files!",
    "Warning: this is a serious furry warning!!",
    "Your GPU has been turned into a tail-wagger!",
    "Please remain calm. *paws at keyboard*",
    "WARNING: Maximum fluff detected!",
    "This is your last warning!! (maybe not)",
    "nuzzles your hard drive OWO",
    "System32 has been petted!",
    "Doomed doomed doomed!! *happy barks*",
    "Serious business furry is SERIOUS. 😾"
)

# Flash stage
$Global:FlashSeconds    = 5
$Global:FlashColors     = @('White','Black','Magenta','Cyan','Lime')

# Fullscreen stage (image or animation)
$Global:FullscreenImage = 'C:\FurryTest\doomed.png'   # gif works too, animated in PictureBox

# Boot stage
$Global:BootImage       = 'C:\FurryTest\boot.png'
$Global:BootAudio       = 'C:\FurryTest\boot.wav'     # wav only (native playback)
$Global:BootText        = "SURPRISE!! your computer did not die *wags tail* <3"
$Global:BootSeconds     = 10