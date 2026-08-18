/** @file
  DDR4 SPD information shell application definitions.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HOMEWORK_SPD_INFO_SHELL_H_
#define HOMEWORK_SPD_INFO_SHELL_H_

#include <Uefi.h>
#include <Protocol/I2cMaster.h>

#define HOMEWORK_SPD_FIRST_ADDRESS  0x50
#define HOMEWORK_SPD_LAST_ADDRESS   0x57
#define HOMEWORK_SPD_DDR4_TYPE      0x0C
#define HOMEWORK_SPD_PAGE_ZERO      0x36
#define HOMEWORK_SPD_PAGE_ONE       0x37
#define HOMEWORK_SPD_BASE_SIZE      128

typedef struct {
  UINTN                OperationCount;
  EFI_I2C_OPERATION    Operation[2];
} HOMEWORK_I2C_REQUEST_PACKET;

#endif
