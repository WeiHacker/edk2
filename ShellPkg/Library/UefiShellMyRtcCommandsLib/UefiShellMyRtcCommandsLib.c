/** @file
  Main file for NULL named library for My Rtc shell command functions.
**
**
**/

#include "UefiShellMyRtcCommandsLib.h"

#include "UefiShellMyRtcCommandsLib.h"

CONST CHAR16 gShelMyRtcFileName[] = L"ShellMyRtcCommands";
EFI_HII_HANDLE gShellMyRtcHiiHandle = NULL;

/**
  return the file name of the help text file if not using HII.

  @return The string pointer to the file name.
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameMyRtc (
  VOID
  )
{
  return (gShelMyRtcFileName);
}

/**
  Constructor for the Shell My Rtc Commands library.

  Install the handlers for My Rtc UEFI Shell 2.0 profile commands.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           The shell command handlers were installed sucessfully.
  @retval EFI_UNSUPPORTED       The shell level required was not found.
**/
EFI_STATUS
EFIAPI
ShellMyRtcCommandsLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  gShellMyRtcHiiHandle = NULL;

  //
  // check our bit of the profiles mask
  //
  if ((PcdGet8(PcdShellProfileMask) & BIT3) == 0) {
    return (EFI_SUCCESS);
  }

  gShellMyRtcHiiHandle = HiiAddPackages (
                             &gShellMyRtcHiiGuid, 
                             gImageHandle, 
                             UefiShellMyRtcCommandsLibStrings, 
                             NULL
                             );
  if (gShellMyRtcHiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }
  //
  // install our shell command handlers
  //
  ShellCommandRegisterCommandName(
    L"MyRtcCmd",    
    ShellCommandRunMyRtc, 
    ShellCommandGetManFileNameMyRtc, 
    0, 
    L"MyRtcCmd", 
    TRUE, 
    gShellMyRtcHiiHandle, 
    STRING_TOKEN(STR_MY_RTC_HELP_PING)
    );

  return (EFI_SUCCESS);
}

/**
  Destructor for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
ShellMyRtcCommandsLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellMyRtcHiiHandle != NULL) {
    HiiRemovePackages(gShellMyRtcHiiHandle);
  }
  return (EFI_SUCCESS);
}
