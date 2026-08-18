/** @file
  Add a homework-specific OEM SMBIOS structure.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Smbios.h>

#include "HomeworkSmbios.h"

/**
  Add OEM Type 0x80 through the SMBIOS Protocol.

  @param[in] ImageHandle  Handle for this driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The OEM type was added.
  @retval Others       Locating the protocol or adding the record failed.
**/
EFI_STATUS
EFIAPI
HomeworkSmbiosDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  HOMEWORK_SMBIOS_OEM_RECORD  Record = {
    {
      // SMBIOS_STRUCTURE,所有Type共有的头字段结构体
      {
        // 自定义type的type value：0x80
        HOMEWORK_SMBIOS_OEM_TYPE_VALUE,
        // 整个自定义type的长度，包含头字段和自定义字段
        sizeof (HOMEWORK_SMBIOS_OEM_TYPE),
        // 由SMBIOS协议分配的handle值
        0
      },
      // 自定义type的签名字段：HW80
      HOMEWORK_SMBIOS_OEM_SIGNATURE,
      // 自定义type的版本号
      1,
      // 自定义type的次版本号
      0,
      // 自定义type的状态字段
      1,
      // 自定义type的特性标志字段
      0x00000003,
      // 自定义type的自定义数据字段
      0x12345678
    },
    // 自定义type的字符串集结束标志，两个连续的0字节表示字符串集结束（遍历字符串集时遇到两个连续的0字节表示结束）
    { 0, 0 }
  };
  // 通过SMBIOS的Handle以及GUID找到对应的Protocol接口
  EFI_SMBIOS_HANDLE          SmbiosHandle;
  // 把找到的Protocol接口赋值给Smbios指针变量
  EFI_SMBIOS_PROTOCOL        *Smbios;
  EFI_STATUS                 Status;

  (VOID)SystemTable;

  // 通过LocateProtocol函数找到SMBIOS协议接口，并将其赋值给Smbios指针变量
  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&Smbios
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // SMBIOS_HANDLE_PI_RESERVED 表示请求 EFI_SMBIOS_PROTOCOL 自动分配一个当前未使用的 Handle，这里注意最终分配的Handle值不是SMBIOS_HANDLE_PI_RESERVED，而是由SMBIOS协议分配的一个唯一值
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  // 通过SMBIOS协议的Add函数将自定义的OEM类型记录添加到SMBIOS表中
  Status       = Smbios->Add (
                           Smbios,
                           ImageHandle,
                           &SmbiosHandle,
                           (EFI_SMBIOS_TABLE_HEADER *)&Record
                           );
  DEBUG ((
    DEBUG_INFO,
    "HomeworkSmbiosDxe: Type 0x80, Handle=0x%04x, Status=%r\n",
    (UINTN)SmbiosHandle,
    Status
    ));
  return Status;
}
