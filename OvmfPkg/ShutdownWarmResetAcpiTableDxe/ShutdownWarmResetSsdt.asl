/** @file
  SSDT for turning OS shutdown into warm reset.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

DefinitionBlock ("ShutdownWarmResetSsdt.aml", "SSDT", 2, "OVMF  ", "SHDWRST ", 0x00000001)
{
    OperationRegion (WRST, SystemIO, 0x64, One)
    Field (WRST, ByteAcc, NoLock, Preserve)
    {
        WRSC,   8
    }

    Method (_PTS, 1, NotSerialized)
    {
        If ((Arg0 == 0x05))
        {
            WRSC = 0xFE
        }
    }
}
