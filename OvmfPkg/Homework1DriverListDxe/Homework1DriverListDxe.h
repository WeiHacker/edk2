/** @file
  Homework 1.1 UEFI driver list dump definitions.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef HOMEWORK1_DRIVER_LIST_DXE_H_
#define HOMEWORK1_DRIVER_LIST_DXE_H_

#include <Uefi.h>

/**
  Dump the UEFI driver list from Driver Binding handles.
**/
VOID
DumpUefiDriverList (
  VOID
  );

#endif
