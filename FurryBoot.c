/** @file FurryBoot.c
 *  Pre-OS furry warning: draws BMP, plays PC-speaker melody,
 *  prints configurable text, then chainloads Windows bootmgfw.efi.
 *  Reads its assets from \EFI\Furry\boot.bmp and \EFI\Furry\boot.cfg (text lines).
 **/

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>`
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>

// ===== Default runtime values; override via boot.cfg lines "TEXT=...", "SECONDS=10" =====
STATIC CHAR16 gBootText[] = L"SURPRISE!! your computer did not die *wags tail* <3";
STATIC UINTN gSeconds    = 10;

STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL *gGop = NULL;

STATIC EFI_STATUS ReadWholeFile (IN CHAR16 *Path, OUT VOID **Data, OUT UINTN *Size)
{
  EFI_STATUS                       st;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
  EFI_FILE_PROTOCOL               *Root, *File;
  EFI_FILE_INFO                   *Info = NULL;
  UINTN                            InfoSize = 0;
  EFI_GUID                         FileInfoGuid = EFI_FILE_INFO_ID;

  st = gBS->LocateProtocol (&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID **)&Fs);
  if (EFI_ERROR (st)) return st;
  st = Fs->OpenVolume (Fs, &Root);
  if (EFI_ERROR (st)) return st;
  st = Root->Open (Root, &File, Path, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (st)) return st;

  st = File->GetInfo (File, &FileInfoGuid, &InfoSize, NULL);
  if (st == EFI_BUFFER_TOO_SMALL) {
    Info = AllocatePool (InfoSize);
    st   = File->GetInfo (File, &FileInfoGuid, &InfoSize, (VOID *)Info);
  } else if (EFI_ERROR (st)) {
    File->Close (File);
    return st;
  }
  *Size = (UINTN)Info->FileSize;
  *Data = AllocatePool (*Size);
  st    = File->Read (File, Size, *Data);
  File->Close (File);
  return st;
}

/** Parse an uncompressed 24/32bpp BMP and Blt it fullscreen. */
STATIC VOID DrawBmp (IN VOID *BmpData)
{
  UINT8   *p     = (UINT8 *)BmpData;
  UINT32   Off   = *(UINT32 *)(p + 10);   // pixel data offset
  INT32    W     = *(INT32  *)(p + 18);
  INT32    H     = *(INT32  *)(p + 22);   // negative => top-down
  UINT16   Bpp   = *(UINT16 *)(p + 28);
  UINT32   RowBytes;
  UINT32   X, Y;
  VOID    *BltBuf;
  UINT8   *Src, *DstRow, *Dst;

  if (p[0] != 'B' || p[1] != 'M') return;
  if (Bpp != 24 && Bpp != 32)     return;

  RowBytes = ((UINT32)W * Bpp + 31) / 32 * 4;
  BltBuf   = AllocatePool ((UINTN)W * H * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  if (BltBuf == NULL) return;

  for (Y = 0; Y < (UINT32)(H < 0 ? -H : H); Y++) {
    // BMP scans are bottom-up when H > 0
    UINT32  SrcY   = (H > 0) ? ((UINT32)H - 1 - Y) : Y;
    Src            = p + Off + SrcY * RowBytes;
    DstRow         = (UINT8 *)BltBuf + (UINTN)Y * W * 4;
    for (X = 0; X < (UINT32)W; X++) {
      Dst = DstRow + X * 4;
      Dst[0] = Src[0];           // Blue
      Dst[1] = Src[1];           // Green
      Dst[2] = Src[2];           // Red
      Dst[3] = 0xFF;             // Reserved
      Src   += (Bpp / 8);
    }
  }

  if (gGop != NULL) {
    gGop->Blt (gGop, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)BltBuf,
               EfiBltBufferToVideo, 0, 0, 0, 0,
               (UINTN)W, (UINTN)(H < 0 ? -H : H), 0);
  }
  FreePool (BltBuf);
}

/** PC speaker beep via PIT channel 2 (classic but real pre-OS audio). */
STATIC VOID Beep (IN UINT32 FreqHz, IN UINT32 Ms)
{
  UINT32 Div;
  UINT8  Old;

  if (FreqHz < 20 || FreqHz > 20000) return;
  Div = 1193182 / FreqHz;
  IoWrite8 (0x43, 0xB6);
  IoWrite8 (0x42, (UINT8)(Div & 0xFF));
  IoWrite8 (0x42, (UINT8)((Div >> 8) & 0xFF));
  Old = IoRead8 (0x61);
  IoWrite8 (0x61, Old | 0x03);
  gBS->Stall (Ms * 1000);
  IoWrite8 (0x61, Old & ~0x03);
}

/** Tiny melody = your "audio" pre-OS (WAV playback needs a driver stack; beeps are the honest UEFI option). */
STATIC VOID PlayMelody (VOID)
{
  STATIC struct { UINT32 Freq; UINT32 Ms; } Tune[] = {
    { 523, 150 }, { 659, 150 }, { 784, 150 }, { 1046, 300 },  // C5 E5 G5 C6
    { 784, 150 }, { 1046, 400 },
  };
  UINTN i;
  for (i = 0; i < sizeof (Tune) / sizeof (Tune[0]); i++) {
    Beep (Tune[i].Freq, Tune[i].Ms);
    gBS->Stall (60000);
  }
}

STATIC VOID LoadConfig (VOID)
{
  VOID  *Data = NULL;
  UINTN Size  = 0;
  CHAR16 *Line, *End, *Val;

  if (EFI_ERROR (ReadWholeFile (L"\\EFI\\Furry\\boot.cfg", &Data, &Size))) {
    if (Data) FreePool (Data);
    return;
  }
  Line = (CHAR16 *)Data;
  End  = (CHAR16 *)((UINT8 *)Data + Size);
  while (Line < End) {
    if (StrnCmp (Line, L"TEXT=", 5) == 0) {
      Val = Line + 5;
      while (*Val != L'\0' && *Val != L'\r' && *Val != L'\n') Val++;
      *Val = L'\0';
      StrCpyS (gBootText, sizeof (gBootText) / sizeof (CHAR16), Line + 5);
    } else if (StrnCmp (Line, L"SECONDS=", 8) == 0) {
      gSeconds = (UINTN)StrDecimalToUintn (Line + 8);
    }
    while (Line < End && *Line != L'\n') Line++;
    if (Line < End) Line++;
  }
  FreePool (Data);
}

EFI_STATUS ChainloadWindows (VOID)
{
  EFI_STATUS st;
  VOID      *Data = NULL;
  UINTN      Size = 0;
  EFI_HANDLE Img  = NULL;

  st = ReadWholeFile (L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi", &Data, &Size);
  if (EFI_ERROR (st)) return st;

  st = gBS->LoadImage (FALSE, gImageHandle, NULL, Data, Size, &Img);
  if (Data) FreePool (Data);
  if (EFI_ERROR (st)) return st;

  return gBS->StartImage (Img, NULL, NULL);
}

EFI_STATUS
EFIAPI
FurryBootEntryPoint (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS st;
  VOID      *Bmp = NULL;
  UINTN      BmpSize = 0;
  UINTN      i;
  CHAR16    *Text;
  UINTN      TextLen;

  st = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&gGop);
  if (EFI_ERROR (st)) gGop = NULL;

  LoadConfig ();

  // Black the screen and print our warning text (drawn image comes right after)
  if (SystemTable->ConOut != NULL) {
    SystemTable->ConOut->SetAttribute (SystemTable->ConOut, EFI_TEXT_ATTR (EFI_WHITE, EFI_BLACK));
    SystemTable->ConOut->ClearScreen (SystemTable->ConOut);
    TextLen = StrLen (gBootText);
    // center-ish placement
    for (i = 0; i < 10; i++) Print (L"\r\n");
    for (i = 0; i < 20; i++) Print (L" ");
    // ConOut has no per-char positioning here; just print the line
    Print (L"%s\r\n", gBootText);
  }

  st = ReadWholeFile (L"\\EFI\\Furry\\boot.bmp", &Bmp, &BmpSize);
  if (!EFI_ERROR (st)) {
    DrawBmp (Bmp);
    FreePool (Bmp);
    // re-print text over image via GOP text overlay is not available; text stays on ConOut layer
  }

  PlayMelody ();

  // Configurable hold time
  for (i = 0; i < gSeconds * 100; i++) {
    gBS->Stall (10000);   // 10 ms ticks
  }

  st = ChainloadWindows ();
  if (EFI_ERROR (st)) {
    if (SystemTable->ConOut) {
      SystemTable->ConOut->SetAttribute (SystemTable->ConOut, EFI_TEXT_ATTR (EFI_RED, EFI_BLACK));
      Print (L"\r\nFurryBoot: chainload failed: %r\r\n", st);
      Print (L"Press any key to attempt recovery reboot...\r\n");
    }
    UINTN Index;
    gBS->WaitForEvent (1, &SystemTable->ConIn->WaitForKey, &Index);
    gRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
  }
  return EFI_SUCCESS;
}