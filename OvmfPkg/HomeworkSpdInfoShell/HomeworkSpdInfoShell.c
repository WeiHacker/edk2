/** @file
  Read and print DDR4 SPD information through the UEFI I2C Master Protocol.

  Copyright (c) 2026, Student. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "HomeworkSpdInfoShell.h"

/**
  Read one byte from an SPD EEPROM.

  The first operation writes the SPD offset. The repeated-start operation then
  reads one byte from that offset.

  @param[in]  I2cMaster  Pointer to the I2C Master Protocol.
  @param[in]  Address    Seven-bit SPD slave address.
  @param[in]  Offset     SPD byte offset.
  @param[out] Data       Returned SPD byte.

  @retval EFI_SUCCESS  The byte was read.
  @retval Others       The I2C transaction failed.
**/
STATIC
EFI_STATUS
ReadSpdByte (
  IN  EFI_I2C_MASTER_PROTOCOL  *I2cMaster,
  IN  UINT8                    Address,
  IN  UINT8                    Offset,
  OUT UINT8                    *Data
  )
{
  HOMEWORK_I2C_REQUEST_PACKET  Request;

  Request.OperationCount             = 2;
  Request.Operation[0].Flags         = 0;
  Request.Operation[0].LengthInBytes = 1;
  Request.Operation[0].Buffer        = &Offset;
  Request.Operation[1].Flags         = I2C_FLAG_READ;
  Request.Operation[1].LengthInBytes = 1;
  Request.Operation[1].Buffer        = Data;

  return I2cMaster->StartRequest (
                      I2cMaster,
                      Address,
                      (EFI_I2C_REQUEST_PACKET *)&Request,
                      NULL,
                      NULL
                      );
}

/**
  Select DDR4 SPD page zero or page one.

  @param[in] I2cMaster   Pointer to the I2C Master Protocol.
  @param[in] PageAddress DDR4 SPD page-select slave address.

  @retval EFI_SUCCESS  The page was selected.
  @retval Others       The I2C transaction failed.
**/
STATIC
EFI_STATUS
SelectSpdPage (
  IN EFI_I2C_MASTER_PROTOCOL  *I2cMaster,
  IN UINT8                    PageAddress
  )
{
  EFI_I2C_REQUEST_PACKET  Request;
  UINT8                   Data;

  Data                               = 0;
  Request.OperationCount             = 1;
  Request.Operation[0].Flags         = 0;
  Request.Operation[0].LengthInBytes = 1;
  Request.Operation[0].Buffer        = &Data;

  return I2cMaster->StartRequest (
                      I2cMaster,
                      PageAddress,
                      &Request,
                      NULL,
                      NULL
                      );
}

/**
  Return a common memory manufacturer name.

  @param[in] Code  JEP106 manufacturer code.

  @return Manufacturer name.
**/
STATIC
CONST CHAR16 *
GetManufacturerName (
  IN UINT8  Code
  )
{
  switch (Code) {
    case 0x2C:
      return L"Micron";
    case 0xAD:
      return L"SK Hynix";
    case 0xCE:
      return L"Samsung";
    case 0x98:
      return L"Kingston";
    default:
      return L"Unknown";
  }
}

/**
  Read and print one DDR4 SPD device.

  @param[in] I2cMaster  Pointer to the I2C Master Protocol.
  @param[in] Address    Seven-bit SPD slave address.
  @param[in] Slot       Logical SPD slot number.

  @retval EFI_SUCCESS      The SPD information was printed.
  @retval EFI_UNSUPPORTED  The SPD organization is invalid.
  @retval Others           Reading the SPD failed.
**/
STATIC
EFI_STATUS
PrintDdr4Spd (
  IN EFI_I2C_MASTER_PROTOCOL  *I2cMaster,
  IN UINT8                    Address,
  IN UINTN                    Slot
  )
{
  EFI_STATUS    Status;
  CONST CHAR16  *ModuleType;
  CONST CHAR16  *RankName;
  UINT8         BusWidthCode;
  UINT8         DensityCode;
  UINT8         DeviceWidth;
  UINT8         DeviceWidthCode;
  UINT8         Manufacturer;
  UINT8         RankCount;
  UINT8         Spd[HOMEWORK_SPD_BASE_SIZE];
  UINT32        BusWidth;
  UINT32        CapacityMb;
  UINT32        DataRate;
  UINT32        DensityMb;
  INT32         TckPs;
  UINTN         Index;

  for (Index = 0; Index < HOMEWORK_SPD_BASE_SIZE; Index++) {
    Status = ReadSpdByte (I2cMaster, Address, (UINT8)Index, &Spd[Index]);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  DensityCode     = Spd[4] & 0x0F;
  DeviceWidthCode = Spd[12] & 0x07;
  BusWidthCode    = Spd[13] & 0x07;
  if ((DensityCode > 7) || (DeviceWidthCode > 3) || (BusWidthCode > 3)) {
    return EFI_UNSUPPORTED;
  }

  DensityMb   = 256U << DensityCode;
  DeviceWidth = (UINT8)(4U << DeviceWidthCode);
  RankCount   = (UINT8)(((Spd[12] >> 3) & 0x07) + 1);
  BusWidth    = 8U << BusWidthCode;
  CapacityMb  = (DensityMb / 8) * (BusWidth / DeviceWidth) * RankCount;

  TckPs    = ((INT32)Spd[18] * 125) + (INT8)Spd[125];
  DataRate = (TckPs > 0) ? (UINT32)((2000000 + (TckPs / 2)) / TckPs) : 0;

  RankName = L"?R";
  if (RankCount == 1) {
    RankName = L"SR";
  } else if (RankCount == 2) {
    RankName = L"DR";
  } else if (RankCount == 4) {
    RankName = L"QR";
  }

  switch (Spd[3] & 0x0F) {
    case 1:
      ModuleType = L"RDIMM";
      break;
    case 2:
      ModuleType = L"UDIMM";
      break;
    case 3:
      ModuleType = L"SO-DIMM";
      break;
    case 4:
      ModuleType = L"LRDIMM";
      break;
    default:
      ModuleType = L"Unknown";
      break;
  }

  Manufacturer = 0;
  Status       = SelectSpdPage (I2cMaster, HOMEWORK_SPD_PAGE_ONE);
  if (!EFI_ERROR (Status)) {
    ReadSpdByte (I2cMaster, Address, 65, &Manufacturer);
    Status = SelectSpdPage (I2cMaster, HOMEWORK_SPD_PAGE_ZERO);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Print (
    L"SPD%u@0x%02x: %uMT/s %s %sx%u %uGB %s\n",
    Slot,
    (UINTN)Address,
    (UINTN)DataRate,
    GetManufacturerName (Manufacturer),
    RankName,
    (UINTN)DeviceWidth,
    (UINTN)(CapacityMb / 1024),
    ModuleType
    );
  return EFI_SUCCESS;
}

/**
  UEFI application entry point.

  @param[in] ImageHandle  Handle for this image.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS    The SPD scan completed.
  @retval EFI_NOT_FOUND  The I2C Master Protocol was not found.
  @retval Others         I2C controller initialization failed.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_I2C_MASTER_PROTOCOL  *I2cMaster;
  EFI_STATUS               Status;
  UINT8                    Address;
  UINT8                    MemoryType;
  UINTN                    BusClockHertz;
  UINTN                    Found;

  (VOID)ImageHandle;
  (VOID)SystemTable;

  Status = gBS->LocateProtocol (
                  &gEfiI2cMasterProtocolGuid,
                  NULL,
                  (VOID **)&I2cMaster
                  );
  if (EFI_ERROR (Status)) {
    Print (L"I2C Master Protocol was not found: %r\n", Status);
    return Status;
  }

  Status = I2cMaster->Reset (I2cMaster);
  if (EFI_ERROR (Status)) {
    Print (L"Failed to reset I2C controller: %r\n", Status);
    return Status;
  }

  BusClockHertz = 100000;
  Status        = I2cMaster->SetBusFrequency (I2cMaster, &BusClockHertz);
  if (EFI_ERROR (Status)) {
    Print (L"Failed to set I2C bus frequency: %r\n", Status);
    return Status;
  }

  Print (L"------------------------------------------------------------------\n");
  Found = 0;
  Status = SelectSpdPage (I2cMaster, HOMEWORK_SPD_PAGE_ZERO);
  if (EFI_ERROR (Status)) {
    Print (L"Failed to select DDR4 SPD page zero: %r\n", Status);
    return Status;
  }

  for (Address = HOMEWORK_SPD_FIRST_ADDRESS;
       Address <= HOMEWORK_SPD_LAST_ADDRESS;
       Address++)
  {
    Status = ReadSpdByte (I2cMaster, Address, 2, &MemoryType);
    if (EFI_ERROR (Status) || (MemoryType != HOMEWORK_SPD_DDR4_TYPE)) {
      continue;
    }

    Status = PrintDdr4Spd (I2cMaster, Address, Found);
    if (!EFI_ERROR (Status)) {
      Found++;
    }
  }

  if (Found == 0) {
    Print (L"No DDR4 SPD EEPROM was found at address 0x50-0x57.\n");
  }

  Print (L"------------------------------------------------------------------\n");
  return EFI_SUCCESS;
}
