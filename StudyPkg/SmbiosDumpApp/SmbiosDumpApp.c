/** @file
  SmbiosDumpApp - SMBIOS EPS Viewer and Custom Type Installer.

  Demonstrates two capabilities:
    1. Access and display the SMBIOS Entry Point Structure (EPS)
       and walk the SMBIOS structure table.
    2. Define a custom OEM SMBIOS type (Type 128) with programmer-
       defined data fields, install via EFI_SMBIOS_PROTOCOL, and
       display its contents.

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "SmbiosDumpApp.h"

SMBIOS_TYPE_NAME  mTypeNameTable[] = {
  { 0,   L"BIOS Information"              },
  { 1,   L"System Information"            },
  { 2,   L"Baseboard Information"         },
  { 3,   L"System Enclosure"              },
  { 4,   L"Processor Information"         },
  { 5,   L"Memory Controller"             },
  { 16,  L"Physical Memory Array"         },
  { 17,  L"Memory Device"                 },
  { 19,  L"Memory Array Mapped Address"   },
  { 20,  L"Memory Device Mapped Address"  },
  { 32,  L"System Boot Information"       },
  { 38,  L"IPMI Device Information"       },
  { 39,  L"System Power Supply"           },
  { 41,  L"Onboard Devices Extended"      },
  { 42,  L"Management Controller Host"    },
  { 43,  L"TPM Device"                    },
  { 126, L"Inactive"                      },
  { 127, L"End of Table"                  },
  { 128, L"OEM-Defined (Custom)"          },
  { 0,   NULL                             }
};

CHAR16 *
GetTypeName (
  IN UINT8  Type
  )
{
  UINTN  Index;

  for (Index = 0; mTypeNameTable[Index].Name != NULL; Index++) {
    if (mTypeNameTable[Index].Type == Type) {
      return mTypeNameTable[Index].Name;
    }
  }

  if ((Type >= 128) && (Type <= 255)) {
    return L"OEM-Defined";
  }

  return L"Unknown";
}

CHAR8 *
GetSmbiosString (
  IN UINT8   *Raw,
  IN UINT8   Length,
  IN UINT16  StringNumber
  )
{
  CHAR8   *String;
  UINT16  Index;

  if (StringNumber == 0) {
    return NULL;
  }

  String = (CHAR8 *)(Raw + Length);

  for (Index = 1; Index < StringNumber; Index++) {
    if (*String == 0) {
      return NULL;
    }

    while (*String != 0) {
      String++;
    }

    String++;
  }

  if (*String == 0) {
    return NULL;
  }

  return String;
}

VOID
PrintAscii (
  IN CHAR8  *String
  )
{
  if (String == NULL) {
    ShellPrintEx (-1, -1, L"(none)");
    return;
  }

  ShellPrintEx (-1, -1, L"%a", String);
}

VOID
DumpEps (
  IN SMBIOS_TABLE_ENTRY_POINT      *Eps32,
  IN SMBIOS_TABLE_3_0_ENTRY_POINT  *Eps64
  )
{
  if (Eps64 != NULL) {
    ShellPrintEx (-1, -1, L"=== SMBIOS 3.0 Entry Point Structure ===\n");
    ShellPrintEx (-1, -1, L"  Anchor       : _SM3_\n");
    ShellPrintEx (-1, -1, L"  Checksum     : 0x%02x\n", (UINTN)Eps64->EntryPointStructureChecksum);
    ShellPrintEx (-1, -1, L"  Length       : %d bytes\n", (UINTN)Eps64->EntryPointLength);
    ShellPrintEx (-1, -1, L"  Version      : %d.%d\n",
      (UINTN)Eps64->MajorVersion, (UINTN)Eps64->MinorVersion);
    ShellPrintEx (-1, -1, L"  DocRev       : %d\n", (UINTN)Eps64->DocRev);
    ShellPrintEx (-1, -1, L"  MaxTableSize : 0x%08x (%d)\n",
      (UINTN)Eps64->TableMaximumSize, (UINTN)Eps64->TableMaximumSize);
    ShellPrintEx (-1, -1, L"  TableAddress : 0x%016lx\n", (UINTN)Eps64->TableAddress);
  }

  if (Eps32 != NULL) {
    ShellPrintEx (-1, -1, L"\n=== SMBIOS 2.x Entry Point Structure ===\n");
    ShellPrintEx (-1, -1, L"  Anchor           : _SM_\n");
    ShellPrintEx (-1, -1, L"  Checksum         : 0x%02x\n", (UINTN)Eps32->EntryPointStructureChecksum);
    ShellPrintEx (-1, -1, L"  Length           : %d bytes\n", (UINTN)Eps32->EntryPointLength);
    ShellPrintEx (-1, -1, L"  Version          : %d.%d\n",
      (UINTN)Eps32->MajorVersion, (UINTN)Eps32->MinorVersion);
    ShellPrintEx (-1, -1, L"  MaxStructSize    : %d\n", (UINTN)Eps32->MaxStructureSize);
    ShellPrintEx (-1, -1, L"  EntryPointRev    : %d\n", (UINTN)Eps32->EntryPointRevision);
    ShellPrintEx (-1, -1, L"  IntermediateAnch : _DMI_\n");
    ShellPrintEx (-1, -1, L"  IntmChecksum     : 0x%02x\n", (UINTN)Eps32->IntermediateChecksum);
    ShellPrintEx (-1, -1, L"  TableLength      : %d bytes\n", (UINTN)Eps32->TableLength);
    ShellPrintEx (-1, -1, L"  TableAddress     : 0x%08x\n", (UINTN)Eps32->TableAddress);
    ShellPrintEx (-1, -1, L"  NumStructures    : %d\n", (UINTN)Eps32->NumberOfSmbiosStructures);
    ShellPrintEx (-1, -1, L"  BcdRevision      : 0x%02x\n", (UINTN)Eps32->SmbiosBcdRevision);
  }

  ShellPrintEx (-1, -1, L"\n");
}

VOID
DumpType0 (
  IN SMBIOS_TABLE_TYPE0  *Type0
  )
{
  ShellPrintEx (-1, -1, L"  >> Type 0: BIOS Information\n");
  ShellPrintEx (-1, -1, L"     Vendor           : ");
  PrintAscii (GetSmbiosString ((UINT8 *)Type0, Type0->Hdr.Length, Type0->Vendor));
  ShellPrintEx (-1, -1, L"\n");
  ShellPrintEx (-1, -1, L"     BIOS Version     : ");
  PrintAscii (GetSmbiosString ((UINT8 *)Type0, Type0->Hdr.Length, Type0->BiosVersion));
  ShellPrintEx (-1, -1, L"\n");
  ShellPrintEx (-1, -1, L"     Release Date     : ");
  PrintAscii (GetSmbiosString ((UINT8 *)Type0, Type0->Hdr.Length, Type0->BiosReleaseDate));
  ShellPrintEx (-1, -1, L"\n");
  ShellPrintEx (-1, -1, L"     BIOS Segment     : 0x%04x\n", (UINTN)Type0->BiosSegment);
  ShellPrintEx (-1, -1, L"     BIOS Size        : %d KB\n",
    (UINTN)((Type0->BiosSize + 1) * 64));
  ShellPrintEx (-1, -1, L"     System BIOS Rev  : %d.%d\n",
    (UINTN)Type0->SystemBiosMajorRelease,
    (UINTN)Type0->SystemBiosMinorRelease);
  ShellPrintEx (-1, -1, L"     EC FW Rev        : %d.%d\n",
    (UINTN)Type0->EmbeddedControllerFirmwareMajorRelease,
    (UINTN)Type0->EmbeddedControllerFirmwareMinorRelease);
}

VOID
DumpType1 (
  IN SMBIOS_TABLE_TYPE1  *Type1
  )
{
  ShellPrintEx (-1, -1, L"  >> Type 1: System Information\n");
  ShellPrintEx (-1, -1, L"     Manufacturer  : ");
  PrintAscii (GetSmbiosString ((UINT8 *)Type1, Type1->Hdr.Length, Type1->Manufacturer));
  ShellPrintEx (-1, -1, L"\n");
  ShellPrintEx (-1, -1, L"     Product Name  : ");
  PrintAscii (GetSmbiosString ((UINT8 *)Type1, Type1->Hdr.Length, Type1->ProductName));
  ShellPrintEx (-1, -1, L"\n");
  ShellPrintEx (-1, -1, L"     Version       : ");
  PrintAscii (GetSmbiosString ((UINT8 *)Type1, Type1->Hdr.Length, Type1->Version));
  ShellPrintEx (-1, -1, L"\n");
  ShellPrintEx (-1, -1, L"     Serial Number : ");
  PrintAscii (GetSmbiosString ((UINT8 *)Type1, Type1->Hdr.Length, Type1->SerialNumber));
  ShellPrintEx (-1, -1, L"\n");
  ShellPrintEx (-1, -1, L"     UUID          : %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
    (UINTN)Type1->Uuid.Data1,
    (UINTN)Type1->Uuid.Data2,
    (UINTN)Type1->Uuid.Data3,
    (UINTN)Type1->Uuid.Data4[0],
    (UINTN)Type1->Uuid.Data4[1],
    (UINTN)Type1->Uuid.Data4[2],
    (UINTN)Type1->Uuid.Data4[3],
    (UINTN)Type1->Uuid.Data4[4],
    (UINTN)Type1->Uuid.Data4[5],
    (UINTN)Type1->Uuid.Data4[6],
    (UINTN)Type1->Uuid.Data4[7]
    );
  ShellPrintEx (-1, -1, L"     Wake-Up Type  : 0x%02x\n", (UINTN)Type1->WakeUpType);
}

VOID
DumpType4 (
  IN SMBIOS_TABLE_TYPE4  *Type4
  )
{
  UINTN  CoreCount;
  UINTN  ThreadCount;

  if (Type4->Hdr.Length >= 0x2C) {
    CoreCount   = (Type4->CoreCount2 != 0) ? Type4->CoreCount2 : Type4->CoreCount;
    ThreadCount = (Type4->ThreadCount2 != 0) ? Type4->ThreadCount2 : Type4->ThreadCount;
  } else {
    CoreCount   = Type4->CoreCount;
    ThreadCount = Type4->ThreadCount;
  }

  ShellPrintEx (-1, -1, L"  >> Type 4: Processor Information\n");
  ShellPrintEx (-1, -1, L"     Socket          : ");
  PrintAscii (GetSmbiosString ((UINT8 *)Type4, Type4->Hdr.Length, Type4->Socket));
  ShellPrintEx (-1, -1, L"\n");
  ShellPrintEx (-1, -1, L"     Manufacturer    : ");
  PrintAscii (GetSmbiosString ((UINT8 *)Type4, Type4->Hdr.Length, Type4->ProcessorManufacturer));
  ShellPrintEx (-1, -1, L"\n");
  ShellPrintEx (-1, -1, L"     Processor Family: 0x%04x\n", (UINTN)Type4->ProcessorFamily);
  ShellPrintEx (-1, -1, L"     Max Speed       : %d MHz\n", (UINTN)Type4->MaxSpeed);
  ShellPrintEx (-1, -1, L"     Current Speed   : %d MHz\n", (UINTN)Type4->CurrentSpeed);
  ShellPrintEx (-1, -1, L"     Core Count      : %d\n", (UINTN)CoreCount);
  ShellPrintEx (-1, -1, L"     Thread Count    : %d\n", (UINTN)ThreadCount);
  ShellPrintEx (-1, -1, L"     External Clock  : %d MHz\n", (UINTN)Type4->ExternalClock);

  if (Type4->ProcessorVersion != 0) {
    ShellPrintEx (-1, -1, L"     Version String  : ");
    PrintAscii (GetSmbiosString ((UINT8 *)Type4, Type4->Hdr.Length, Type4->ProcessorVersion));
    ShellPrintEx (-1, -1, L"\n");
  }
}

VOID
DumpCustomType (
  IN CUSTOM_SMBIOS_TYPE  *Custom
  )
{
  UINTN  Index;

  ShellPrintEx (-1, -1, L"  >> Custom OEM Type (128): Programmer-Defined\n");
  ShellPrintEx (-1, -1, L"     Magic           : 0x%08x", (UINTN)Custom->Magic);

  if (Custom->Magic == CUSTOM_MAGIC) {
    ShellPrintEx (-1, -1, L" (\"STDY\")");
  }

  ShellPrintEx (-1, -1, L"\n");
  ShellPrintEx (-1, -1, L"     Version         : %d.%d\n",
    (UINTN)Custom->MajorVer, (UINTN)Custom->MinorVer);
  ShellPrintEx (-1, -1, L"     Feature Flags   : 0x%04x", (UINTN)Custom->FeatureFlags);

  if ((Custom->FeatureFlags & CUSTOM_FLAG_BOOT_COMPLETE) != 0) {
    ShellPrintEx (-1, -1, L" [BOOT_COMPLETE]");
  }

  if ((Custom->FeatureFlags & CUSTOM_FLAG_ACPI_ENABLED) != 0) {
    ShellPrintEx (-1, -1, L" [ACPI_ENABLED]");
  }

  if ((Custom->FeatureFlags & CUSTOM_FLAG_DEBUG) != 0) {
    ShellPrintEx (-1, -1, L" [DEBUG]");
  }

  ShellPrintEx (-1, -1, L"\n");
  ShellPrintEx (-1, -1, L"     Timestamp       : %lld (monotonic count)\n",
    (UINTN)Custom->Timestamp);
  ShellPrintEx (-1, -1, L"     Data Values     :");

  for (Index = 0; Index < 8; Index++) {
    if ((Index % 4) == 0) {
      ShellPrintEx (-1, -1, L"\n       ");
    }

    ShellPrintEx (-1, -1, L" [%d] 0x%08x  ", (UINTN)Index, (UINTN)Custom->DataValues[Index]);
  }

  ShellPrintEx (-1, -1, L"\n");
}

VOID
WalkSmbiosTable (
  IN UINT8   *TableStart,
  IN UINTN   TableLength
  )
{
  SMBIOS_STRUCTURE_POINTER  Struct;
  UINT8                     *End;
  UINTN                     Index;
  CHAR8                     *Str;

  Struct.Raw = TableStart;
  End        = TableStart + TableLength;
  Index      = 0;

  while ((Struct.Raw + sizeof (SMBIOS_STRUCTURE)) <= End) {
    if (Struct.Hdr->Type == SMBIOS_TYPE_END_OF_TABLE) {
      ShellPrintEx (-1, -1, L"  --- [%d] Type 127: End of Table ---\n", (UINTN)Index);
      break;
    }

    if (Struct.Hdr->Type == SMBIOS_TYPE_INACTIVE) {
      ShellPrintEx (-1, -1, L"  --- [%d] Type 126: Inactive (Handle=0x%04x) ---\n",
        (UINTN)Index, (UINTN)Struct.Hdr->Handle);
      goto Skip;
    }

    if (Struct.Hdr->Length < 4) {
      ShellPrintEx (-1, -1, L"  ** Corrupt record at offset %d (Length=%d)\n",
        (UINTN)(Struct.Raw - TableStart), (UINTN)Struct.Hdr->Length);
      break;
    }

    if ((Struct.Raw + Struct.Hdr->Length) > End) {
      break;
    }

    ShellPrintEx (-1, -1, L"  --- [%d] Type %3d (0x%02x): %s ---\n",
      (UINTN)Index,
      (UINTN)Struct.Hdr->Type,
      (UINTN)Struct.Hdr->Type,
      GetTypeName (Struct.Hdr->Type)
      );
    ShellPrintEx (-1, -1, L"       Length=%d  Handle=0x%04x\n",
      (UINTN)Struct.Hdr->Length, (UINTN)Struct.Hdr->Handle);

    switch (Struct.Hdr->Type) {
    case 0:
      DumpType0 (Struct.Type0);
      break;
    case 1:
      DumpType1 (Struct.Type1);
      break;
    case 4:
      DumpType4 (Struct.Type4);
      break;
    case 128:
      DumpCustomType ((CUSTOM_SMBIOS_TYPE *)Struct.Raw);
      break;
    default:
      //
      // Show first string (if any) as a quick preview.
      //
      Str = GetSmbiosString (Struct.Raw, Struct.Hdr->Length, 1);
      if (Str != NULL) {
        ShellPrintEx (-1, -1, L"       String 1     : ");
        PrintAscii (Str);
        ShellPrintEx (-1, -1, L"\n");
      }

      break;
    }

    ShellPrintEx (-1, -1, L"\n");

Skip:
    //
    // Advance past formatted area + string section.
    //
    {
      UINT8  *RawPtr;
      RawPtr = Struct.Raw + Struct.Hdr->Length;
      while ((RawPtr < End) && (*RawPtr != 0 || *(RawPtr + 1) != 0)) {
        RawPtr++;
      }

      RawPtr += 2;
      Struct.Raw = RawPtr;
      Index++;
    }
  }
}

EFI_STATUS
InstallCustomSmbiosType (
  VOID
  )
{
  EFI_SMBIOS_PROTOCOL  *SmbiosProtocol;
  CUSTOM_SMBIOS_TYPE   *Custom;
  EFI_SMBIOS_HANDLE    SmbiosHandle;
  EFI_STATUS           Status;
  UINTN                Index;

  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&SmbiosProtocol
                  );

  if (EFI_ERROR (Status)) {
    ShellPrintEx (-1, -1, L"  ** EFI_SMBIOS_PROTOCOL not available (Status=%r)\n", Status);
    return Status;
  }

  Custom = AllocateZeroPool (sizeof (CUSTOM_SMBIOS_TYPE) + 2);
  if (Custom == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Fill header.
  //
  Custom->Hdr.Type   = CUSTOM_SMBIOS_TYPE_NUM;
  Custom->Hdr.Length = sizeof (CUSTOM_SMBIOS_TYPE);
  Custom->Hdr.Handle = SMBIOS_HANDLE_PI_RESERVED;

  //
  // Fill custom data.
  //
  Custom->Magic    = CUSTOM_MAGIC;
  Custom->MajorVer = 1;
  Custom->MinorVer = 0;
  Custom->FeatureFlags = CUSTOM_FLAG_BOOT_COMPLETE | CUSTOM_FLAG_ACPI_ENABLED;

  {
    UINT64  Ticks;
    Ticks = 0;
    gBS->Stall (1);
    gBS->GetNextMonotonicCount (&Ticks);
    Custom->Timestamp = Ticks;
  }

  for (Index = 0; Index < 8; Index++) {
    Custom->DataValues[Index] = (UINT32)(0x53540000 + (UINT32)(Index * 0x2222));
  }

  //
  // Append double-null terminator (no strings).
  //
  {
    UINT8  *End;
    End  = (UINT8 *)Custom + sizeof (CUSTOM_SMBIOS_TYPE);
    End[0] = 0;
    End[1] = 0;
  }

  //
  // Install the record.  SMBIOS_HANDLE_PI_RESERVED (0xFFFE) means
  // auto-assign a handle.
  //
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;

  Status = SmbiosProtocol->Add (
                             SmbiosProtocol,
                             NULL,
                             &SmbiosHandle,
                             (EFI_SMBIOS_TABLE_HEADER *)Custom
                             );

  if (!EFI_ERROR (Status)) {
    ShellPrintEx (-1, -1, L"  ** Custom Type 128 installed (Handle=0x%04x).\n",
      (UINTN)SmbiosHandle);
  } else {
    ShellPrintEx (-1, -1, L"  ** EFI_SMBIOS_PROTOCOL.Add() failed: %r\n", Status);
  }

  FreePool (Custom);
  return Status;
}

INTN
EFIAPI
ShellAppMain (
  IN UINTN   Argc,
  IN CHAR16  **Argv
  )
{
  SMBIOS_TABLE_ENTRY_POINT      *Eps32;
  SMBIOS_TABLE_3_0_ENTRY_POINT  *Eps64;
  UINT8                         *TableStart;
  UINTN                         TableLength;
  EFI_SMBIOS_PROTOCOL           *SmbiosProtocol;
  EFI_SMBIOS_HANDLE             SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER       *Record;
  EFI_STATUS                    Status;

  ShellPrintEx (-1, -1, L"===== SMBIOS Entry Point Structure Viewer =====\n\n");

  //
  // 1. Locate SMBIOS Entry Point(s) from the EFI Configuration Table.
  //
  Eps32 = NULL;
  Eps64 = NULL;

  /**
   * 获取系统配置表中的SMBIOS表
   */
  Status = EfiGetSystemConfigurationTable (
             &gEfiSmbios3TableGuid,
             (VOID **)&Eps64
             );

  if (EFI_ERROR (Status) || (Eps64 == NULL)) {
    Eps64 = NULL;
    Status = EfiGetSystemConfigurationTable (
               &gEfiSmbiosTableGuid,
               (VOID **)&Eps32
               );
  } else {
    //
    // Also try to get the 32-bit EPS for backward compatibility display.
    //
    EfiGetSystemConfigurationTable (
      &gEfiSmbiosTableGuid,
      (VOID **)&Eps32
      );
  }

  if ((Eps32 == NULL) && (Eps64 == NULL)) {
    ShellPrintEx (-1, -1, L"ERROR: No SMBIOS Entry Point Structure found.\n");
    return SHELL_ABORTED;
  }

  /**
   * 打印信息
   */
  DumpEps (Eps32, Eps64);

  //
  // 2. Walk the raw SMBIOS table.
  //
  /**
   * 获取SMBIOS表的起始地址和长度
   */
  if (Eps64 != NULL) {
    TableStart  = (UINT8 *)(UINTN)Eps64->TableAddress;
    TableLength = (UINTN)Eps64->TableMaximumSize;
  } else {
    TableStart  = (UINT8 *)(UINTN)Eps32->TableAddress;
    TableLength = (UINTN)Eps32->TableLength;
  }

  if ((TableStart == NULL) || (TableLength == 0)) {
    ShellPrintEx (-1, -1, L"ERROR: SMBIOS table address or length is 0.\n");
    return SHELL_ABORTED;
  }

  ShellPrintEx (-1, -1, L"=== SMBIOS Structure Table (raw walk) ===\n");
  ShellPrintEx (-1, -1, L"  Table at 0x%p, Length=%d bytes\n\n",
    (VOID *)TableStart, (UINTN)TableLength);

  WalkSmbiosTable (TableStart, TableLength);

  //
  // 3. Install custom SMBIOS type via the protocol.
  //
  ShellPrintEx (-1, -1, L"\n=== Installing Custom SMBIOS Type 128 ===\n");
  /**
   * 安装自定义SMBIOS类型
   */
  InstallCustomSmbiosType ();

  //
  // 4. Use EFI_SMBIOS_PROTOCOL.GetNext to locate the custom type.
  //
  ShellPrintEx (-1, -1, L"\n=== Custom Type Verification via Protocol ===\n");

  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&SmbiosProtocol
                  );

  if (!EFI_ERROR (Status)) {
    SMBIOS_TYPE  FilterType;
    FilterType  = CUSTOM_SMBIOS_TYPE_NUM;
    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;

    /**
     * 获取自定义SMBIOS类型的记录
     */
    Status = SmbiosProtocol->GetNext (
                               SmbiosProtocol,
                               &SmbiosHandle,
                               &FilterType,
                               &Record,
                               NULL
                               );

    if (!EFI_ERROR (Status)) {
      /**
       * 打印自定义SMBIOS类型记录
       */
      DumpCustomType ((CUSTOM_SMBIOS_TYPE *)Record);
      ShellPrintEx (-1, -1, L"       (verified via EFI_SMBIOS_PROTOCOL, Handle=0x%04x)\n",
        (UINTN)SmbiosHandle);
    } else {
      ShellPrintEx (-1, -1, L"  ** GetNext (Type=128) failed: %r\n", Status);
    }
  } else {
    ShellPrintEx (-1, -1, L"  ** EFI_SMBIOS_PROTOCOL not available for verification.\n");
  }

  ShellPrintEx (-1, -1, L"\n===== SmbiosDumpApp Done =====\n");
  return SHELL_SUCCESS;
}
