/** @file
  Shutdown warm reset ACPI table definitions.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SHUTDOWN_WARM_RESET_ACPI_TABLE_DXE_H_
#define SHUTDOWN_WARM_RESET_ACPI_TABLE_DXE_H_

#include <IndustryStandard/Acpi.h>

#define EFI_ACPI_SHUTDOWN_WARM_RESET_TABLE_SIGNATURE  SIGNATURE_32 ('S', 'S', 'D', 'T')
#define EFI_ACPI_SHUTDOWN_WARM_RESET_TABLE_REVISION   2
#define EFI_ACPI_SHUTDOWN_WARM_RESET_OEM_TABLE_ID     SIGNATURE_64 ('S', 'H', 'D', 'W', 'R', 'S', 'T', ' ')

#pragma pack(1)

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT8                          AmlCode[43];
} EFI_ACPI_SHUTDOWN_WARM_RESET_TABLE;

#pragma pack()

#endif
