/** @file
  Header file for ShellRtcCmd - a UEFI Shell dynamic command.

  This driver registers the "rtc" dynamic command with the UEFI Shell.
  When executed, it reads the current date and time from the CMOS RTC
  via I/O ports 0x70/0x71 and prints it to the console.

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SHELL_RTC_CMD_H_
#define SHELL_RTC_CMD_H_

#include <Uefi.h>
#include <Protocol/ShellDynamicCommand.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>

//
// CMOS RTC Register Addresses
//
#define RTC_REG_SECONDS             0x00
#define RTC_REG_MINUTES             0x02
#define RTC_REG_HOURS               0x04
#define RTC_REG_DAY_OF_MONTH        0x07
#define RTC_REG_MONTH               0x08
#define RTC_REG_YEAR                0x09
#define RTC_REG_STATUS_A            0x0A
#define RTC_REG_STATUS_B            0x0B
#define RTC_REG_CENTURY             0x32

//
// Status Register A bit definitions
//
#define RTC_STATUS_A_UIP            BIT7   // Update In Progress

//
// Status Register B bit definitions
//
#define RTC_STATUS_B_DM             BIT2   // Data Mode (0=BCD, 1=Binary)
#define RTC_STATUS_B_24H            BIT1   // 24/12 Hour mode (1=24-hour)

/**
  Reads a CMOS RTC register via I/O ports 0x70/0x71.

  @param[in]  Register  The CMOS register index to read (0x00-0x7F).

  @return The 8-bit value read from the specified register.
**/
UINT8
ReadRtcRegister (
  IN UINT8  Register
  );

/**
  Converts a BCD-encoded byte to a binary value.

  @param[in]  BcdValue  The BCD-encoded value (0x00-0x99).

  @return The binary equivalent.
**/
UINT8
BcdToDecimal (
  IN UINT8  BcdValue
  );

/**
  Handler for the "rtc" dynamic command.

  @param[in] This             The dynamic command protocol instance.
  @param[in] SystemTable      A pointer to the UEFI System Table.
  @param[in] ShellParameters  The shell parameters for this command.
  @param[in] Shell            The shell protocol instance.

  @retval SHELL_SUCCESS       The command executed successfully.
**/
SHELL_STATUS
EFIAPI
RtcCommandHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN EFI_SYSTEM_TABLE                    *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL      *ShellParameters,
  IN EFI_SHELL_PROTOCOL                 *Shell
  );

/**
  Returns the help text for the "rtc" command.

  @param[in]  This     The dynamic command protocol instance.
  @param[in]  Language The language code for the help text.

  @return A pointer to the help text string, or NULL if unsupported.
**/
CHAR16 *
EFIAPI
RtcCommandGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                          *Language
  );

#endif
