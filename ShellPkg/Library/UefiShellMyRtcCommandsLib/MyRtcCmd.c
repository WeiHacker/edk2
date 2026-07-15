/** @file
  The implementation for Shell command my Rtc

  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellMyRtcCommandsLib.h"
#include <Library/IoLib.h>
#define CMOS_INDEX    0x70
#define CMOS_DATA     0x71 
STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v", TypeValue},
  {L"-h", TypeValue},
  {NULL, TypeMax}
  };


VOID
PrintRtcTime (
  VOID
  )
{
    Print (L"this is my Rtc cmd !\n");
      

    UINT8 Second;


    IoWrite8 (CMOS_INDEX, 0x00);
    Second = IoRead8 (CMOS_DATA);

    Print (L"RTC Second (raw): 0x%02X\n", Second);

    Print (L"RTC Second (raw): 0x%02X\n", Second);

/*     UINT8 second = 0;
    UINT8 minute = 0;
    UINT8 hour = 0;
    UINT8 weekday = 0;
    UINT8 date = 0;
    UINT8 month = 0;
    UINT8 year = 0;

    IoWrite8 (CMOS_INDEX, 0x00); 
    second = IoRead8 (CMOS_DATA);    
    IoWrite8 (CMOS_INDEX, 0x02); 
    minute = IoRead8 (CMOS_DATA);
    IoWrite8 (CMOS_INDEX, 0x04); 
    hour = IoRead8 (CMOS_DATA);
    IoWrite8 (CMOS_INDEX, 0x06); 
    weekday = IoRead8 (CMOS_DATA);
    IoWrite8 (CMOS_INDEX, 0x07); 
    date = IoRead8 (CMOS_DATA);
    IoWrite8 (CMOS_INDEX, 0x08); 
    month = IoRead8 (CMOS_DATA);
    IoWrite8 (CMOS_INDEX, 0x09); 
    year = IoRead8 (CMOS_DATA); 
    Print (L"%02x/%02x/%02x %02x %02x:%02x:%02x\n", year,month,date,weekday,hour,minute,second); 
 */

}



/**
  Function for my Rtc command.

  @param[in]  ImageHandle           Handle to the Image (NULL if Internal).
  @param[in]  SystemTable           Pointer to the System Table (NULL if Internal).

  @retval  SHELL_SUCCESS            The command completed successfully.
  @retval  Others                   The command failed.

**/
SHELL_STATUS
EFIAPI
ShellCommandRunMyRtc (
  IN  EFI_HANDLE                    ImageHandle,
  IN  EFI_SYSTEM_TABLE              *SystemTable
  )
{
  EFI_STATUS                      Status;
  LIST_ENTRY                      *CheckPackage;
  CHAR16                  *ProblemParam;

  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = ShellCommandLineParse (ParamList, &CheckPackage, &ProblemParam, TRUE);
  ASSERT_EFI_ERROR (Status); 

  if (ShellCommandLineGetFlag (CheckPackage, L"-v")) {
    ShellPrintEx (-1, -1, L"%V v1.0 %N\r\n");
    return SHELL_SUCCESS;
  }
  
  if (ShellCommandLineGetFlag (CheckPackage, L"-h")) { 
      //ShellPrintEx (-1, -1, L"%V my Rtc cmd for help... %N\r\n");
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MY_RTC_HELP_PING), gShellMyRtcHiiHandle, L"MyRtcCmd");
      return SHELL_SUCCESS;
  }

  Print (L"this is my Rtc cmd !\n");
  PrintRtcTime ();
  return SHELL_SUCCESS;
}
