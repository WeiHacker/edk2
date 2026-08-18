/** @file
  Install a student-defined ACPI table.

  This driver installs a small ACPI table with the "STUD" signature. The
  table is intended for verifying that OVMF can publish a firmware-created
  ACPI table and that the operating system can discover it.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/AcpiTable.h>

#include "StudentAcpiTableDxe.h"

// 自定义的 ACPI 表实例，初始化为默认值
STATIC EFI_ACPI_STUDENT_TABLE  mStudentAcpiTable = {
  {
    EFI_ACPI_STUDENT_TABLE_SIGNATURE,
    sizeof (EFI_ACPI_STUDENT_TABLE),
    EFI_ACPI_STUDENT_TABLE_REVISION,
    0,
    { 0 },
    0,
    0,
    0,
    0
  },
  EFI_ACPI_STUDENT_TABLE_MAGIC,
  1,
  0,
  0x2026071800000001ULL
};

STATIC_ASSERT (
  sizeof (EFI_ACPI_STUDENT_TABLE) == 52,
  "EFI_ACPI_STUDENT_TABLE size must match the ACPI table layout"
  );

/**
  Entry point for the Student ACPI Table DXE driver.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  The ACPI table was installed successfully.
  @retval Others       Error returned by protocol location or table installation.
**/
EFI_STATUS
EFIAPI
StudentAcpiTableDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTable;
  EFI_STATUS               Status;
  UINTN                    TableKey;

  // 填充 ACPI 表头中的 OEM 信息
  CopyMem (
    mStudentAcpiTable.Header.OemId,
    PcdGetPtr (PcdAcpiDefaultOemId),
    sizeof (mStudentAcpiTable.Header.OemId)
    );
    // 填充 ACPI 表头中的 OEM 信息
  mStudentAcpiTable.Header.OemTableId      = PcdGet64 (PcdAcpiDefaultOemTableId);
  mStudentAcpiTable.Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  mStudentAcpiTable.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  mStudentAcpiTable.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  // 获取 EFI_ACPI_TABLE_PROTOCOL 协议接口
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
  // 安装自定义的 ACPI 表
  Status   = AcpiTable->InstallAcpiTable (
                          AcpiTable,
                          &mStudentAcpiTable,
                          sizeof (mStudentAcpiTable),
                          &TableKey
                          );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Installed STUD ACPI table\n",
    __func__
    ));

  return EFI_SUCCESS;
}
