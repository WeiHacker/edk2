/** @file
  Homework 1.2 boot device path dump DXE driver.

  This driver waits until ReadyToBoot, then reads BootOrder and Boot#### UEFI
  variables, parses each EFI_LOAD_OPTION, and prints the boot device path.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/EventGroup.h>
#include <Guid/GlobalVariable.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "Homework12BootDevicePathDxe.h"

STATIC BOOLEAN  mBootDevicePathDumped;

/**
  Read a UEFI global variable into an allocated buffer.

  @param[in]  VariableName  Name of the UEFI global variable.
  @param[out] DataSize      Size of the returned data buffer.
  @param[out] Data          Allocated data buffer.

  @retval EFI_SUCCESS  The variable was read.
  @retval Others       Error returned by GetVariable() or AllocatePool().
**/
STATIC
EFI_STATUS
ReadGlobalVariable (
  IN  CHAR16  *VariableName,
  OUT UINTN   *DataSize,
  OUT VOID    **Data
  )
{
  EFI_STATUS  Status;

  *DataSize = 0;
  *Data     = NULL;

  Status = gRT->GetVariable (
                  VariableName,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  DataSize,
                  NULL
                  );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  *Data = AllocatePool (*DataSize);
  if (*Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gRT->GetVariable (
                  VariableName,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  DataSize,
                  *Data
                  );
  if (EFI_ERROR (Status)) {
    FreePool (*Data);
    *Data = NULL;
  }

  return Status;
}

/**
  Return the size of a null-terminated CHAR16 string inside a bounded buffer.

  @param[in]  String      Start of the string.
  @param[in]  BufferSize  Maximum number of bytes available.
  @param[out] StringSize  Size in bytes including the null terminator.

  @retval TRUE   A null terminator was found.
  @retval FALSE  The string is not valid in the supplied buffer.
**/
STATIC
BOOLEAN
GetBoundedStringSize (
  IN  CONST CHAR16  *String,
  IN  UINTN         BufferSize,
  OUT UINTN         *StringSize
  )
{
  UINTN  Index;
  UINTN  CharCount;

  CharCount = BufferSize / sizeof (CHAR16);
  for (Index = 0; Index < CharCount; Index++) {
    if (String[Index] == L'\0') {
      *StringSize = (Index + 1) * sizeof (CHAR16);
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Dump one Boot#### option's device path.

  @param[in] BootNumber  Boot option number.
**/
STATIC
VOID
DumpOneBootOption (
  IN UINT16  BootNumber
  )
{
  EFI_STATUS                Status;
  CHAR16                    VariableName[sizeof (L"Boot0000") / sizeof (CHAR16)];
  EFI_LOAD_OPTION           *LoadOption;
  EFI_DEVICE_PATH_PROTOCOL  *FilePath;
  EFI_DEVICE_PATH_PROTOCOL  *AlignedFilePath;
  UINTN                     LoadOptionSize;
  UINTN                     DescriptionSize;
  UINTN                     FilePathOffset;
  CHAR16                    *DevicePathText;
  VOID                      *LoadOptionBuffer;

  UnicodeSPrint (VariableName, sizeof (VariableName), L"Boot%04x", BootNumber);

  Status = ReadGlobalVariable (VariableName, &LoadOptionSize, &LoadOptionBuffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Homework12BootDevicePathDxe: %s read failed: %r\n", VariableName, Status));
    return;
  }

  if (LoadOptionSize < sizeof (EFI_LOAD_OPTION)) {
    DEBUG ((DEBUG_INFO, "Homework12BootDevicePathDxe: %s is too small\n", VariableName));
    FreePool (LoadOptionBuffer);
    return;
  }

  LoadOption = (EFI_LOAD_OPTION *)LoadOptionBuffer;
  if (!GetBoundedStringSize (
         (CHAR16 *)((UINT8 *)LoadOption + sizeof (UINT32) + sizeof (UINT16)),
         LoadOptionSize - sizeof (UINT32) - sizeof (UINT16),
         &DescriptionSize
         ))
  {
    DEBUG ((DEBUG_INFO, "Homework12BootDevicePathDxe: %s description is invalid\n", VariableName));
    FreePool (LoadOptionBuffer);
    return;
  }

  FilePathOffset = sizeof (UINT32) + sizeof (UINT16) + DescriptionSize;
  if ((FilePathOffset > LoadOptionSize) ||
      (LoadOption->FilePathListLength > LoadOptionSize - FilePathOffset))
  {
    DEBUG ((DEBUG_INFO, "Homework12BootDevicePathDxe: %s device path is invalid\n", VariableName));
    FreePool (LoadOptionBuffer);
    return;
  }

  FilePath        = (EFI_DEVICE_PATH_PROTOCOL *)((UINT8 *)LoadOption + FilePathOffset);
  AlignedFilePath = AllocateCopyPool (LoadOption->FilePathListLength, FilePath);
  if (AlignedFilePath == NULL) {
    FreePool (LoadOptionBuffer);
    return;
  }

  DevicePathText = ConvertDevicePathToText (AlignedFilePath, FALSE, FALSE);

  DEBUG ((
    DEBUG_INFO,
    "Homework12BootDevicePathDxe: %s Attr=0x%08x Desc=%s\n",
    VariableName,
    LoadOption->Attributes,
    (CHAR16 *)((UINT8 *)LoadOption + sizeof (UINT32) + sizeof (UINT16))
    ));
  DEBUG ((
    DEBUG_INFO,
    "Homework12BootDevicePathDxe:       DevicePath=%s\n",
    DevicePathText != NULL ? DevicePathText : L"<unknown>"
    ));

  if (DevicePathText != NULL) {
    FreePool (DevicePathText);
  }

  FreePool (AlignedFilePath);
  FreePool (LoadOptionBuffer);
}

/**
  Dump BootOrder device paths from UEFI boot option variables.
**/
VOID
DumpBootDevicePaths (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT16      *BootOrder;
  UINTN       BootOrderSize;
  UINTN       BootCount;
  UINTN       Index;
  VOID        *BootOrderBuffer;

  Status = ReadGlobalVariable (L"BootOrder", &BootOrderSize, &BootOrderBuffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Homework12BootDevicePathDxe: BootOrder read failed: %r\n", Status));
    return;
  }

  if ((BootOrderSize == 0) || ((BootOrderSize % sizeof (UINT16)) != 0)) {
    DEBUG ((DEBUG_INFO, "Homework12BootDevicePathDxe: BootOrder size is invalid\n"));
    FreePool (BootOrderBuffer);
    return;
  }

  BootOrder = (UINT16 *)BootOrderBuffer;
  BootCount = BootOrderSize / sizeof (UINT16);

  DEBUG ((DEBUG_INFO, "Homework12BootDevicePathDxe: boot device path list begin, count=%u\n", (UINT32)BootCount));

  for (Index = 0; Index < BootCount; Index++) {
    DumpOneBootOption (BootOrder[Index]);
  }

  DEBUG ((DEBUG_INFO, "Homework12BootDevicePathDxe: boot device path list end\n"));

  FreePool (BootOrderBuffer);
}

/**
  ReadyToBoot event callback.

  @param[in] Event    ReadyToBoot event.
  @param[in] Context  Optional context.
**/
STATIC
VOID
EFIAPI
Homework12BootDevicePathReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  (VOID)Context;

  if (mBootDevicePathDumped) {
    return;
  }

  mBootDevicePathDumped = TRUE;
  DumpBootDevicePaths ();
  gBS->CloseEvent (Event);
}

/**
  Entry point for Homework 1.2 boot device path DXE driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  ReadyToBoot callback was registered.
  @retval Others       Error returned by CreateEventEx().
**/
EFI_STATUS
EFIAPI
Homework12BootDevicePathDxeEntryPoint (
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
                  Homework12BootDevicePathReadyToBoot,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &ReadyToBootEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Homework12BootDevicePathDxe: CreateEventEx failed: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "Homework12BootDevicePathDxe: ReadyToBoot callback registered\n"));
  return EFI_SUCCESS;
}
