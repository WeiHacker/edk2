/** @file
  Homework 1.1 UEFI driver list dump DXE driver.

  This driver waits until ReadyToBoot, then enumerates handles that support
  EFI_DRIVER_BINDING_PROTOCOL and prints driver name, version, and image path.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Guid/EventGroup.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/LoadedImage.h>

#include "Homework1DriverListDxe.h"

STATIC BOOLEAN  mDriverListDumped;

/**
  Get the driver display name from Component Name protocols.

  @param[in] DriverBindingHandle  Handle that installs Driver Binding Protocol.

  @return Driver name string, or NULL if no driver name is available.
**/
STATIC
CHAR16 *
GetDriverName (
  IN EFI_HANDLE  DriverBindingHandle
  )
{
  EFI_STATUS                    Status;
  EFI_COMPONENT_NAME_PROTOCOL   *ComponentName;
  EFI_COMPONENT_NAME2_PROTOCOL  *ComponentName2;
  CHAR16                        *DriverName;

  DriverName = NULL;

  Status = gBS->HandleProtocol (
                  DriverBindingHandle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **)&ComponentName2
                  );
  if (!EFI_ERROR (Status)) {
    Status = ComponentName2->GetDriverName (ComponentName2, "en", &DriverName);
    if (!EFI_ERROR (Status)) {
      return DriverName;
    }
  }

  Status = gBS->HandleProtocol (
                  DriverBindingHandle,
                  &gEfiComponentNameProtocolGuid,
                  (VOID **)&ComponentName
                  );
  if (!EFI_ERROR (Status)) {
    Status = ComponentName->GetDriverName (ComponentName, "eng", &DriverName);
    if (!EFI_ERROR (Status)) {
      return DriverName;
    }
  }

  return NULL;
}

/**
  Get the loaded image path for a Driver Binding image handle.

  @param[in] DriverBinding  Pointer to EFI_DRIVER_BINDING_PROTOCOL.

  @return Allocated image path string, or NULL if no path is available.
**/
STATIC
CHAR16 *
GetDriverImagePath (
  IN EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;

  if (DriverBinding->ImageHandle == NULL) {
    return NULL;
  }

  Status = gBS->HandleProtocol (
                  DriverBinding->ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  if (EFI_ERROR (Status) || (LoadedImage->FilePath == NULL)) {
    return NULL;
  }

  return ConvertDevicePathToText (LoadedImage->FilePath, FALSE, FALSE);
}

/**
  Dump the UEFI driver list from Driver Binding handles.
**/
VOID
DumpUefiDriverList (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_HANDLE                   *DriverHandleBuffer;
  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding;
  UINTN                        DriverHandleCount;
  UINTN                        Index;
  CHAR16                       *DriverName;
  CHAR16                       *ImagePath;

  DriverHandleBuffer = NULL;
  DriverHandleCount  = 0;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDriverBindingProtocolGuid,
                  NULL,
                  &DriverHandleCount,
                  &DriverHandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Homework1DriverListDxe: LocateHandleBuffer failed: %r\n", Status));
    return;
  }

  DEBUG ((DEBUG_INFO, "Homework1DriverListDxe: UEFI driver list begin, count=%u\n", (UINT32)DriverHandleCount));

  for (Index = 0; Index < DriverHandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    DriverHandleBuffer[Index],
                    &gEfiDriverBindingProtocolGuid,
                    (VOID **)&DriverBinding
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    DriverName = GetDriverName (DriverHandleBuffer[Index]);
    ImagePath  = GetDriverImagePath (DriverBinding);

    DEBUG ((
      DEBUG_INFO,
      "Homework1DriverListDxe: [%03u] Handle=%p Version=0x%08x Name=%s\n",
      (UINT32)Index,
      DriverHandleBuffer[Index],
      DriverBinding->Version,
      DriverName != NULL ? DriverName : L"<unknown>"
      ));
    DEBUG ((
      DEBUG_INFO,
      "Homework1DriverListDxe:       Image=%s\n",
      ImagePath != NULL ? ImagePath : L"<unknown>"
      ));

    if (ImagePath != NULL) {
      FreePool (ImagePath);
    }
  }

  DEBUG ((DEBUG_INFO, "Homework1DriverListDxe: UEFI driver list end\n"));

  if (DriverHandleBuffer != NULL) {
    FreePool (DriverHandleBuffer);
  }
}

/**
  ReadyToBoot event callback.

  @param[in] Event    ReadyToBoot event.
  @param[in] Context  Optional context.
**/
STATIC
VOID
EFIAPI
Homework1DriverListReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  (VOID)Context;

  if (mDriverListDumped) {
    return;
  }

  mDriverListDumped = TRUE;
  DumpUefiDriverList ();
  gBS->CloseEvent (Event);
}

/**
  Entry point for Homework 1.1 driver list DXE driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  ReadyToBoot callback was registered.
  @retval Others       Error returned by CreateEventEx().
**/
EFI_STATUS
EFIAPI
Homework1DriverListDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   ReadyToBootEvent;

  (VOID)ImageHandle;
  (VOID)SystemTable;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  Homework1DriverListReadyToBoot,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &ReadyToBootEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Homework1DriverListDxe: CreateEventEx failed: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Homework1DriverListDxe: ReadyToBoot callback registered\n"));
  return EFI_SUCCESS;
}
