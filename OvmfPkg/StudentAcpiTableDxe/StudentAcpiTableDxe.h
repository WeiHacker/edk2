/** @file
  Student ACPI table definitions.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef STUDENT_ACPI_TABLE_DXE_H_
#define STUDENT_ACPI_TABLE_DXE_H_

#include <IndustryStandard/Acpi.h>

#define EFI_ACPI_STUDENT_TABLE_SIGNATURE  SIGNATURE_32 ('S', 'T', 'U', 'D')
#define EFI_ACPI_STUDENT_TABLE_REVISION   1
#define EFI_ACPI_STUDENT_TABLE_MAGIC      SIGNATURE_32 ('O', 'V', 'M', 'F')

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT32                         Magic;
  UINT16                         MajorVersion;
  UINT16                         MinorVersion;
  UINT64                         Data;
} EFI_ACPI_STUDENT_TABLE;

#pragma pack()

#endif
