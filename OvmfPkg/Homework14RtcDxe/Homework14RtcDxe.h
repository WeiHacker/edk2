/** @file
  Homework 1.4 RTC read DXE driver definitions.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HOMEWORK14_RTC_DXE_H_
#define HOMEWORK14_RTC_DXE_H_

#include <Uefi.h>

#define HOMEWORK14_RTC_INDEX_PORT  0x70
#define HOMEWORK14_RTC_DATA_PORT   0x71

#define HOMEWORK14_RTC_SECONDS       0x00
#define HOMEWORK14_RTC_MINUTES       0x02
#define HOMEWORK14_RTC_HOURS         0x04
#define HOMEWORK14_RTC_DAY_OF_MONTH  0x07
#define HOMEWORK14_RTC_MONTH         0x08
#define HOMEWORK14_RTC_YEAR          0x09
#define HOMEWORK14_RTC_STATUS_A      0x0A
#define HOMEWORK14_RTC_STATUS_B      0x0B
#define HOMEWORK14_RTC_CENTURY       0x32

#define HOMEWORK14_RTC_STATUS_A_UIP  BIT7
#define HOMEWORK14_RTC_STATUS_B_24H  BIT1
#define HOMEWORK14_RTC_STATUS_B_DM   BIT2

/**
  Read RTC and print the result.

  @param[in] ModuleName  Module name printed in the debug log.
**/
VOID
Homework14DumpRtcTime (
  IN CONST CHAR8  *ModuleName
  );

#endif
