/** @file
  Homework 1.4 RTC read DXE driver.

  This DXE driver reads the CMOS RTC through I/O ports 0x70/0x71 and prints the
  current date and time during DXE.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>

#include "Homework14RtcDxe.h"

/**
  Read one CMOS RTC register.

  @param[in] Register  RTC register index.

  @return Value read from the RTC register.
**/
STATIC
UINT8
Homework14ReadRtcRegister (
  IN UINT8  Register
  )
{
  IoWrite8 (HOMEWORK14_RTC_INDEX_PORT, (UINT8)(Register & 0x7F));
  return IoRead8 (HOMEWORK14_RTC_DATA_PORT);
}

/**
  Convert one BCD byte to binary.

  @param[in] Value  BCD value.

  @return Binary value.
**/
STATIC
UINT8
Homework14BcdToBinary (
  IN UINT8  Value
  )
{
  return (UINT8)(((Value >> 4) * 10) + (Value & 0x0F));
}

/**
  Wait until RTC update is not in progress.
**/
STATIC
VOID
Homework14WaitRtcUpdateComplete (
  VOID
  )
{
  UINTN  Retry;

  for (Retry = 0; Retry < 100000; Retry++) {
    if ((Homework14ReadRtcRegister (HOMEWORK14_RTC_STATUS_A) & HOMEWORK14_RTC_STATUS_A_UIP) == 0) {
      return;
    }
  }
}

/**
  Read RTC and print the result.

  @param[in] ModuleName  Module name printed in the debug log.
**/
VOID
Homework14DumpRtcTime (
  IN CONST CHAR8  *ModuleName
  )
{
  UINT8    Seconds;
  UINT8    Minutes;
  UINT8    Hours;
  UINT8    Day;
  UINT8    Month;
  UINT8    Year;
  UINT8    Century;
  UINT8    StatusB;
  BOOLEAN  IsBinary;
  BOOLEAN  Is24Hour;
  BOOLEAN  IsPm;
  UINT16   FullYear;

  Homework14WaitRtcUpdateComplete ();

  StatusB  = Homework14ReadRtcRegister (HOMEWORK14_RTC_STATUS_B);
  IsBinary = (BOOLEAN)((StatusB & HOMEWORK14_RTC_STATUS_B_DM) != 0);
  Is24Hour = (BOOLEAN)((StatusB & HOMEWORK14_RTC_STATUS_B_24H) != 0);

  Seconds = Homework14ReadRtcRegister (HOMEWORK14_RTC_SECONDS);
  Minutes = Homework14ReadRtcRegister (HOMEWORK14_RTC_MINUTES);
  Hours   = Homework14ReadRtcRegister (HOMEWORK14_RTC_HOURS);
  Day     = Homework14ReadRtcRegister (HOMEWORK14_RTC_DAY_OF_MONTH);
  Month   = Homework14ReadRtcRegister (HOMEWORK14_RTC_MONTH);
  Year    = Homework14ReadRtcRegister (HOMEWORK14_RTC_YEAR);
  Century = Homework14ReadRtcRegister (HOMEWORK14_RTC_CENTURY);

  IsPm = (BOOLEAN)((!Is24Hour) && ((Hours & BIT7) != 0));
  if (!Is24Hour) {
    Hours = (UINT8)(Hours & 0x7F);
  }

  if (!IsBinary) {
    Seconds = Homework14BcdToBinary (Seconds);
    Minutes = Homework14BcdToBinary (Minutes);
    Hours   = Homework14BcdToBinary (Hours);
    Day     = Homework14BcdToBinary (Day);
    Month   = Homework14BcdToBinary (Month);
    Year    = Homework14BcdToBinary (Year);
    Century = Homework14BcdToBinary (Century);
  }

  if (!Is24Hour) {
    if (IsPm && (Hours != 12)) {
      Hours = (UINT8)(Hours + 12);
    } else if (!IsPm && (Hours == 12)) {
      Hours = 0;
    }
  }

  if ((Century >= 19) && (Century <= 21)) {
    FullYear = (UINT16)(Century * 100 + Year);
  } else if (Year < 80) {
    FullYear = (UINT16)(2000 + Year);
  } else {
    FullYear = (UINT16)(1900 + Year);
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: RTC Time %04u-%02u-%02u %02u:%02u:%02u, StatusB=0x%02x\n",
    ModuleName,
    (UINTN)FullYear,
    (UINTN)Month,
    (UINTN)Day,
    (UINTN)Hours,
    (UINTN)Minutes,
    (UINTN)Seconds,
    StatusB
    ));
}

/**
  Entry point for the RTC read DXE driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The driver executed successfully.
**/
EFI_STATUS
EFIAPI
Homework14RtcDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  (VOID)ImageHandle;
  (VOID)SystemTable;

  Homework14DumpRtcTime ("Homework14RtcDxe");
  return EFI_SUCCESS;
}
