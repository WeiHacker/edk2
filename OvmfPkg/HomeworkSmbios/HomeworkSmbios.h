/** @file
  Homework SMBIOS OEM type definitions.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HOMEWORK_SMBIOS_H_
#define HOMEWORK_SMBIOS_H_

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>

#define HOMEWORK_SMBIOS_OEM_TYPE_VALUE  0x80
#define HOMEWORK_SMBIOS_OEM_SIGNATURE   SIGNATURE_32 ('H', 'W', '8', '0')

#pragma pack (push, 1)

typedef struct {
  SMBIOS_STRUCTURE    Header;
  UINT32              Signature;
  UINT8               MajorVersion;
  UINT8               MinorVersion;
  UINT16              Status;
  UINT32              FeatureFlags;
  UINT32              DataValue;
} HOMEWORK_SMBIOS_OEM_TYPE;

typedef struct {
  HOMEWORK_SMBIOS_OEM_TYPE    Formatted;
  UINT8                       EndOfStringSet[2];
} HOMEWORK_SMBIOS_OEM_RECORD;

#pragma pack (pop)

#endif
