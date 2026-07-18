/** @file
  BgrtLogoDumpApp - ACPI XSDT and BGRT logo dump utility.

  This UEFI Shell application demonstrates how to:
    1. Locate the ACPI RSDP from the EFI Configuration Table.
    2. Walk the XSDT and print ACPI table headers.
    3. Locate and decode the BGRT (Boot Graphics Resource Table).
    4. Export the BGRT BMP logo to the current boot device using
       EFI_LOADED_IMAGE_PROTOCOL and EFI_SIMPLE_FILE_SYSTEM_PROTOCOL.

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "BgrtLogoDumpApp.h"

VOID
SignatureToString (
  IN  UINT32  Signature,
  OUT CHAR8   SignatureString[5]
  )
{
  SignatureString[0] = (CHAR8)(Signature & 0xFF);
  SignatureString[1] = (CHAR8)((Signature >> 8) & 0xFF);
  SignatureString[2] = (CHAR8)((Signature >> 16) & 0xFF);
  SignatureString[3] = (CHAR8)((Signature >> 24) & 0xFF);
  SignatureString[4] = '\0';
}

BOOLEAN
IsChecksumValid (
  IN VOID   *Buffer,
  IN UINTN  Length
  )
{
  UINT8  Sum;
  UINT8  *Ptr;
  UINTN  Index;

  if ((Buffer == NULL) || (Length == 0)) {
    return FALSE;
  }

  Sum = 0;
  Ptr = (UINT8 *)Buffer;

  for (Index = 0; Index < Length; Index++) {
    Sum = (UINT8)(Sum + Ptr[Index]);
  }

  return (BOOLEAN)(Sum == 0);
}

EFI_STATUS
LocateRsdp (
  OUT EFI_ACPI_5_0_ROOT_SYSTEM_DESCRIPTION_POINTER  **Rsdp,
  OUT BOOLEAN                                       *IsAcpi20
  )
{
  EFI_STATUS  Status;

  if ((Rsdp == NULL) || (IsAcpi20 == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Rsdp     = NULL;
  *IsAcpi20 = FALSE;

  Status = EfiGetSystemConfigurationTable (
             &gEfiAcpi20TableGuid,
             (VOID **)Rsdp
             );
  if (!EFI_ERROR (Status) && (*Rsdp != NULL)) {
    *IsAcpi20 = TRUE;
    return EFI_SUCCESS;
  }

  Status = EfiGetSystemConfigurationTable (
             &gEfiAcpi10TableGuid,
             (VOID **)Rsdp
             );
  if (!EFI_ERROR (Status) && (*Rsdp != NULL)) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

VOID
DumpRsdp (
  IN EFI_ACPI_5_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp,
  IN BOOLEAN                                       IsAcpi20
  )
{
  CHAR8  OemId[7];

  CopyMem (OemId, Rsdp->OemId, sizeof (Rsdp->OemId));
  OemId[6] = '\0';

  ShellPrintEx (-1, -1, L"=== RSDP (Root System Description Pointer) ===\n");
  ShellPrintEx (-1, -1, L"  Address            : 0x%p\n", Rsdp);
  ShellPrintEx (-1, -1, L"  Source GUID        : %s\n", IsAcpi20 ? L"gEfiAcpi20TableGuid" : L"gEfiAcpi10TableGuid");
  ShellPrintEx (-1, -1, L"  Signature          : RSD PTR \n");
  ShellPrintEx (-1, -1, L"  Checksum           : 0x%02x (%s)\n",
    (UINTN)Rsdp->Checksum,
    IsChecksumValid (Rsdp, 20) ? L"OK" : L"BAD"
    );
  ShellPrintEx (-1, -1, L"  OEM ID             : %a\n", OemId);
  ShellPrintEx (-1, -1, L"  Revision           : 0x%02x\n", (UINTN)Rsdp->Revision);
  ShellPrintEx (-1, -1, L"  RSDT Address       : 0x%08x\n", (UINTN)Rsdp->RsdtAddress);

  if (IsAcpi20 && (Rsdp->Revision >= 2)) {
    ShellPrintEx (-1, -1, L"  Length             : 0x%08x (%d)\n",
      (UINTN)Rsdp->Length, (UINTN)Rsdp->Length);
    ShellPrintEx (-1, -1, L"  XSDT Address       : 0x%016lx\n", (UINTN)Rsdp->XsdtAddress);
    ShellPrintEx (-1, -1, L"  Extended Checksum  : 0x%02x (%s)\n",
      (UINTN)Rsdp->ExtendedChecksum,
      IsChecksumValid (Rsdp, Rsdp->Length) ? L"OK" : L"BAD"
      );
  }

  ShellPrintEx (-1, -1, L"\n");
}

VOID
DumpAcpiHeader (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Header
  )
{
  CHAR8  Signature[5];
  CHAR8  OemId[7];
  CHAR8  OemTableId[9];
  CHAR8  CreatorId[5];

  SignatureToString (Header->Signature, Signature);
  CopyMem (OemId, Header->OemId, sizeof (Header->OemId));
  OemId[6] = '\0';
  CopyMem (OemTableId, &Header->OemTableId, sizeof (Header->OemTableId));
  OemTableId[8] = '\0';
  SignatureToString (Header->CreatorId, CreatorId);

  ShellPrintEx (-1, -1, L"  Signature       : %a\n", Signature);
  ShellPrintEx (-1, -1, L"  Address         : 0x%p\n", Header);
  ShellPrintEx (-1, -1, L"  Length          : 0x%08x (%d)\n",
    (UINTN)Header->Length, (UINTN)Header->Length);
  ShellPrintEx (-1, -1, L"  Revision        : 0x%02x\n", (UINTN)Header->Revision);
  ShellPrintEx (-1, -1, L"  Checksum        : 0x%02x (%s)\n",
    (UINTN)Header->Checksum,
    IsChecksumValid (Header, Header->Length) ? L"OK" : L"BAD"
    );
  ShellPrintEx (-1, -1, L"  OEM ID          : %a\n", OemId);
  ShellPrintEx (-1, -1, L"  OEM Table ID    : %a\n", OemTableId);
  ShellPrintEx (-1, -1, L"  OEM Revision    : 0x%08x\n", (UINTN)Header->OemRevision);
  ShellPrintEx (-1, -1, L"  Creator ID      : %a\n", CreatorId);
  ShellPrintEx (-1, -1, L"  Creator Revision: 0x%08x\n", (UINTN)Header->CreatorRevision);
}

EFI_STATUS
WalkXsdt (
  IN  EFI_ACPI_DESCRIPTION_HEADER                *Xsdt,
  OUT EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE  **Bgrt
  )
{
  UINTN                        EntryCount;
  UINTN                        Index;
  UINT64                       *Entry;
  EFI_ACPI_DESCRIPTION_HEADER  *Header;
  CHAR8                        Signature[5];

  if ((Xsdt == NULL) || (Bgrt == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Bgrt = NULL;

  if (Xsdt->Signature != EFI_ACPI_5_0_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
    return EFI_UNSUPPORTED;
  }

  if (Xsdt->Length < sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
    return EFI_COMPROMISED_DATA;
  }

  EntryCount = (Xsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT64);
  Entry      = (UINT64 *)((UINT8 *)Xsdt + sizeof (EFI_ACPI_DESCRIPTION_HEADER));

  ShellPrintEx (-1, -1, L"=== XSDT (Extended System Description Table) ===\n");
  DumpAcpiHeader (Xsdt);
  ShellPrintEx (-1, -1, L"  Entry Count     : %d\n\n", EntryCount);

  ShellPrintEx (-1, -1, L"=== XSDT Entries ===\n");

  for (Index = 0; Index < EntryCount; Index++) {
    Header = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Entry[Index];
    if (Header == NULL) {
      ShellPrintEx (-1, -1, L"  [%02d] NULL\n", Index);
      continue;
    }

    SignatureToString (Header->Signature, Signature);
    ShellPrintEx (-1, -1, L"  [%02d] %a  Address=0x%p  Length=0x%08x  Rev=0x%02x  Csum=%s\n",
      Index,
      Signature,
      Header,
      (UINTN)Header->Length,
      (UINTN)Header->Revision,
      IsChecksumValid (Header, Header->Length) ? L"OK" : L"BAD"
      );

    if (Header->Signature == EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE_SIGNATURE) {
      *Bgrt = (EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE *)Header;
    }
  }

  ShellPrintEx (-1, -1, L"\n");
  return (*Bgrt != NULL) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

VOID
DumpBgrt (
  IN EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE  *Bgrt
  )
{
  ShellPrintEx (-1, -1, L"=== BGRT (Boot Graphics Resource Table) ===\n");
  DumpAcpiHeader (&Bgrt->Header);
  ShellPrintEx (-1, -1, L"  Version         : 0x%04x\n", (UINTN)Bgrt->Version);
  ShellPrintEx (-1, -1, L"  Status          : 0x%02x (%s)\n",
    (UINTN)Bgrt->Status,
    (Bgrt->Status == EFI_ACPI_5_0_BGRT_STATUS_VALID) ? L"Valid/Displayed" : L"Invalid/Not Displayed"
    );
  ShellPrintEx (-1, -1, L"  Image Type      : 0x%02x (%s)\n",
    (UINTN)Bgrt->ImageType,
    (Bgrt->ImageType == EFI_ACPI_5_0_BGRT_IMAGE_TYPE_BMP) ? L"BMP" : L"Unknown"
    );
  ShellPrintEx (-1, -1, L"  Image Address   : 0x%016lx\n", (UINTN)Bgrt->ImageAddress);
  ShellPrintEx (-1, -1, L"  Image Offset X  : %d\n", (UINTN)Bgrt->ImageOffsetX);
  ShellPrintEx (-1, -1, L"  Image Offset Y  : %d\n", (UINTN)Bgrt->ImageOffsetY);
  ShellPrintEx (-1, -1, L"\n");
}

EFI_STATUS
GetBgrtBmpSize (
  IN  VOID   *Image,
  OUT UINTN  *FileSize
  )
{
  BMP_IMAGE_HEADER  *Bmp;

  if ((Image == NULL) || (FileSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Bmp = (BMP_IMAGE_HEADER *)Image;

  if ((Bmp->CharB != 'B') || (Bmp->CharM != 'M')) {
    return EFI_UNSUPPORTED;
  }

  if ((Bmp->Size < sizeof (BMP_IMAGE_HEADER)) ||
      (Bmp->Size > BGRT_LOGO_MAX_BMP_SIZE) ||
      (Bmp->ImageOffset >= Bmp->Size))
  {
    return EFI_BAD_BUFFER_SIZE;
  }

  if ((Bmp->PixelWidth == 0) || (Bmp->PixelHeight == 0)) {
    return EFI_UNSUPPORTED;
  }

  ShellPrintEx (-1, -1, L"=== BGRT BMP Image ===\n");
  ShellPrintEx (-1, -1, L"  Address         : 0x%p\n", Image);
  ShellPrintEx (-1, -1, L"  File Size       : 0x%08x (%d)\n", (UINTN)Bmp->Size, (UINTN)Bmp->Size);
  ShellPrintEx (-1, -1, L"  Image Offset    : 0x%08x\n", (UINTN)Bmp->ImageOffset);
  ShellPrintEx (-1, -1, L"  Header Size     : 0x%08x\n", (UINTN)Bmp->HeaderSize);
  ShellPrintEx (-1, -1, L"  Width           : %d\n", (UINTN)Bmp->PixelWidth);
  ShellPrintEx (-1, -1, L"  Height          : %d\n", (UINTN)Bmp->PixelHeight);
  ShellPrintEx (-1, -1, L"  Bit Per Pixel   : %d\n", (UINTN)Bmp->BitPerPixel);
  ShellPrintEx (-1, -1, L"  Compression     : 0x%08x\n", (UINTN)Bmp->CompressionType);
  ShellPrintEx (-1, -1, L"\n");

  *FileSize = (UINTN)Bmp->Size;
  return EFI_SUCCESS;
}

EFI_STATUS
WriteLogoFile (
  IN CHAR16  *FileName,
  IN VOID    *Buffer,
  IN UINTN   Size
  )
{
  EFI_STATUS                       Status;
  EFI_LOADED_IMAGE_PROTOCOL        *LoadedImage;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFileSystem;
  EFI_FILE_PROTOCOL                *Root;
  EFI_FILE_PROTOCOL                *File;
  UINTN                            WriteSize;

  if ((FileName == NULL) || (Buffer == NULL) || (Size == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  LoadedImage      = NULL;
  SimpleFileSystem = NULL;
  Root             = NULL;
  File             = NULL;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  LoadedImage->DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&SimpleFileSystem
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SimpleFileSystem->OpenVolume (SimpleFileSystem, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Root->Open (
                   Root,
                   &File,
                   FileName,
                   EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                   0
                   );
  if (EFI_ERROR (Status)) {
    Root->Close (Root);
    return Status;
  }

  WriteSize = Size;
  Status    = File->Write (File, &WriteSize, Buffer);
  if (!EFI_ERROR (Status) && (WriteSize != Size)) {
    Status = EFI_VOLUME_FULL;
  }

  File->Flush (File);
  File->Close (File);
  Root->Close (Root);
  return Status;
}

INTN
EFIAPI
ShellAppMain (
  IN UINTN   Argc,
  IN CHAR16  **Argv
  )
{
  EFI_STATUS                                      Status;
  EFI_ACPI_5_0_ROOT_SYSTEM_DESCRIPTION_POINTER   *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                     *Xsdt;
  EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE      *Bgrt;
  VOID                                           *LogoImage;
  UINTN                                          LogoSize;
  CHAR16                                         *OutputFileName;
  BOOLEAN                                        IsAcpi20;

  ShellPrintEx (-1, -1, L"===== ACPI XSDT / BGRT Logo Dump Utility =====\n\n");

  OutputFileName = (Argc > 1) ? Argv[1] : BGRT_LOGO_DEFAULT_FILE;

  Status = LocateRsdp (&Rsdp, &IsAcpi20);
  if (EFI_ERROR (Status)) {
    ShellPrintEx (-1, -1, L"ERROR: ACPI RSDP was not found: %r\n", Status);
    return SHELL_ABORTED;
  }

  DumpRsdp (Rsdp, IsAcpi20);

  if ((!IsAcpi20) || (Rsdp->Revision < 2) || (Rsdp->XsdtAddress == 0)) {
    ShellPrintEx (-1, -1, L"ERROR: XSDT is not available. This app requires ACPI 2.0+ XSDT.\n");
    return SHELL_ABORTED;
  }

  Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Rsdp->XsdtAddress;
  ShellPrintEx (-1, -1, L"XSDT Address: 0x%p\n\n", Xsdt);

  Status = WalkXsdt (Xsdt, &Bgrt);
  if (EFI_ERROR (Status)) {
    ShellPrintEx (-1, -1, L"ERROR: BGRT was not found in XSDT: %r\n", Status);
    return SHELL_NOT_FOUND;
  }

  ShellPrintEx (-1, -1, L"BGRT Address: 0x%p\n\n", Bgrt);
  DumpBgrt (Bgrt);

  if (Bgrt->ImageType != EFI_ACPI_5_0_BGRT_IMAGE_TYPE_BMP) {
    ShellPrintEx (-1, -1, L"ERROR: BGRT image type is not BMP.\n");
    return SHELL_UNSUPPORTED;
  }

  if (Bgrt->ImageAddress == 0) {
    ShellPrintEx (-1, -1, L"ERROR: BGRT image address is zero.\n");
    return SHELL_ABORTED;
  }

  LogoImage = (VOID *)(UINTN)Bgrt->ImageAddress;
  Status    = GetBgrtBmpSize (LogoImage, &LogoSize);
  if (EFI_ERROR (Status)) {
    ShellPrintEx (-1, -1, L"ERROR: BGRT image is not a valid BMP buffer: %r\n", Status);
    return SHELL_ABORTED;
  }

  ShellPrintEx (-1, -1, L"Writing BGRT logo to: %s\n", OutputFileName);
  Status = WriteLogoFile (OutputFileName, LogoImage, LogoSize);
  if (EFI_ERROR (Status)) {
    ShellPrintEx (-1, -1, L"ERROR: Failed to write BMP file: %r\n", Status);
    return SHELL_ABORTED;
  }

  ShellPrintEx (-1, -1, L"SUCCESS: BGRT logo exported to %s (%d bytes).\n",
    OutputFileName,
    LogoSize
    );
  ShellPrintEx (-1, -1, L"\n===== BgrtLogoDumpApp Done =====\n");
  return SHELL_SUCCESS;
}
