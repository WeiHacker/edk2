/** @file
  MyDriver - A UEFI Driver Model example.

  This driver demonstrates the standard UEFI Driver Model by:
  - Implementing EFI_DRIVER_BINDING_PROTOCOL (Supported, Start, Stop)
  - Installing Component Name protocols for user-friendly identification
  - Binding to any handle that supports the DevicePath protocol

  When the driver is loaded, its entry point installs the driver binding
  and component name protocols into the system. The DXE dispatcher then
  invokes Supported() for each controller in the system. When a match is
  found, Start() is called to initialize the controller; Stop() tears it
  down.

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "MyDriver.h"

//
// -----------------------------------------------------------------------------
// Driver Binding Protocol instance
// -----------------------------------------------------------------------------
//
EFI_DRIVER_BINDING_PROTOCOL  gStudyMyDriverBinding = {
  StudyMyDriverSupported,
  StudyMyDriverStart,
  StudyMyDriverStop,
  0x10,                         // Version - driver version number
  NULL,                         // ImageHandle - filled by entry point
  NULL                          // DriverBindingHandle - filled by entry point
};

//
// -----------------------------------------------------------------------------
// Component Name Protocol instance
// -----------------------------------------------------------------------------
//
EFI_COMPONENT_NAME_PROTOCOL  gStudyMyDriverComponentName = {
  StudyMyDriverGetDriverName,
  StudyMyDriverGetControllerName,
  "eng"                         // Supported language: English
};

//
// -----------------------------------------------------------------------------
// Component Name 2 Protocol instance (UEFI 2.0+, uses RFC 4646 language codes)
// -----------------------------------------------------------------------------
//
EFI_COMPONENT_NAME2_PROTOCOL  gStudyMyDriverComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)    StudyMyDriverGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) StudyMyDriverGetControllerName,
  "en"                                      // Supported RFC 4646 language
};

//
// -----------------------------------------------------------------------------
// Table of driver name strings for different languages
// -----------------------------------------------------------------------------
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  mDriverNameTable[] = {
  { "eng;en", L"Study UEFI Driver (MyDriver)" },
  { NULL,     NULL                                }
};

//
// -----------------------------------------------------------------------------
// Table of controller name strings for different languages
// -----------------------------------------------------------------------------
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  mControllerNameTable[] = {
  { "eng;en", L"MyDriver-Managed Device" },
  { NULL,     NULL                       }
};

/**
  Check whether the driver supports the given controller.

  This function opens the DevicePath protocol on the controller handle.
  If the protocol exists, the driver supports the controller.

  @param[in] This                 Pointer to the EFI_DRIVER_BINDING_PROTOCOL.
  @param[in] ControllerHandle     Handle of the controller to test.
  @param[in] RemainingDevicePath  Optional pointer to the remaining device path.

  @retval EFI_SUCCESS             The driver supports the controller.
  @retval EFI_UNSUPPORTED         The driver does not support the controller.
  @retval other                   An error occurred while testing.
**/
EFI_STATUS
EFIAPI
StudyMyDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  //
  // Check whether the DevicePath protocol is present on the controller.
  // Open it BY_DRIVER to see if it is available. If it is not,
  // then this driver does not support the controller.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Immediately close the protocol. We only opened it to test for its
  // presence. The real open will happen in Start().
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return EFI_SUCCESS;
}

/**
  Start the driver on the given controller.

  This function opens the DevicePath protocol BY_DRIVER, allocates
  a private context structure, saves the DevicePath pointer, and
  installs the marker protocol to indicate that this driver is managing
  the controller.

  @param[in] This                 Pointer to the EFI_DRIVER_BINDING_PROTOCOL.
  @param[in] ControllerHandle     Handle of the controller to start.
  @param[in] RemainingDevicePath  Optional pointer to the remaining device path.

  @retval EFI_SUCCESS             The driver was successfully started.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
  @retval EFI_ALREADY_STARTED     The driver is already running on this controller.
  @retval other                   An error occurred while starting the driver.
**/
EFI_STATUS
EFIAPI
StudyMyDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                 Status;
  STUDY_MYDRIVER_PRIVATE_DATA  *Private;

  //
  // Check whether we are already started on this controller.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gStudyMyDriverProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Allocate the private context structure.
  //
  Private = AllocateZeroPool (sizeof (STUDY_MYDRIVER_PRIVATE_DATA));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature = STUDY_MYDRIVER_PRIVATE_DATA_SIGNATURE;
  Private->ControllerHandle = ControllerHandle;

  //
  // Open the DevicePath protocol on the controller for exclusive access.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&Private->DevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FREE_PRIVATE;
  }

  //
  // Install the marker protocol to indicate that we are managing this
  // controller. The protocol interface is set to the private context
  // so that Stop() can retrieve it.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gStudyMyDriverProtocolGuid,
                  (VOID *)Private,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto CLOSE_DEVICE_PATH;
  }

  DEBUG ((
    DEBUG_INFO,
    "MyDriver: Started on controller (Handle=%p)\n",
    ControllerHandle
    ));

  return EFI_SUCCESS;

CLOSE_DEVICE_PATH:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

FREE_PRIVATE:
  FreePool (Private);
  return Status;
}

/**
  Stop the driver on the given controller.

  This function uninstalls the marker protocol, closes the DevicePath
  protocol, and frees the private context structure.

  @param[in] This              Pointer to the EFI_DRIVER_BINDING_PROTOCOL.
  @param[in] ControllerHandle  Handle of the controller to stop.
  @param[in] NumberOfChildren  Number of child handles.
  @param[in] ChildHandleBuffer List of child handles to stop (unused).

  @retval EFI_SUCCESS          The driver was successfully stopped.
  @retval EFI_DEVICE_ERROR     The driver could not be stopped.
**/
EFI_STATUS
EFIAPI
StudyMyDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                 Status;
  STUDY_MYDRIVER_PRIVATE_DATA  *Private;

  //
  // Retrieve our private context from the marker protocol instance.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gStudyMyDriverProtocolGuid,
                  (VOID **)&Private,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Uninstall the marker protocol.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ControllerHandle,
                  &gStudyMyDriverProtocolGuid,
                  (VOID *)Private,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Close the DevicePath protocol that we opened in Start().
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  //
  // Free the private context structure.
  //
  FreePool (Private);

  DEBUG ((
    DEBUG_INFO,
    "MyDriver: Stopped on controller (Handle=%p)\n",
    ControllerHandle
    ));

  return EFI_SUCCESS;
}

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
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gStudyMyDriverComponentName)
           );
}

/**
  Retrieves a Unicode string that is the user-readable name of the controller
  managed by this driver.

  @param[in]  This             Pointer to the EFI_COMPONENT_NAME_PROTOCOL.
  @param[in]  ControllerHandle Handle of the controller.
  @param[in]  ChildHandle      Handle of the child (or NULL for the parent).
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
  )
{
  //
  // This driver does not manage child devices.
  //
  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }

  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mControllerNameTable,
           ControllerName,
           (BOOLEAN)(This == &gStudyMyDriverComponentName)
           );
}

/**
  The entry point for the MyDriver UEFI Driver.

  This function installs the driver binding protocol and the component
  name protocols into the system, registering this driver with the
  UEFI Driver Model.

  @param[in] ImageHandle  The firmware-allocated handle for this image.
  @param[in] SystemTable  A pointer to the EFI system table.

  @retval EFI_SUCCESS           The entry point executed successfully.
  @retval EFI_OUT_OF_RESOURCES  Insufficient resources to install protocols.
  @retval other                 An error occurred during installation.
**/
EFI_STATUS
EFIAPI
StudyMyDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install the Driver Binding Protocol and Component Name Protocols.
  // This registers the driver with the UEFI Driver Model dispatcher.
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gStudyMyDriverBinding,
             ImageHandle,
             &gStudyMyDriverComponentName,
             &gStudyMyDriverComponentName2
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "MyDriver: Failed to install driver binding protocols: %r\n",
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_INIT,
    "MyDriver: Successfully loaded. Version 0.1\n"
    ));

  return EFI_SUCCESS;
}
