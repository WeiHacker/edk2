/** @file
  Homework 4 software SMI trigger application.

  This application locates EFI_SMM_CONTROL2_PROTOCOL and triggers a software
  SMI with command value 0x88 and data value 0x88.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Protocol/SmmControl2.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#define HOMEWORK4_SW_SMI_VALUE  0x88

/**
  Application entry point.

  @param[in] ImageHandle  The firmware allocated handle for the image.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS      The software SMI was triggered.
  @retval EFI_UNSUPPORTED  EFI_SMM_CONTROL2_PROTOCOL was not found.
  @retval Others           Error returned by EFI_SMM_CONTROL2_PROTOCOL.Trigger().
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;
  EFI_SMM_CONTROL2_PROTOCOL  *SmmControl2;
  UINT8                      CommandPort;
  UINT8                      DataPort;

  Status = gBS->LocateProtocol (
                  &gEfiSmmControl2ProtocolGuid,
                  NULL,
                  (VOID **)&SmmControl2
                  );
  if (EFI_ERROR (Status)) {
    Print (L"Homework4SwSmiTriggerApp: EFI_SMM_CONTROL2_PROTOCOL not found: %r\r\n", Status);
    return Status;
  }

  CommandPort = HOMEWORK4_SW_SMI_VALUE;
  DataPort    = HOMEWORK4_SW_SMI_VALUE;

  Print (L"Homework4SwSmiTriggerApp: trigger SW SMI 0x%02x\r\n", CommandPort);

  Status = SmmControl2->Trigger (
                          SmmControl2,
                          &CommandPort,
                          &DataPort,
                          FALSE,
                          0
                          );
  if (EFI_ERROR (Status)) {
    Print (L"Homework4SwSmiTriggerApp: trigger failed: %r\r\n", Status);
    return Status;
  }

  Print (L"Homework4SwSmiTriggerApp: trigger done\r\n");
  return EFI_SUCCESS;
}
