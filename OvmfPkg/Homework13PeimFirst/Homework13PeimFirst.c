/** @file
  Homework 1.3 first PEIM for PEI dispatch order verification.

  This PEIM prints a marker during PEI dispatch. Its order is controlled by
  the PEI APRIORI file in OvmfPkgX64.fdf.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>

/**
  Entry point for the first homework PEIM.

  @param[in] FileHandle   Handle of the file being invoked.
  @param[in] PeiServices  Describes the list of possible PEI Services.

  @retval EFI_SUCCESS  The PEIM executed successfully.
**/
EFI_STATUS
EFIAPI
Homework13PeimFirstEntryPoint (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  (VOID)FileHandle;
  (VOID)PeiServices;

  DEBUG ((DEBUG_INFO, "Homework13PeimOrder: 1 - Homework13PeimFirst executed\n"));
  return EFI_SUCCESS;
}
