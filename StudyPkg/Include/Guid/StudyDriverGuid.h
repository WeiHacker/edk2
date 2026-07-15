/** @file
  GUID definitions for StudyPkg MyDriver.

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef STUDY_DRIVER_GUID_H_
#define STUDY_DRIVER_GUID_H_

///
/// GUID installed by MyDriver on the controller handle
/// after a successful Start() to indicate ownership.
///
#define STUDY_MYDRIVER_BINDING_GUID \
  { 0xC45A8E9D, 0x5B0A, 0x4F8C, { 0x9A, 0x2E, 0x3F, 0x4D, 0x5E, 0x6A, 0x7B, 0x8D } }

extern EFI_GUID  gStudyMyDriverBindingGuid;

#endif
