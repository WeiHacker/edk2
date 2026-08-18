/** @file
  Q35 ICH9 I2C Master Protocol definitions.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HOMEWORK_I2C_MASTER_DXE_H_
#define HOMEWORK_I2C_MASTER_DXE_H_

#include <Uefi.h>
#include <Library/PciLib.h>
#include <Protocol/I2cMaster.h>

#define HOMEWORK_ICH9_SMBUS_PCI_ADDRESS  PCI_LIB_ADDRESS (0, 31, 3, 0)
#define HOMEWORK_ICH9_SMBUS_BASE         0x20
#define HOMEWORK_ICH9_SMBUS_HOSTC        0x40

#define HOMEWORK_SMB_HSTS       0x00
#define HOMEWORK_SMB_HST_CNT    0x02
#define HOMEWORK_SMB_HST_CMD    0x03
#define HOMEWORK_SMB_XMIT_SLVA  0x04
#define HOMEWORK_SMB_HST_D0     0x05

#define HOMEWORK_SMB_HSTS_BUSY   BIT0
#define HOMEWORK_SMB_HSTS_INTR   BIT1
#define HOMEWORK_SMB_HSTS_ERROR  (BIT2 | BIT3 | BIT4)
#define HOMEWORK_SMB_START       BIT6
#define HOMEWORK_SMB_BYTE        BIT2
#define HOMEWORK_SMB_BYTE_DATA   BIT3

#define HOMEWORK_I2C_BUS_FREQUENCY  100000

#endif
