/** @file
  header file for NULL named library for my Rtc shell command functions.

**/

#ifndef _UEFI_SHELL_MYRTC_COMMANDS_LIB_H_
#define _UEFI_SHELL_MYRTC_COMMANDS_LIB_H_

#include <Uefi.h>

#include <Guid/ShellLibHiiGuid.h>

#include <Protocol/Cpu.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/Arp.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/SortLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>

extern EFI_HII_HANDLE gShellMyRtcHiiHandle;

/**
  Function for 'ping' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunMyRtc (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif

