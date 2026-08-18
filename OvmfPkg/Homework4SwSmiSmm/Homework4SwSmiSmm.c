/** @file
  Homework 4 software SMI handler.

  This SMM driver registers SwSmiInputValue 0x88 through the SMM Software
  Dispatch2 Protocol. The handler writes 0x88 to I/O port 0x80 and dumps the
  Q35 power-management registers required by the assignment.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Q35MchIch9.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SmmSwDispatch2.h>

#include "Homework4SwSmiSmm.h"

/**
  Dump the power-management registers required by the assignment.
**/
STATIC
VOID
DumpHomework4PowerManagementRegisters (
  VOID
  )
{
  UINT32  GenPmConA;
  UINT32  ABase;
  UINT32  Pm1EnSts;

  GenPmConA = PciRead32 (POWER_MGMT_REGISTER_Q35 (ICH9_GEN_PMCON_1));
  ABase     = PciRead32 (POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE)) & ICH9_PMBASE_MASK;
  Pm1EnSts  = IoRead32 (ABase + HOMEWORK4_PM1_EN_STS);

  DEBUG ((
    DEBUG_INFO,
    "Homework4SwSmiSmm: GEN_PMCON_A=0x%08x, ABASE=0x%04x, PM1_EN_STS=0x%08x\n",
    GenPmConA,
    ABase,
    Pm1EnSts
    ));
}

/**
  Child handler for homework 4 software SMI value 0x88.

  @param[in]     DispatchHandle  The unique handle assigned by SMM Core.
  @param[in]     Context         Optional context.
  @param[in,out] CommBuffer      Optional communication buffer.
  @param[in,out] CommBufferSize  Optional communication buffer size.

  @retval EFI_SUCCESS  The software SMI was handled successfully.
**/
EFI_STATUS
EFIAPI
Homework4SwSmiHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer      OPTIONAL,
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  IoWrite8 (HOMEWORK4_POST_PORT, HOMEWORK4_PORT80_CODE);
  DumpHomework4PowerManagementRegisters ();

  return EFI_SUCCESS;
}

/**
  Entry point for the Homework 4 SW SMI SMM driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The handler was registered successfully.
  @retval Others       Error returned by SMM protocol location or registration.
**/
EFI_STATUS
EFIAPI
Homework4SwSmiSmmEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_SMM_SW_DISPATCH2_PROTOCOL  *SwDispatch;
  EFI_SMM_SW_REGISTER_CONTEXT    RegisterContext;
  EFI_STATUS                     Status;
  EFI_HANDLE                     DispatchHandle;

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    (VOID **)&SwDispatch
                    );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (HOMEWORK4_SW_SMI_VALUE > SwDispatch->MaximumSwiValue) {
    return EFI_UNSUPPORTED;
  }

  RegisterContext.SwSmiInputValue = HOMEWORK4_SW_SMI_VALUE;
  Status = SwDispatch->Register (
                         SwDispatch,
                         Homework4SwSmiHandler,
                         &RegisterContext,
                         &DispatchHandle
                         );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "Homework4SwSmiSmm: registered SwSmiInputValue 0x%02x\n",
    (UINT32)RegisterContext.SwSmiInputValue
    ));

  return EFI_SUCCESS;
}
