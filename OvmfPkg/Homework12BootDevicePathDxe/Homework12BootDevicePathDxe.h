/** @file
  Homework 1.2 boot device path dump definitions.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HOMEWORK12_BOOT_DEVICE_PATH_DXE_H_
#define HOMEWORK12_BOOT_DEVICE_PATH_DXE_H_

#include <Uefi.h>

/**
  Dump BootOrder device paths from UEFI boot option variables.
**/
VOID
DumpBootDevicePaths (
  VOID
  );

#endif
