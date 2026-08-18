/** @file
  Q35 software SMI child dispatcher.

  This driver publishes EFI_SMM_SW_DISPATCH2_PROTOCOL for OVMF Q35. It
  dispatches writes to the APM control port by their SwSmiInputValue.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Q35MchIch9.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Protocol/SmmSwDispatch2.h>

#define Q35_SMI_STS_OFFSET    0x34
#define Q35_SMI_STS_APM_STS  BIT5
#define Q35_SW_SMI_MIN        1
#define Q35_SW_SMI_MAX        0xFF

typedef struct {
  EFI_SMM_HANDLER_ENTRY_POINT2    Handler;
  EFI_HANDLE                      DispatchHandle;
} Q35_SW_SMI_HANDLER;

STATIC Q35_SW_SMI_HANDLER  mSwSmiHandlers[Q35_SW_SMI_MAX + 1];
STATIC UINT32              mPmBase;

/**
  Register a child handler for one software SMI value.

  @param[in]     This              Software SMI Dispatch2 protocol instance.
  @param[in]     DispatchFunction  Child handler to register.
  @param[in,out] RegisterContext   Requested software SMI value.
  @param[out]    DispatchHandle    Handle assigned to the child handler.

  @retval EFI_SUCCESS            The child handler was registered.
  @retval EFI_INVALID_PARAMETER  A parameter or SMI value is invalid.
  @retval EFI_OUT_OF_RESOURCES   No free software SMI value is available.
**/
STATIC
EFI_STATUS
EFIAPI
Q35SwSmiRegister (
  IN     CONST EFI_SMM_SW_DISPATCH2_PROTOCOL  *This,
  IN           EFI_SMM_HANDLER_ENTRY_POINT2   DispatchFunction,
  IN OUT       EFI_SMM_SW_REGISTER_CONTEXT    *RegisterContext,
  OUT          EFI_HANDLE                     *DispatchHandle
  )
{
  UINTN  SwSmiInputValue;

  if ((DispatchFunction == NULL) || (RegisterContext == NULL) ||
      (DispatchHandle == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  SwSmiInputValue = RegisterContext->SwSmiInputValue;
  if (SwSmiInputValue == MAX_UINTN) {
    for (SwSmiInputValue = Q35_SW_SMI_MIN;
         SwSmiInputValue <= Q35_SW_SMI_MAX;
         SwSmiInputValue++)
    {
      if (mSwSmiHandlers[SwSmiInputValue].Handler == NULL) {
        RegisterContext->SwSmiInputValue = SwSmiInputValue;
        break;
      }
    }

    if (SwSmiInputValue > Q35_SW_SMI_MAX) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  if ((SwSmiInputValue < Q35_SW_SMI_MIN) ||
      (SwSmiInputValue > Q35_SW_SMI_MAX) ||
      (mSwSmiHandlers[SwSmiInputValue].Handler != NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  mSwSmiHandlers[SwSmiInputValue].Handler        = DispatchFunction;
  mSwSmiHandlers[SwSmiInputValue].DispatchHandle =
    (EFI_HANDLE)&mSwSmiHandlers[SwSmiInputValue];
  *DispatchHandle = mSwSmiHandlers[SwSmiInputValue].DispatchHandle;

  return EFI_SUCCESS;
}

/**
  Unregister a software SMI child handler.

  @param[in] This            Software SMI Dispatch2 protocol instance.
  @param[in] DispatchHandle  Handle returned by Q35SwSmiRegister().

  @retval EFI_SUCCESS            The child handler was removed.
  @retval EFI_INVALID_PARAMETER  DispatchHandle is not registered.
**/
STATIC
EFI_STATUS
EFIAPI
Q35SwSmiUnregister (
  IN CONST EFI_SMM_SW_DISPATCH2_PROTOCOL  *This,
  IN       EFI_HANDLE                     DispatchHandle
  )
{
  UINTN  SwSmiInputValue;

  for (SwSmiInputValue = Q35_SW_SMI_MIN;
       SwSmiInputValue <= Q35_SW_SMI_MAX;
       SwSmiInputValue++)
  {
    if (mSwSmiHandlers[SwSmiInputValue].DispatchHandle == DispatchHandle) {
      mSwSmiHandlers[SwSmiInputValue].Handler        = NULL;
      mSwSmiHandlers[SwSmiInputValue].DispatchHandle = NULL;
      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

STATIC EFI_SMM_SW_DISPATCH2_PROTOCOL  mSwDispatch2 = {
  Q35SwSmiRegister,
  Q35SwSmiUnregister,
  Q35_SW_SMI_MAX
};

/**
  Dispatch a Q35 APM software SMI to its registered child handler.

  @param[in]     DispatchHandle  Root SMI dispatch handle.
  @param[in]     Context         Root SMI context.
  @param[in,out] CommBuffer      Root SMI communication buffer.
  @param[in,out] CommBufferSize  Root SMI communication buffer size.

  @retval EFI_SUCCESS  The SMI was handled or no child was registered.
  @return              Status returned by the registered child handler.
**/
STATIC
EFI_STATUS
EFIAPI
Q35SwSmiDispatcher (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context         OPTIONAL,
  IN OUT VOID        *CommBuffer      OPTIONAL,
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_SMM_SW_REGISTER_CONTEXT  RegisterContext;
  EFI_SMM_SW_CONTEXT           SwContext;
  Q35_SW_SMI_HANDLER           *SwSmiHandler;
  EFI_STATUS                   Status;
  UINT8                        CommandPort;
  UINT8                        DataPort;
  UINT32                       SmiStatus;
  UINTN                        SwContextSize;

  CommandPort  = IoRead8 (ICH9_APM_CNT);
  DataPort     = IoRead8 (ICH9_APM_STS);
  SmiStatus    = IoRead32 (mPmBase + Q35_SMI_STS_OFFSET);
  SwSmiHandler = &mSwSmiHandlers[CommandPort];

  //
  // QEMU provides the SW SMI selector through APM_CNT even when the APM bit
  // is not reported in SMI_STS. Dispatch only explicitly registered values.
  //
  if (SwSmiHandler->Handler == NULL) {
    return EFI_SUCCESS;
  }

  DEBUG ((
    DEBUG_INFO,
    "Q35SwSmiDispatchSmm: CommandPort=0x%02x DataPort=0x%02x SMI_STS=0x%08x\n",
    CommandPort,
    DataPort,
    SmiStatus
    ));

  RegisterContext.SwSmiInputValue = CommandPort;
  SwContext.SwSmiCpuIndex         = gSmst->CurrentlyExecutingCpu;
  SwContext.CommandPort           = CommandPort;
  SwContext.DataPort              = DataPort;
  SwContextSize                   = sizeof (SwContext);

  Status = SwSmiHandler->Handler (
                           SwSmiHandler->DispatchHandle,
                           &RegisterContext,
                           &SwContext,
                           &SwContextSize
                           );

  //
  // APM_STS is write-one-to-clear. Acknowledge the source after the child
  // handler finishes so that the same software SMI is not dispatched again.
  //
  IoWrite32 (mPmBase + Q35_SMI_STS_OFFSET, Q35_SMI_STS_APM_STS);

  return Status;
}

/**
  Publish the Q35 software SMI Dispatch2 protocol.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The protocol and root handler were installed.
  @retval Others       Registration or protocol installation failed.
**/
EFI_STATUS
EFIAPI
Q35SwSmiDispatchSmmEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HANDLE  ProtocolHandle;
  EFI_HANDLE  RootDispatchHandle;
  EFI_STATUS  Status;

  mPmBase = PciRead32 (POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE)) &
            ICH9_PMBASE_MASK;
  if (mPmBase == 0) {
    return EFI_UNSUPPORTED;
  }

  Status = gSmst->SmiHandlerRegister (
                    Q35SwSmiDispatcher,
                    NULL,
                    &RootDispatchHandle
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ProtocolHandle = NULL;
  Status = gSmst->SmmInstallProtocolInterface (
                    &ProtocolHandle,
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mSwDispatch2
                    );
  if (EFI_ERROR (Status)) {
    gSmst->SmiHandlerUnRegister (RootDispatchHandle);
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Q35SwSmiDispatchSmm: installed SMM SW Dispatch2 protocol\n"));
  return EFI_SUCCESS;
}
