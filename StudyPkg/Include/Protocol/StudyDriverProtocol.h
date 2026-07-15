/** @file
  Protocol definition for StudyPkg MyDriver.

  This protocol is installed by MyDriver on the controller handle
  to mark that the driver has successfully bound to that controller.

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef STUDY_DRIVER_PROTOCOL_H_
#define STUDY_DRIVER_PROTOCOL_H_

///
/// Protocol GUID for MyDriver.
/// This protocol has no methods; it is simply a marker protocol
/// indicating that MyDriver is managing the controller.
///
#define STUDY_MYDRIVER_PROTOCOL_GUID \
  { 0xD56B9F0E, 0x6A1B, 0x4F8D, { 0x0A, 0x3E, 0x4F, 0x5D, 0x6E, 0x7A, 0x8B, 0x9E } }

extern EFI_GUID  gStudyMyDriverProtocolGuid;

#endif
