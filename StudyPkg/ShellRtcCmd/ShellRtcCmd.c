/** @file
  Implementation of the "rtc" UEFI Shell dynamic command.

  This driver registers a "rtc" dynamic command with the UEFI Shell.
  The command reads the current date and time from the CMOS RTC
  using direct I/O port access (0x70/0x71), without using any
  UEFI Runtime Services or protocol abstractions.

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "ShellRtcCmd.h"

//
// -----------------------------------------------------------------------------
// The dynamic command protocol instance published to the UEFI Shell.
// -----------------------------------------------------------------------------
//
EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  mShellRtcCmdProtocol = {
  L"rtc",                        // Command name in the shell
  RtcCommandHandler,             // Called when user types "rtc"
  RtcCommandGetHelp              // Returns help text
};

/**
  Reads a CMOS RTC register via I/O ports 0x70/0x71.

  Writes the register index to port 0x70, then reads the value from port 0x71.
  Bit 7 of port 0x70 controls NMI; it is always cleared (NMI enabled) when
  accessing RTC registers.

  @param[in]  Register  The CMOS register index to read (0x00-0x7F).

  @return The 8-bit value read from the specified register.
**/
UINT8
ReadRtcRegister (
  IN UINT8  Register
  )
{
  //
  // Write the register index with bit 7 = 0 to keep NMI enabled.
  //
  IoWrite8 (0x70, (UINT8)(Register & 0x7F));

  //
  // Read the register value from the data port.
  //
  return IoRead8 (0x71);
}

/**
  Converts a BCD-encoded byte to a binary value.

  Each nibble of the input represents a decimal digit.
  For example: BCD 0x59 = 59 decimal.

  @param[in]  BcdValue  The BCD-encoded value (0x00-0x99).

  @return The binary equivalent.
**/
UINT8
BcdToDecimal (
  IN UINT8  BcdValue
  )
{
  return (UINT8)(((BcdValue >> 4) & 0x0F) * 10 + (BcdValue & 0x0F));
}

/**
  Handler for the "rtc" dynamic command.

  Reads the current date and time from the CMOS RTC and prints it in a
  human-readable format to the console. The function waits for the RTC
  Update-In-Progress flag to clear before reading to ensure consistency.

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
  )
{
  UINT8   Seconds;
  UINT8   Minutes;
  UINT8   Hours;
  UINT8   Day;
  UINT8   Month;
  UINT16  Year;
  UINT8   Century;
  BOOLEAN IsBinary;
  BOOLEAN Is24Hour;
  UINT8   StatusB;
  UINTN   Retries;

  //
  // Wait for the RTC to finish any in-progress update.
  // The UIP (Update In Progress) bit is set while the RTC is
  // updating its internal registers. We must wait until it clears
  // before reading to obtain consistent values.
  // A timeout is enforced in case the RTC is not functional.
  //
  Retries = 1000;
  while ((ReadRtcRegister (RTC_REG_STATUS_A) & RTC_STATUS_A_UIP) != 0) {
    gBS->Stall (10);
    Retries--;
    if (Retries == 0) {
      break;
    }
  }

  //
  // Read the Status Register B to determine data format and hour mode.
  //
  StatusB  = ReadRtcRegister (RTC_REG_STATUS_B);
  IsBinary = (BOOLEAN)((StatusB & RTC_STATUS_B_DM)  != 0);
  Is24Hour = (BOOLEAN)((StatusB & RTC_STATUS_B_24H) != 0);

  //
  // Read all time/date registers sequentially.
  //
  Seconds = ReadRtcRegister (RTC_REG_SECONDS);
  Minutes = ReadRtcRegister (RTC_REG_MINUTES);
  Hours   = ReadRtcRegister (RTC_REG_HOURS);
  Day     = ReadRtcRegister (RTC_REG_DAY_OF_MONTH);
  Month   = ReadRtcRegister (RTC_REG_MONTH);
  Year    = ReadRtcRegister (RTC_REG_YEAR);
  Century = ReadRtcRegister (RTC_REG_CENTURY);

  //
  // Convert BCD values to binary if the RTC is in BCD mode.
  //
  if (!IsBinary) {
    Seconds = BcdToDecimal (Seconds);
    Minutes = BcdToDecimal (Minutes);
    Hours   = BcdToDecimal (Hours);
    Day     = BcdToDecimal (Day);
    Month   = BcdToDecimal (Month);
    Year    = BcdToDecimal ((UINT8)Year);
    Century = BcdToDecimal (Century);
  }

  //
  // If the RTC is in 12-hour mode, extract the PM flag and convert to 24-hour.
  //
  if (!Is24Hour) {
    if ((Hours & 0x80) != 0) {
      //
      // PM indicator is set. Convert 1-12 PM to 13-23.
      //
      Hours = (UINT8)((Hours & 0x7F));
      if (Hours != 12) {
        Hours = (UINT8)(Hours + 12);
      }
    } else {
      //
      // AM indicator. 12 AM should be 0.
      //
      Hours = (UINT8)(Hours & 0x7F);
      if (Hours == 12) {
        Hours = 0;
      }
    }
  }

  //
  // Determine the full 4-digit year.
  // If the century register gives a reasonable value, use it;
  // otherwise use the heuristic: year < 80 -> 2000 + year, else 1900 + year.
  //
  if ((Century >= 19) && (Century <= 21)) {
    Year = (UINT16)(Century * 100 + Year);
  } else if (Year < 80) {
    Year = (UINT16)(2000 + Year);
  } else {
    Year = (UINT16)(1900 + Year);
  }

  {
    //
    // Format and print the date and time using ConOut->OutputString,
    // which is the most reliable output method in a dynamic command.
    //
    CHAR16  OutputStr[64];

    UnicodeSPrint (
      OutputStr,
      sizeof (OutputStr),
      L"Current RTC Time: %04d-%02d-%02d %02d:%02d:%02d\r\n",
      (UINTN)Year,
      (UINTN)Month,
      (UINTN)Day,
      (UINTN)Hours,
      (UINTN)Minutes,
      (UINTN)Seconds
      );

    SystemTable->ConOut->OutputString (SystemTable->ConOut, OutputStr);
  }

  return SHELL_SUCCESS;
}

/**
  Returns the help text for the "rtc" command.

  Provides a brief description and usage information for the "rtc" command
  in the requested language.

  @param[in]  This     The dynamic command protocol instance.
  @param[in]  Language The language code for the help text.

  @return A pointer to the help text string, or NULL if unsupported.
**/
CHAR16 *
EFIAPI
RtcCommandGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL  *This,
  IN CONST CHAR8                          *Language
  )
{
  //
  // Return a statically allocated help string.
  //
  return L"Reads the current date and time from the CMOS RTC.\n"
          "Uses direct I/O port access (0x70/0x71).\n"
          "Does not use any UEFI Runtime Services.\n";
}

/**
  The entry point for the ShellRtcCmd driver.

  Installs the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL so that the UEFI Shell
  recognizes "rtc" as an internal command.

  @param[in] ImageHandle  The firmware-allocated handle for this image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The protocol was installed successfully.
  @retval EFI_OUT_OF_RESOURCES  Insufficient resources.
  @retval others                An error occurred during installation.
**/
EFI_STATUS
EFIAPI
ShellRtcCmdEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install the dynamic command protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mShellRtcCmdProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ShellRtcCmd: Failed to install dynamic command protocol: %r\n",
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_INIT,
    "ShellRtcCmd: Registered \"rtc\" dynamic command.\n"
    ));

  return EFI_SUCCESS;
}

/**
  Unloads the ShellRtcCmd driver.

  Uninstalls the EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL when the driver is
  unloaded, removing the "rtc" command from the shell.

  @param[in] ImageHandle  The firmware-allocated handle for this image.

  @retval EFI_SUCCESS           The protocol was uninstalled successfully.
  @retval others                An error occurred during uninstallation.
**/
EFI_STATUS
EFIAPI
ShellRtcCmdUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = gBS->UninstallProtocolInterface (
                  ImageHandle,
                  &gEfiShellDynamicCommandProtocolGuid,
                  &mShellRtcCmdProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ShellRtcCmd: Failed to uninstall dynamic command protocol: %r\n",
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_INIT,
    "ShellRtcCmd: Unregistered \"rtc\" dynamic command.\n"
    ));

  return EFI_SUCCESS;
}
