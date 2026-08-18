/** @file
  Produce the UEFI I2C Master Protocol for the Q35 ICH9 controller.

  The ICH9 controller supports the two transactions required by the DDR4 SPD
  shell application: byte-data read and send-byte write.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "HomeworkI2cMasterDxe.h"

STATIC UINT16  mSmbusBase;

/**
  Wait for the current controller transaction.

  @retval EFI_SUCCESS       The transaction completed.
  @retval EFI_DEVICE_ERROR  The controller reported an error.
  @retval EFI_TIMEOUT       The transaction timed out.
**/
STATIC
EFI_STATUS
WaitForTransaction (
  VOID
  )
{
  UINT8  HostStatus;
  UINTN  Retry;

  for (Retry = 0; Retry < 10000; Retry++) {
    HostStatus = IoRead8 (mSmbusBase + HOMEWORK_SMB_HSTS);
    if ((HostStatus & HOMEWORK_SMB_HSTS_ERROR) != 0) {
      IoWrite8 (mSmbusBase + HOMEWORK_SMB_HSTS, HostStatus);
      return EFI_DEVICE_ERROR;
    }

    if ((HostStatus & HOMEWORK_SMB_HSTS_INTR) != 0) {
      IoWrite8 (mSmbusBase + HOMEWORK_SMB_HSTS, HostStatus);
      return EFI_SUCCESS;
    }

    MicroSecondDelay (10);
  }

  return EFI_TIMEOUT;
}

/**
  Read one byte by using the controller byte-data transaction.

  @param[in]  Address  Seven-bit I2C slave address.
  @param[in]  Offset   Device byte offset.
  @param[out] Data     Returned byte.

  @retval EFI_SUCCESS  The byte was read.
  @retval Others       The transaction failed.
**/
STATIC
EFI_STATUS
ReadByte (
  IN  UINT8  Address,
  IN  UINT8  Offset,
  OUT UINT8  *Data
  )
{
  EFI_STATUS  Status;

  if ((IoRead8 (mSmbusBase + HOMEWORK_SMB_HSTS) & HOMEWORK_SMB_HSTS_BUSY) != 0) {
    return EFI_ALREADY_STARTED;
  }

  IoWrite8 (mSmbusBase + HOMEWORK_SMB_HSTS, 0xFF);
  IoWrite8 (mSmbusBase + HOMEWORK_SMB_HST_CMD, Offset);
  IoWrite8 (mSmbusBase + HOMEWORK_SMB_XMIT_SLVA, (UINT8)((Address << 1) | 1));
  IoWrite8 (
    mSmbusBase + HOMEWORK_SMB_HST_CNT,
    HOMEWORK_SMB_START | HOMEWORK_SMB_BYTE_DATA
    );

  Status = WaitForTransaction ();
  if (!EFI_ERROR (Status)) {
    *Data = IoRead8 (mSmbusBase + HOMEWORK_SMB_HST_D0);
  }

  return Status;
}

/**
  Write one byte by using the controller send-byte transaction.

  @param[in] Address  Seven-bit I2C slave address.
  @param[in] Data     Byte to write.

  @retval EFI_SUCCESS  The byte was written.
  @retval Others       The transaction failed.
**/
STATIC
EFI_STATUS
WriteByte (
  IN UINT8  Address,
  IN UINT8  Data
  )
{
  if ((IoRead8 (mSmbusBase + HOMEWORK_SMB_HSTS) & HOMEWORK_SMB_HSTS_BUSY) != 0) {
    return EFI_ALREADY_STARTED;
  }

  IoWrite8 (mSmbusBase + HOMEWORK_SMB_HSTS, 0xFF);
  IoWrite8 (mSmbusBase + HOMEWORK_SMB_HST_CMD, Data);
  IoWrite8 (mSmbusBase + HOMEWORK_SMB_XMIT_SLVA, (UINT8)(Address << 1));
  IoWrite8 (
    mSmbusBase + HOMEWORK_SMB_HST_CNT,
    HOMEWORK_SMB_START | HOMEWORK_SMB_BYTE
    );
  return WaitForTransaction ();
}

/**
  Set the I2C bus frequency.

  @param[in]     This           Pointer to the I2C Master Protocol.
  @param[in,out] BusClockHertz  Requested and returned bus frequency.

  @retval EFI_SUCCESS            The fixed 100 kHz frequency was selected.
  @retval EFI_INVALID_PARAMETER  BusClockHertz is NULL.
  @retval EFI_UNSUPPORTED        The requested frequency is below 100 kHz.
**/
STATIC
EFI_STATUS
EFIAPI
SetBusFrequency (
  IN CONST EFI_I2C_MASTER_PROTOCOL  *This,
  IN OUT UINTN                      *BusClockHertz
  )
{
  (VOID)This;

  if (BusClockHertz == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*BusClockHertz < HOMEWORK_I2C_BUS_FREQUENCY) {
    return EFI_UNSUPPORTED;
  }

  *BusClockHertz = HOMEWORK_I2C_BUS_FREQUENCY;
  return EFI_SUCCESS;
}

/**
  Reset the I2C controller.

  @param[in] This  Pointer to the I2C Master Protocol.

  @retval EFI_SUCCESS          The status bits were cleared.
  @retval EFI_ALREADY_STARTED  The controller is busy.
**/
STATIC
EFI_STATUS
EFIAPI
Reset (
  IN CONST EFI_I2C_MASTER_PROTOCOL  *This
  )
{
  (VOID)This;

  if ((IoRead8 (mSmbusBase + HOMEWORK_SMB_HSTS) & HOMEWORK_SMB_HSTS_BUSY) != 0) {
    return EFI_ALREADY_STARTED;
  }

  IoWrite8 (mSmbusBase + HOMEWORK_SMB_HSTS, 0xFF);
  return EFI_SUCCESS;
}

/**
  Start a synchronous I2C request.

  @param[in]  This           Pointer to the I2C Master Protocol.
  @param[in]  SlaveAddress   Seven-bit I2C slave address.
  @param[in]  RequestPacket  I2C request packet.
  @param[in]  Event          Optional asynchronous event.
  @param[out] I2cStatus      Optional transaction status.

  @retval EFI_SUCCESS            The transaction completed.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_UNSUPPORTED        The request format is not supported.
  @retval Others                 The controller transaction failed.
**/
STATIC
EFI_STATUS
EFIAPI
StartRequest (
  IN CONST EFI_I2C_MASTER_PROTOCOL  *This,
  IN UINTN                          SlaveAddress,
  IN EFI_I2C_REQUEST_PACKET         *RequestPacket,
  IN EFI_EVENT                      Event      OPTIONAL,
  OUT EFI_STATUS                    *I2cStatus OPTIONAL
  )
{
  EFI_STATUS  Status;

  (VOID)This;

  if ((RequestPacket == NULL) || (SlaveAddress > 0x7F)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Event != NULL) {
    return EFI_UNSUPPORTED;
  }

  if ((RequestPacket->OperationCount == 2) &&
      (RequestPacket->Operation[0].Flags == 0) &&
      (RequestPacket->Operation[0].LengthInBytes == 1) &&
      (RequestPacket->Operation[0].Buffer != NULL) &&
      (RequestPacket->Operation[1].Flags == I2C_FLAG_READ) &&
      (RequestPacket->Operation[1].LengthInBytes == 1) &&
      (RequestPacket->Operation[1].Buffer != NULL))
  {
    Status = ReadByte (
               (UINT8)SlaveAddress,
               RequestPacket->Operation[0].Buffer[0],
               RequestPacket->Operation[1].Buffer
               );
  } else if ((RequestPacket->OperationCount == 1) &&
             (RequestPacket->Operation[0].Flags == 0) &&
             (RequestPacket->Operation[0].LengthInBytes == 1) &&
             (RequestPacket->Operation[0].Buffer != NULL))
  {
    Status = WriteByte (
               (UINT8)SlaveAddress,
               RequestPacket->Operation[0].Buffer[0]
               );
  } else {
    Status = EFI_UNSUPPORTED;
  }

  if (I2cStatus != NULL) {
    *I2cStatus = Status;
  }

  return Status;
}

STATIC EFI_I2C_CONTROLLER_CAPABILITIES  mI2cCapabilities = {
  sizeof (EFI_I2C_CONTROLLER_CAPABILITIES),
  1,
  1,
  2
};

STATIC EFI_I2C_MASTER_PROTOCOL  mI2cMaster = {
  SetBusFrequency,
  Reset,
  StartRequest,
  &mI2cCapabilities
};

/**
  Install the I2C Master Protocol for the Q35 ICH9 controller.

  @param[in] ImageHandle  Handle for this driver.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS      The protocol was installed.
  @retval EFI_UNSUPPORTED  The Q35 ICH9 controller was not found.
  @retval Others           Protocol installation failed.
**/
EFI_STATUS
EFIAPI
HomeworkI2cMasterDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HANDLE  Handle;
  UINTN       PciAddress;

  (VOID)ImageHandle;
  (VOID)SystemTable;

  PciAddress = HOMEWORK_ICH9_SMBUS_PCI_ADDRESS;
  if ((PciRead16 (PciAddress) != 0x8086) ||
      (PciRead8 (PciAddress + 0x0B) != 0x0C) ||
      (PciRead8 (PciAddress + 0x0A) != 0x05))
  {
    return EFI_UNSUPPORTED;
  }

  mSmbusBase = (UINT16)(PciRead32 (PciAddress + HOMEWORK_ICH9_SMBUS_BASE) & 0xFFE0);
  if (mSmbusBase == 0) {
    return EFI_UNSUPPORTED;
  }

  PciOr16 (PciAddress + 0x04, BIT0);
  PciOr8 (PciAddress + HOMEWORK_ICH9_SMBUS_HOSTC, BIT0);

  Handle = NULL;
  DEBUG ((
    DEBUG_INFO,
    "HomeworkI2cMasterDxe: I2C Master installed at 0x%04x\n",
    (UINTN)mSmbusBase
    ));
  return gBS->InstallMultipleProtocolInterfaces (
                &Handle,
                &gEfiI2cMasterProtocolGuid,
                &mI2cMaster,
                NULL
                );
}
