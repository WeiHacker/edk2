/** @file
  Install an SSDT that turns OS shutdown into warm reset.

  The SSDT provides \_PTS. When the operating system prepares to enter S5,
  the method writes 0xFE to I/O port 0x64, which matches OVMF ResetWarm().

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>

#include "ShutdownWarmResetAcpiTableDxe.h"

STATIC EFI_ACPI_SHUTDOWN_WARM_RESET_TABLE  mShutdownWarmResetSsdt = {
  {
    EFI_ACPI_SHUTDOWN_WARM_RESET_TABLE_SIGNATURE,//signature
    sizeof (EFI_ACPI_SHUTDOWN_WARM_RESET_TABLE),//length
    EFI_ACPI_SHUTDOWN_WARM_RESET_TABLE_REVISION,//Reversion
    0,//checksum
    { 0 },//OEM ID
    EFI_ACPI_SHUTDOWN_WARM_RESET_OEM_TABLE_ID,//OEM Table ID
    1,//OEM Revision
    0,//Creator ID
    0//Creator Revision
  },
  {//AML code
    0x5B, 0x80, 0x57, 0x52, 0x53, 0x54, 0x01, 0x0A,
    0x64, 0x01, 0x5B, 0x81, 0x0B, 0x57, 0x52, 0x53,
    0x54, 0x01, 0x57, 0x52, 0x53, 0x43, 0x08, 0x14,
    0x13, 0x5F, 0x50, 0x54, 0x53, 0x01, 0xA0, 0x0C,
    0x93, 0x68, 0x0A, 0x05, 0x70, 0x0A, 0xFE, 0x57,
    0x52, 0x53, 0x43
  }
};

STATIC_ASSERT (
  sizeof (EFI_ACPI_SHUTDOWN_WARM_RESET_TABLE) == 79,
  "EFI_ACPI_SHUTDOWN_WARM_RESET_TABLE size must match ShutdownWarmResetSsdt.asl"
  );

/**
  Entry point for the Shutdown Warm Reset ACPI Table DXE driver.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_REQUEST_UNLOAD_IMAGE  The SSDT was installed successfully.
  @retval Others                    Error returned by protocol location or table installation.
**/
EFI_STATUS
EFIAPI
ShutdownWarmResetAcpiTableDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;
  EFI_STATUS               Status;
  UINTN                    TableKey;

  CopyMem (
    mShutdownWarmResetSsdt.Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (mShutdownWarmResetSsdt.Header.OemId)
    );
  mShutdownWarmResetSsdt.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  mShutdownWarmResetSsdt.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTable
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TableKey = 0;
  Status   = AcpiTable->InstallAcpiTable (
                          AcpiTable,
                          &mShutdownWarmResetSsdt,
                          sizeof (mShutdownWarmResetSsdt),
                          &TableKey
                          );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Installed shutdown warm reset SSDT\n",
    __func__
    ));

  return EFI_REQUEST_UNLOAD_IMAGE;
}
