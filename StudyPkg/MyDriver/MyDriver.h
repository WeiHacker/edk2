/** @file
  Header file for MyDriver - a UEFI Driver Model example.

  This driver demonstrates the standard UEFI Driver Model by:
  - Implementing EFI_DRIVER_BINDING_PROTOCOL (Supported, Start, Stop)
  - Installing Component Name protocols for user-friendly identification
  - Binding to any handle that supports DevicePath protocol

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MYDRIVER_H_
#define MYDRIVER_H_

#include <Uefi.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DevicePath.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Guid/StudyDriverGuid.h>
#include <Protocol/StudyDriverProtocol.h>

///
/// Driver Binding Protocol instance declaration.
///
extern EFI_DRIVER_BINDING_PROTOCOL   gStudyMyDriverBinding;

///
/// Component Name Protocol instance declarations.
///
extern EFI_COMPONENT_NAME_PROTOCOL   gStudyMyDriverComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gStudyMyDriverComponentName2;

///
/// Private context structure for each device this driver manages.
///
typedef struct {
  ///
  /// The signature used to identify the private data structure.
  ///
  UINTN                             Signature;

  ///
  /// The standard driver binding protocol handle.
  ///
  EFI_HANDLE                        Handle;

  ///
  /// The DevicePath protocol on the controller handle, opened BY_DRIVER.
  ///
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;

  ///
  /// The controller handle this instance is bound to.
  ///
  EFI_HANDLE                        ControllerHandle;
} STUDY_MYDRIVER_PRIVATE_DATA;

///
/// Signature for identifying the private context structure.
///
#define STUDY_MYDRIVER_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('S', 'M', 'D', 'r')

///
/// Macro to retrieve a STUDY_MYDRIVER_PRIVATE_DATA from a base pointer.
///
#define STUDY_MYDRIVER_PRIVATE_DATA_FROM_THIS(a)  CR (a, STUDY_MYDRIVER_PRIVATE_DATA, Handle, STUDY_MYDRIVER_PRIVATE_DATA_SIGNATURE)

/**
  Check whether the driver supports the given controller.

  @param[in] This                 Pointer to the EFI_DRIVER_BINDING_PROTOCOL.
  @param[in] ControllerHandle     Handle of the controller to test.
  @param[in] RemainingDevicePath  Optional pointer to the remaining device path.

  @retval EFI_SUCCESS             The driver supports the controller.
  @retval EFI_UNSUPPORTED         The driver does not support the controller.
**/
EFI_STATUS
EFIAPI
StudyMyDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start the driver on the given controller.

  @param[in] This                 Pointer to the EFI_DRIVER_BINDING_PROTOCOL.
  @param[in] ControllerHandle     Handle of the controller to start.
  @param[in] RemainingDevicePath  Optional pointer to the remaining device path.

  @retval EFI_SUCCESS             The driver was successfully started.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
  @retval others                  An error occurred while starting the driver.
**/
EFI_STATUS
EFIAPI
StudyMyDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stop the driver on the given controller.

  @param[in] This                 Pointer to the EFI_DRIVER_BINDING_PROTOCOL.
  @param[in] ControllerHandle     Handle of the controller to stop.
  @param[in] NumberOfChildren     Number of child handles.
  @param[in] ChildHandleBuffer    List of child handles to stop.

  @retval EFI_SUCCESS             The driver was successfully stopped.
  @retval EFI_DEVICE_ERROR        The driver could not be stopped.
**/
EFI_STATUS
EFIAPI
StudyMyDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  );

/**
  Retrieves a Unicode string that is the user-readable name of the driver.

  @param[in]  This       Pointer to the EFI_COMPONENT_NAME_PROTOCOL.
  @param[in]  Language   Pointer to the language code string.
  @param[out] DriverName Pointer to the Unicode driver name string.

  @retval EFI_SUCCESS           The driver name was returned.
  @retval EFI_UNSUPPORTED       The requested language is not supported.
**/
EFI_STATUS
EFIAPI
StudyMyDriverGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user-readable name of the controller
  that is managed by this driver.

  @param[in]  This             Pointer to the EFI_COMPONENT_NAME_PROTOCOL.
  @param[in]  ControllerHandle Handle of the controller.
  @param[in]  ChildHandle      Handle of the child (or NULL).
  @param[in]  Language         Pointer to the language code string.
  @param[out] ControllerName   Pointer to the Unicode controller name string.

  @retval EFI_SUCCESS           The controller name was returned.
  @retval EFI_UNSUPPORTED       The requested language is not supported.
**/
EFI_STATUS
EFIAPI
StudyMyDriverGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

#endif
