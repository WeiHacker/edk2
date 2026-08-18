/** @file
  Homework 4 software SMI handler definitions.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HOMEWORK4_SW_SMI_SMM_H_
#define HOMEWORK4_SW_SMI_SMM_H_

#include <Uefi.h>

#define HOMEWORK4_SW_SMI_VALUE  0x88
#define HOMEWORK4_PORT80_CODE   0x88
#define HOMEWORK4_POST_PORT     0x80
#define HOMEWORK4_PM1_EN_STS    0x00

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
  );

#endif
