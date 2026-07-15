/** @file
  Header for SmbiosDumpApp - SMBIOS EPS Viewer and Custom Type Installer.

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SMBIOS_DUMP_APP_H_
#define SMBIOS_DUMP_APP_H_

#include <Uefi.h>
#include <Guid/SmBios.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

//
// Custom SMBIOS OEM Type (Type 128, first OEM-defined type).
//
#define CUSTOM_SMBIOS_TYPE_NUM  128

//
// Custom SMBIOS type structure — programmer-defined data type.
//
#pragma pack(1)
typedef struct {
  SMBIOS_STRUCTURE    Hdr;              // Type=128, Length=sizeof(this struct)
  UINT32              Magic;            // 'STDY'
  UINT8               MajorVer;         // 1
  UINT8               MinorVer;         // 0
  UINT16              FeatureFlags;     // bit-field
  UINT64              Timestamp;        // monotonic count at install time
  UINT32              DataValues[8];    // custom data array
} CUSTOM_SMBIOS_TYPE;
#pragma pack()

#define CUSTOM_MAGIC  SIGNATURE_32('S', 'T', 'D', 'Y')

#define CUSTOM_FLAG_BOOT_COMPLETE  BIT0
#define CUSTOM_FLAG_ACPI_ENABLED   BIT1
#define CUSTOM_FLAG_DEBUG          BIT2

//
// String indices for type-name lookup table.
//
typedef struct {
  UINT8    Type;
  CHAR16   *Name;
} SMBIOS_TYPE_NAME;

/**
  Display the SMBIOS Entry Point Structure fields.

  @param[in]  Eps32  32-bit entry point (may be NULL).
  @param[in]  Eps64  64-bit entry point (may be NULL).
**/
VOID
DumpEps (
  IN SMBIOS_TABLE_ENTRY_POINT      *Eps32,
  IN SMBIOS_TABLE_3_0_ENTRY_POINT  *Eps64
  );

/**
  Get a human-readable name for an SMBIOS type.

  @param[in]  Type  SMBIOS type number.

  @return  Static string describing the type.
**/
CHAR16 *
GetTypeName (
  IN UINT8  Type
  );

/**
  Get an SMBIOS string by its 1-based index from the current structure.

  @param[in]  Raw           Pointer to the start of the SMBIOS record.
  @param[in]  Length        Formatted area length.
  @param[in]  StringNumber  1-based string index.

  @return  Pointer to the string, or NULL if not found.
**/
CHAR8 *
GetSmbiosString (
  IN UINT8   *Raw,
  IN UINT8   Length,
  IN UINT16  StringNumber
  );

/**
  Print a CHAR8 string as CHAR16 via ShellPrintEx.
**/
VOID
PrintAscii (
  IN CHAR8  *String
  );

/**
  Decode and display SMBIOS Type 0 (BIOS Information).
**/
VOID
DumpType0 (
  IN SMBIOS_TABLE_TYPE0  *Type0
  );

/**
  Decode and display SMBIOS Type 1 (System Information).
**/
VOID
DumpType1 (
  IN SMBIOS_TABLE_TYPE1  *Type1
  );

/**
  Decode and display SMBIOS Type 4 (Processor Information).
**/
VOID
DumpType4 (
  IN SMBIOS_TABLE_TYPE4  *Type4
  );

/**
  Display the custom SMBIOS type (Type 128).
**/
VOID
DumpCustomType (
  IN CUSTOM_SMBIOS_TYPE  *Custom
  );

/**
  Build and install a custom SMBIOS record via EFI_SMBIOS_PROTOCOL.
**/
EFI_STATUS
InstallCustomSmbiosType (
  VOID
  );

/**
  Walk SMBIOS raw table entries and display them.
**/
VOID
WalkSmbiosTable (
  IN UINT8   *TableStart,
  IN UINTN   TableLength
  );

#endif
