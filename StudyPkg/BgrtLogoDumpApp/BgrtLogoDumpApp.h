/** @file
  Header for BgrtLogoDumpApp - ACPI XSDT and BGRT logo dump utility.

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BGRT_LOGO_DUMP_APP_H_
#define BGRT_LOGO_DUMP_APP_H_

#include <Uefi.h>
#include <Guid/Acpi.h>
#include <IndustryStandard/Acpi50.h>
#include <IndustryStandard/Bmp.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

//
// Keep a defensive upper bound when trusting the BMP size field from BGRT.
//
#define BGRT_LOGO_MAX_BMP_SIZE  (32 * 1024 * 1024)

//
// Default output file written to the same file system that loaded the app.
//
#define BGRT_LOGO_DEFAULT_FILE  L"bgrt_logo.bmp"

/**
  Convert a 32-bit ACPI signature to a printable ASCII buffer.

  @param[in]   Signature        ACPI table signature.
  @param[out]  SignatureString  Five-byte buffer that receives "XXXX\0".
**/
VOID
SignatureToString (
  IN  UINT32  Signature,
  OUT CHAR8   SignatureString[5]
  );

/**
  Checksum an ACPI structure.

  @param[in]  Buffer  Buffer to checksum.
  @param[in]  Length  Length in bytes.

  @retval TRUE   The 8-bit checksum is valid.
  @retval FALSE  The checksum is invalid.
**/
BOOLEAN
IsChecksumValid (
  IN VOID   *Buffer,
  IN UINTN  Length
  );

/**
  Locate the ACPI RSDP via EFI Configuration Table.

  @param[out]  Rsdp       RSDP pointer.
  @param[out]  IsAcpi20   TRUE if located through ACPI 2.0+ GUID.

  @retval EFI_SUCCESS    RSDP was found.
  @retval EFI_NOT_FOUND  RSDP was not found.
**/
EFI_STATUS
LocateRsdp (
  OUT EFI_ACPI_5_0_ROOT_SYSTEM_DESCRIPTION_POINTER  **Rsdp,
  OUT BOOLEAN                                       *IsAcpi20
  );

/**
  Print ACPI RSDP fields.

  @param[in]  Rsdp      RSDP pointer.
  @param[in]  IsAcpi20  TRUE if ACPI 2.0+ table GUID was used.
**/
VOID
DumpRsdp (
  IN EFI_ACPI_5_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp,
  IN BOOLEAN                                       IsAcpi20
  );

/**
  Print common ACPI table header fields.

  @param[in]  Header  ACPI table header.
**/
VOID
DumpAcpiHeader (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Header
  );

/**
  Walk the XSDT and locate BGRT.

  @param[in]   Xsdt  XSDT header.
  @param[out]  Bgrt  BGRT pointer if found.

  @retval EFI_SUCCESS    XSDT was parsed.
  @retval EFI_NOT_FOUND  BGRT was not found.
**/
EFI_STATUS
WalkXsdt (
  IN  EFI_ACPI_DESCRIPTION_HEADER             *Xsdt,
  OUT EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE **Bgrt
  );

/**
  Print BGRT fields.

  @param[in]  Bgrt  BGRT pointer.
**/
VOID
DumpBgrt (
  IN EFI_ACPI_5_0_BOOT_GRAPHICS_RESOURCE_TABLE  *Bgrt
  );

/**
  Validate a BGRT BMP image and return its file size.

  @param[in]   Image     BMP image address.
  @param[out]  FileSize  BMP file size.

  @retval EFI_SUCCESS            BMP header looks valid.
  @retval EFI_INVALID_PARAMETER  Image or FileSize is NULL.
  @retval EFI_UNSUPPORTED        Image does not look like a BMP file.
  @retval EFI_BAD_BUFFER_SIZE    BMP size is suspicious.
**/
EFI_STATUS
GetBgrtBmpSize (
  IN  VOID   *Image,
  OUT UINTN  *FileSize
  );

/**
  Write the BGRT BMP image to the same file system that loaded the app.

  @param[in]  FileName  Destination file name relative to the file system root.
  @param[in]  Buffer    BMP image buffer.
  @param[in]  Size      BMP image size in bytes.

  @retval EFI_SUCCESS  File was written.
  @return              Other errors are returned by UEFI protocols.
**/
EFI_STATUS
WriteLogoFile (
  IN CHAR16  *FileName,
  IN VOID    *Buffer,
  IN UINTN   Size
  );

#endif
