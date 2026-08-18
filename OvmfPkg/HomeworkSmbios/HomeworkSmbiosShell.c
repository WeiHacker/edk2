/** @file
  Display the SMBIOS entry point, BIOS Information, and homework OEM type.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/SmBios.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>

#include "HomeworkSmbios.h"

typedef struct {
  UINT64          Mask;
  CONST CHAR16    *Description;
} SMBIOS_BIT_DESCRIPTION;


// 根据[SmbiosViewStrings.uni定义而来
STATIC CONST SMBIOS_BIT_DESCRIPTION  mBiosCharacteristics[] = {
  { BIT0,  L"Reserved bit" },
  { BIT1,  L"Reserved bit" },
  { BIT2,  L"Unknown bit" },
  { BIT3,  L"BIOS Characteristics are not supported" },
  { BIT4,  L"ISA is supported" },
  { BIT5,  L"MCA is supported" },
  { BIT6,  L"EISA is supported" },
  { BIT7,  L"PCI is supported" },
  { BIT8,  L"PC card (PCMCIA) is supported" },
  { BIT9,  L"Plug and play is supported" },
  { BIT10, L"APM is supported" },
  { BIT11, L"BIOS is upgradeable (Flash)" },
  { BIT12, L"BIOS shadowing is allowed" },
  { BIT13, L"VL-VESA is supported" },
  { BIT14, L"ESCD support is available" },
  { BIT15, L"Boot from CD is supported" },
  { BIT16, L"Selectable Boot is supported" },
  { BIT17, L"BIOS ROM is socketed" },
  { BIT18, L"Boot from PC card (PCMCIA) is supported" },
  { BIT19, L"EDD specification is supported" },
  { BIT20, L"Int 13h - Japanese floppy for NEC 9800 is supported" },
  { BIT21, L"Int 13h - Japanese floppy for Toshiba is supported" },
  { BIT22, L"Int 13h - 5.25 inch/360 KB floppy services are supported" },
  { BIT23, L"Int 13h - 5.25 inch/1.2 MB floppy services are supported" },
  { BIT24, L"Int 13h - 3.5 inch/720 KB floppy services are supported" },
  { BIT25, L"Int 13h - 3.5 inch/2.88 MB floppy services are supported" },
  { BIT26, L"Int 5h print screen service is supported" },
  { BIT27, L"Int 9h 8042 keyboard service is supported" },
  { BIT28, L"Int 14h serial service is supported" },
  { BIT29, L"Int 17h printer service is supported" },
  { BIT30, L"Int 10h CGA/Mono video service is supported" },
  { BIT31, L"NEC PC-98" }
};

STATIC CONST SMBIOS_BIT_DESCRIPTION  mBiosCharacteristicsExtension1[] = {
  { BIT0, L"ACPI is supported" },
  { BIT1, L"USB Legacy is supported" },
  { BIT2, L"AGP is supported" },
  { BIT3, L"I2O boot is supported" },
  { BIT4, L"LS-120 SuperDisk boot is supported" },
  { BIT5, L"ATAPI ZIP drive boot is supported" },
  { BIT6, L"1394 boot is supported" },
  { BIT7, L"Smart battery is supported" }
};

STATIC CONST SMBIOS_BIT_DESCRIPTION  mBiosCharacteristicsExtension2[] = {
  { BIT0, L"BIOS Boot Specification is supported" },
  { BIT1, L"Function key-initiated network service boot is supported" },
  { BIT2, L"Enable targeted content distribution" },
  { BIT3, L"UEFI Specification is supported" },
  { BIT4, L"SMBIOS table describes a virtual machine" },
  { BIT5, L"Manufacturing mode is supported" },
  { BIT6, L"Manufacturing mode is enabled" }
};

/**
  Display descriptions for all set bits in a value.

  @param[in] Value         Value containing the feature bits.
  @param[in] Descriptions  Bit descriptions to display.
  @param[in] Count         Number of entries in Descriptions.
**/
STATIC
VOID
DisplaySetBits (
  IN UINT64                         Value,
  IN CONST SMBIOS_BIT_DESCRIPTION  *Descriptions,
  IN UINTN                          Count
  )
{
  UINTN  Index;

  for (Index = 0; Index < Count; Index++) {
    if ((Value & Descriptions[Index].Mask) != 0) {
      Print (L"%s\n", Descriptions[Index].Description);
    }
  }
}

/**
  Display the BIOS Characteristics field.

  @param[in] Type0  Pointer to the Type 0 structure.
**/
STATIC
VOID
DisplayBiosCharacteristics (
  IN SMBIOS_TABLE_TYPE0  *Type0
  )
{
  UINT64  Characteristics;
  UINT8   *Data;
  UINTN   Index;

  Characteristics = 0;
  Data            = (UINT8 *)&Type0->BiosCharacteristics;
  for (Index = 0; Index < sizeof (Type0->BiosCharacteristics); Index++) {
    Characteristics |= (UINT64)Data[Index] << (Index * 8);
  }

  Print (L"BIOS Characteristics:\n");
  DisplaySetBits (
    Characteristics,
    mBiosCharacteristics,
    ARRAY_SIZE (mBiosCharacteristics)
    );
  Print (L"Bits 32:47 are reserved for BIOS vendor\n");
  Print (L"Bits 48:63 are reserved for system vendor\n");
}

/**
  Display BIOS Characteristics Extension Byte 1.

  @param[in] Value  Extension byte value.
**/
STATIC
VOID
DisplayBiosCharacteristicsExtension1 (
  IN UINT8  Value
  )
{
  Print (L"BIOS Characteristics Extension Byte1:\n");
  DisplaySetBits (
    Value,
    mBiosCharacteristicsExtension1,
    ARRAY_SIZE (mBiosCharacteristicsExtension1)
    );
}

/**
  Display BIOS Characteristics Extension Byte 2.

  @param[in] Value  Extension byte value.
**/
STATIC
VOID
DisplayBiosCharacteristicsExtension2 (
  IN UINT8  Value
  )
{
  Print (L"BIOS Characteristics Extension Byte2:\n");
  DisplaySetBits (
    Value,
    mBiosCharacteristicsExtension2,
    ARRAY_SIZE (mBiosCharacteristicsExtension2)
    );
  Print (L"Bit 7 is reserved for future assignment\n");
}

/**
  Return the next SMBIOS structure after the double-null terminator.

  @param[in] Structure  Current SMBIOS structure.
  @param[in] TableEnd   Address immediately after the table buffer.

  @return Address of the next structure, or NULL if the table is invalid.
**/
/**
┌────────────────────────────┐
│ SMBIOS Header              │
│ Type / Length / Handle     │
├────────────────────────────┤
│ Formatted Area             │
├────────────────────────────┤
│ String 1 + 00              │
│ String 2 + 00              │
├────────────────────────────┤
│ 00                         │ ← 与上一个字符串的 00 组成双 NULL
├────────────────────────────┤
│ 下一个 SMBIOS Structure    │
└────────────────────────────┘
 */
STATIC
SMBIOS_STRUCTURE *
GetNextStructure (
  IN SMBIOS_STRUCTURE  *Structure,
  IN UINT8             *TableEnd
  )
{
  UINT8  *Current;

  // SMBIOS Structure 的长度是由 Length 字段指定的，Length 字段表示的是整个结构体的长度，包括头字段和格式化区域的长度，但是不包括字符串集的长度。字符串集是紧跟在格式化区域之后的，它由一个或多个以 NULL 结尾的字符串组成，最后以两个连续的 NULL 字节表示字符串集的结束。因此，要找到下一个 SMBIOS Structure，需要先跳过当前结构体的格式化区域，然后再跳过字符串集，直到遇到两个连续的 NULL 字节为止。
  Current = (UINT8 *)Structure + Structure->Length;
  while ((Current + 1) < TableEnd) {
    if ((Current[0] == 0) && (Current[1] == 0)) {
      return (SMBIOS_STRUCTURE *)(Current + 2);
    }

    Current++;
  }

  return NULL;
}

/**
  Display an SMBIOS 2.x Entry Point Structure.

  @param[in] Eps  Pointer to the SMBIOS 2.x entry point.
**/
STATIC
VOID
DisplaySmbios2EntryPoint (
  IN SMBIOS_TABLE_ENTRY_POINT  *Eps
  )
{
  Print (L"\nSMBIOS Entry Point Structure:\n");
  Print (
    L"Anchor String:         %c%c%c%c\n",
    (UINTN)Eps->AnchorString[0],
    (UINTN)Eps->AnchorString[1],
    (UINTN)Eps->AnchorString[2],
    (UINTN)Eps->AnchorString[3]
    );
  Print (L"EPS Checksum:         0x%02x\n", (UINTN)Eps->EntryPointStructureChecksum);
  Print (L"Entry Point Len:      %u\n", (UINTN)Eps->EntryPointLength);
  Print (L"Version:              %u.%u\n", (UINTN)Eps->MajorVersion, (UINTN)Eps->MinorVersion);
  Print (L"Number of Structures: %u\n", (UINTN)Eps->NumberOfSmbiosStructures);
  Print (L"Max Struct size:      %u\n", (UINTN)Eps->MaxStructureSize);
  Print (L"Table Address:        0x%08x\n", (UINTN)Eps->TableAddress);
  Print (L"Table Length:         %u\n", (UINTN)Eps->TableLength);
  Print (L"Entry Point revision: 0x%x\n", (UINTN)Eps->EntryPointRevision);
  Print (L"SMBIOS BCD Revision:  0x%02x\n", (UINTN)Eps->SmbiosBcdRevision);
  Print (
    L"Inter Anchor:         %c%c%c%c%c\n",
    (UINTN)Eps->IntermediateAnchorString[0],
    (UINTN)Eps->IntermediateAnchorString[1],
    (UINTN)Eps->IntermediateAnchorString[2],
    (UINTN)Eps->IntermediateAnchorString[3],
    (UINTN)Eps->IntermediateAnchorString[4]
    );
  Print (L"Inter Checksum:       0x%02x\n", (UINTN)Eps->IntermediateChecksum);
  Print (L"Formatted Area:\n");
  Print (
    L"  00000000: %02x %02x %02x %02x %02x  *.....*\n",
    (UINTN)Eps->FormattedArea[0],
    (UINTN)Eps->FormattedArea[1],
    (UINTN)Eps->FormattedArea[2],
    (UINTN)Eps->FormattedArea[3],
    (UINTN)Eps->FormattedArea[4]
    );
}

/**
  Display an SMBIOS 3.x Entry Point Structure.

  @param[in] Eps  Pointer to the SMBIOS 3.x entry point.
**/
STATIC
VOID
DisplaySmbios3EntryPoint (
  IN SMBIOS_TABLE_3_0_ENTRY_POINT  *Eps
  )
{
  Print (L"\nSMBIOS 3.0 (64-bit) Entry Point Structure:\n");
  Print (
    L"Anchor String:         %c%c%c%c%c\n",
    (UINTN)Eps->AnchorString[0],
    (UINTN)Eps->AnchorString[1],
    (UINTN)Eps->AnchorString[2],
    (UINTN)Eps->AnchorString[3],
    (UINTN)Eps->AnchorString[4]
    );
  Print (L"EPS Checksum:         0x%02x\n", (UINTN)Eps->EntryPointStructureChecksum);
  Print (L"Entry Point Len:      %u\n", (UINTN)Eps->EntryPointLength);
  Print (L"Version:              %u.%u\n", (UINTN)Eps->MajorVersion, (UINTN)Eps->MinorVersion);
  Print (L"SMBIOS Docrev:        0x%x\n", (UINTN)Eps->DocRev);
  Print (L"Entry Point revision: 0x%x\n", (UINTN)Eps->EntryPointRevision);
  Print (L"Table Max Size:       %u\n", (UINTN)Eps->TableMaximumSize);
  Print (L"Table Address:        0x%lx\n", Eps->TableAddress);
}

/**
  Display raw SMBIOS structure bytes in hexadecimal and ASCII.

  @param[in] Data      Pointer to the structure data.
  @param[in] DataSize  Size of the structure including its string-set.
**/
STATIC
VOID
DumpStructure (
  IN UINT8  *Data,
  IN UINTN  DataSize
  )
{
  UINTN  ByteIndex;
  UINTN  Offset;

  Print (L"Dump Structure as:\n");
  for (Offset = 0; Offset < DataSize; Offset += 16) {
    Print (L"%08x:", Offset);
    for (ByteIndex = 0; ByteIndex < 16; ByteIndex++) {
      Print (ByteIndex == 8 ? L"-" : L" ");
      if ((Offset + ByteIndex) < DataSize) {
        Print (L"%02x", (UINTN)Data[Offset + ByteIndex]);
      } else {
        Print (L"  ");
      }
    }

    Print (L"  *");
    for (ByteIndex = 0; (ByteIndex < 16) && ((Offset + ByteIndex) < DataSize); ByteIndex++) {
      if ((Data[Offset + ByteIndex] >= 0x20) && (Data[Offset + ByteIndex] <= 0x7E)) {
        Print (L"%c", (UINTN)Data[Offset + ByteIndex]);
      } else {
        Print (L".");
      }
    }

    Print (L"*\n");
  }
}

/**
  Return a string from an SMBIOS structure string-set.

  @param[in] Structure     SMBIOS structure containing the string-set.
  @param[in] StringNumber  One-based SMBIOS string number.
  @param[in] StructureEnd  Address immediately after the structure.

  @return The requested ASCII string, or an empty string if it is invalid.
**/
STATIC
CHAR8 *
GetSmbiosString (
  IN SMBIOS_STRUCTURE     *Structure,
  IN SMBIOS_TABLE_STRING  StringNumber,
  IN UINT8                *StructureEnd
  )
{
  UINT8  *Current;
  UINT8  *StringEnd;
  UINTN  Index;

  if (StringNumber == 0) {
    return "";
  }

  // type字符集的开始指针
  Current = (UINT8 *)Structure + Structure->Length;
  for (Index = 1; Index <= StringNumber; Index++) {
    if ((Current >= StructureEnd) || (*Current == 0)) {
      return "";
    }

    // 寻找目标字符串
    /**
     * EDK II\0unknown\002/02/2022\0\0
          1       2          3
     */
    if (Index == StringNumber) {
      /**
        * 找到指定编号的字符串
                  ↓
          从字符串开头寻找 \0
                  ↓
          是否在有效范围内找到？
            ┌────┴────┐
            是        否
            ↓         ↓
          返回地址    返回空字符串
       */
      StringEnd = Current;
      while ((StringEnd < StructureEnd) && (*StringEnd != 0)) {
        StringEnd++;
      }

      return (StringEnd < StructureEnd) ? (CHAR8 *)Current : "";
    }

    while ((Current < StructureEnd) && (*Current != 0)) {
      Current++;
    }

    if (Current >= StructureEnd) {
      return "";
    }

    Current++;
  }

  return "";
}

/**
  Display an SMBIOS BIOS Information structure (Type 0).

  @param[in] Type0       Pointer to the Type 0 structure.
  @param[in] RecordSize  Total record size including the string-set.
**/
STATIC
VOID
DisplaySmbiosType0 (
  IN SMBIOS_TABLE_TYPE0  *Type0,
  IN UINTN               RecordSize
  )
{
  UINT8  *StructureEnd;

  // StructureEnd表示type 0的末尾地址，RecordSize表示整个type 0的大小，什么都包括，字符串集也包括
  StructureEnd = (UINT8 *)Type0 + RecordSize;

  Print (
    L"\nType=%u, Handle=0x%x\n",
    (UINTN)Type0->Hdr.Type,
    (UINTN)Type0->Hdr.Handle
    );
  DumpStructure ((UINT8 *)Type0, RecordSize);
  Print (L"Type: BIOS Information\n");
  Print (L"Length: %u\n", (UINTN)Type0->Hdr.Length);
  Print (L"Handle: %u\n", (UINTN)Type0->Hdr.Handle);
  Print (L"Vendor: %a\n", GetSmbiosString (&Type0->Hdr, Type0->Vendor, StructureEnd));
  Print (L"BIOS Version: %a\n", GetSmbiosString (&Type0->Hdr, Type0->BiosVersion, StructureEnd));
  Print (L"BIOS Starting Address Segment: 0x%04x\n", (UINTN)Type0->BiosSegment);
  Print (
    L"BIOS Release Date: %a\n",
    GetSmbiosString (&Type0->Hdr, Type0->BiosReleaseDate, StructureEnd)
    );

    // 判断容量大小，是否大于0xff，计算方式有所不用
  if (Type0->BiosSize != 0xFF) {
    Print (L"BIOS ROM Size: %u KB\n", (UINTN)(Type0->BiosSize + 1) * 64);
  } else if (Type0->Hdr.Length >= sizeof (SMBIOS_TABLE_TYPE0)) {
    Print (
      L"Extended BIOS ROM Size: %u %s\n",
      (UINTN)Type0->ExtendedBiosSize.Size,
      Type0->ExtendedBiosSize.Unit == 0 ? L"MB" : L"GB"
      );
  } else {
    Print (L"BIOS ROM Size: Unknown\n");
  }

  if (Type0->Hdr.Length >= OFFSET_OF (SMBIOS_TABLE_TYPE0, BIOSCharacteristicsExtensionBytes)) {
    DisplayBiosCharacteristics (Type0);
  }

  if (Type0->Hdr.Length > OFFSET_OF (SMBIOS_TABLE_TYPE0, BIOSCharacteristicsExtensionBytes)) {
    DisplayBiosCharacteristicsExtension1 (Type0->BIOSCharacteristicsExtensionBytes[0]);
  }

  if (Type0->Hdr.Length > (OFFSET_OF (SMBIOS_TABLE_TYPE0, BIOSCharacteristicsExtensionBytes) + 1)) {
    DisplayBiosCharacteristicsExtension2 (Type0->BIOSCharacteristicsExtensionBytes[1]);
  }

  if (Type0->Hdr.Length > OFFSET_OF (SMBIOS_TABLE_TYPE0, EmbeddedControllerFirmwareMinorRelease)) {
    Print (L"System BIOS Major Release: %u\n", (UINTN)Type0->SystemBiosMajorRelease);
    Print (L"System BIOS Minor Release: %u\n", (UINTN)Type0->SystemBiosMinorRelease);
    Print (
      L"Embedded Controller Firmware Major Release: %u\n",
      (UINTN)Type0->EmbeddedControllerFirmwareMajorRelease
      );
    Print (
      L"Embedded Controller Firmware Minor Release: %u\n",
      (UINTN)Type0->EmbeddedControllerFirmwareMinorRelease
      );
  }
}

/**
  Display the homework-defined SMBIOS Type 0x80.

  @param[in] OemType     Pointer to the homework OEM structure.
  @param[in] RecordSize  Total record size including the string-set.
**/
STATIC
VOID
DisplayHomeworkOemType (
  IN HOMEWORK_SMBIOS_OEM_TYPE  *OemType,
  IN UINTN                     RecordSize
  )
{
  Print (
    L"\nType=%u, Handle=0x%x\n",
    (UINTN)OemType->Header.Type,
    (UINTN)OemType->Header.Handle
    );
  DumpStructure ((UINT8 *)OemType, RecordSize);
  Print (L"Type: Homework OEM Information (Type 0x80)\n");
  Print (L"Length: %u\n", (UINTN)OemType->Header.Length);
  Print (L"Handle: %u\n", (UINTN)OemType->Header.Handle);
  Print (
    L"Signature: %c%c%c%c\n",
    (UINTN)(OemType->Signature & 0xFF),
    (UINTN)((OemType->Signature >> 8) & 0xFF),
    (UINTN)((OemType->Signature >> 16) & 0xFF),
    (UINTN)((OemType->Signature >> 24) & 0xFF)
    );
  Print (
    L"Version: %u.%u\n",
    (UINTN)OemType->MajorVersion,
    (UINTN)OemType->MinorVersion
    );
  Print (L"Status: 0x%04x\n", (UINTN)OemType->Status);
  Print (L"Feature Flags: 0x%08x\n", (UINTN)OemType->FeatureFlags);
  Print (L"Data Value: 0x%08x\n", (UINTN)OemType->DataValue);
}

/**
  Walk the SMBIOS table and display Type 0 and homework Type 0x80.

  @param[in] TableAddress  Address of the SMBIOS structure table.
  @param[in] TableSize     Maximum available table size.

  @retval EFI_SUCCESS           Both requested types were displayed.
  @retval EFI_NOT_FOUND         A requested type was not found.
  @retval EFI_COMPROMISED_DATA  The SMBIOS table layout is invalid.
**/
STATIC
EFI_STATUS
DisplaySmbiosTable (
  IN UINT64  TableAddress,
  IN UINT32  TableSize
  )
{
  HOMEWORK_SMBIOS_OEM_TYPE  *OemType;
  SMBIOS_STRUCTURE          *NextStructure;
  SMBIOS_STRUCTURE          *Structure;
  UINT8                     *TableEnd;
  UINTN                     RecordSize;
  BOOLEAN                   OemTypeFound;
  BOOLEAN                   Type0Found;

  Structure    = (SMBIOS_STRUCTURE *)(UINTN)TableAddress;
  TableEnd     = (UINT8 *)Structure + TableSize;
  OemTypeFound = FALSE;
  Type0Found   = FALSE;

  /**
    TableAddress
      │
      ▼
  ┌──────────────┐
  │ Type 0       │  BIOS Information
  ├──────────────┤
  │ Type 1       │  System Information
  ├──────────────┤
  │ Type 2       │  Baseboard Information
  ├──────────────┤
  │ ...          │
  ├──────────────┤
  │ Type 127     │  End-of-Table
  └──────────────┘
   */
  while (((UINT8 *)Structure + sizeof (SMBIOS_STRUCTURE)) <= TableEnd) {
    if ((Structure->Length < sizeof (SMBIOS_STRUCTURE)) ||
        (((UINT8 *)Structure + Structure->Length) > TableEnd))
    {
      return EFI_COMPROMISED_DATA;
    }

    NextStructure = GetNextStructure (Structure, TableEnd);
    if (NextStructure == NULL) {
      return EFI_COMPROMISED_DATA;
    }

    RecordSize = (UINTN)((UINT8 *)NextStructure - (UINT8 *)Structure);
    if (Structure->Type == SMBIOS_TYPE_BIOS_INFORMATION) {
      if (Structure->Length <= OFFSET_OF (SMBIOS_TABLE_TYPE0, BiosSize)) {
        return EFI_COMPROMISED_DATA;
      }

      DisplaySmbiosType0 ((SMBIOS_TABLE_TYPE0 *)Structure, RecordSize);
      Type0Found = TRUE;
    } else if ((Structure->Type == HOMEWORK_SMBIOS_OEM_TYPE_VALUE) &&
               (Structure->Length >= sizeof (HOMEWORK_SMBIOS_OEM_TYPE)))
    {
      OemType = (HOMEWORK_SMBIOS_OEM_TYPE *)Structure;
      if (OemType->Signature == HOMEWORK_SMBIOS_OEM_SIGNATURE) {
        DisplayHomeworkOemType (OemType, RecordSize);
        OemTypeFound = TRUE;
      }
    }

    if (Structure->Type == SMBIOS_TYPE_END_OF_TABLE) {
      break;
    }

    Structure = NextStructure;
  }

  if (!Type0Found) {
    Print (L"SMBIOS Type 0 was not found.\n");
  }

  if (!OemTypeFound) {
    Print (L"Homework SMBIOS Type 0x80 was not found.\n");
  }

  return (Type0Found && OemTypeFound) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

/**
  UEFI application entry point.

  @param[in] ImageHandle  Handle for this image.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS    The EPS, Type 0, and OEM type were displayed.
  @retval EFI_NOT_FOUND  The EPS or a requested type was not found.
  @retval Others         The SMBIOS table is invalid.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SMBIOS_TABLE_3_0_ENTRY_POINT  *Eps3;
  SMBIOS_TABLE_ENTRY_POINT      *Eps2;
  EFI_STATUS                    Status;
  UINT64                        TableAddress;
  UINT32                        TableSize;

  (VOID)ImageHandle;
  (VOID)SystemTable;

  //先寻找SMBIOS 2.x Entry Point Structure，如果找不到，再寻找SMBIOS 3.x Entry Point Structure
  Status = EfiGetSystemConfigurationTable (
             &gEfiSmbiosTableGuid,
             (VOID **)&Eps2
             );
  if (!EFI_ERROR (Status)) {
    // 显示打印SMBIOS 2.x Entry Point Structure
    DisplaySmbios2EntryPoint (Eps2);
    // 获取SMBIOS表的地址和大小
    TableAddress = Eps2->TableAddress;
    TableSize    = Eps2->TableLength;
  } else {
    // 如果找不到SMBIOS 2.x Entry Point Structure，再寻找SMBIOS 3.x Entry Point Structure
    Status = EfiGetSystemConfigurationTable (
               &gEfiSmbios3TableGuid,
               (VOID **)&Eps3
               );
    if (EFI_ERROR (Status)) {
      Print (L"SMBIOS entry point was not found.\n");
      return EFI_NOT_FOUND;
    }

    // 显示打印SMBIOS 3.x Entry Point Structure
    DisplaySmbios3EntryPoint (Eps3);
    TableAddress = Eps3->TableAddress;
    TableSize    = Eps3->TableMaximumSize;
  }

  // 调用DisplaySmbiosTable函数，遍历SMBIOS表，显示Type 0和自定义的Type 0x80
  return DisplaySmbiosTable (TableAddress, TableSize);
}
