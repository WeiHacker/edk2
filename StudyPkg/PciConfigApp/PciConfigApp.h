/** @file
  Header file for PciConfigApp - a UEFI Shell application.

  This application scans the PCI/PCIe bus and displays device information.
  It uses direct I/O port access (0xCF8/0xCFC) to read PCI configuration
  space, without using any UEFI protocol services.

  Features:
  - Without BDF argument: enumerates all PCI/PCIe devices (VID & DID)
  - With BDF argument (B:D:F): dumps 256-byte config space, determines
    Type 0/1 and PCI/PCIe nature

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef PCI_CONFIG_APP_H_
#define PCI_CONFIG_APP_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

//
// PCI Configuration Space I/O Ports
//
#define PCI_CONFIG_ADDRESS_PORT   0xCF8
#define PCI_CONFIG_DATA_PORT      0xCFC

//
// PCI Configuration Space Register Offsets
//
#define PCI_VENDOR_ID_OFFSET      0x00
#define PCI_DEVICE_ID_OFFSET      0x02
#define PCI_COMMAND_OFFSET        0x04
#define PCI_STATUS_OFFSET         0x06
#define PCI_REVISION_ID_OFFSET    0x08
#define PCI_CLASS_CODE_OFFSET     0x09
#define PCI_HEADER_TYPE_OFFSET    0x0E
#define PCI_SECONDARY_BUS_OFFSET  0x19  // Type 1 only
#define PCI_CAP_PTR_OFFSET        0x34  // Type 0 Capabilities Pointer

//
// PCI Header Types
//
#define PCI_HEADER_TYPE_DEVICE    0x00   // Type 0 (Endpoint)
#define PCI_HEADER_TYPE_BRIDGE    0x01   // Type 1 (PCI-PCI Bridge)
#define PCI_HEADER_TYPE_CARDBUS   0x02   // Type 2 (CardBus Bridge)
#define PCI_HEADER_TYPE_MULTI     BIT7   // Multi-function bit mask

//
// PCI Capability IDs
//
#define PCI_CAP_ID_PCI_EXPRESS    0x10   // PCI Express Capability

//
// PCI Express Capability Register - Device/Port Type values
//
#define PCIE_DEVICE_PORT_TYPE_MASK  0x0F
#define PCIE_ENDPOINT               0x00
#define PCIE_LEGACY_ENDPOINT        0x01
#define PCIE_ROOT_PORT              0x04
#define PCIE_UPSTREAM_PORT          0x05
#define PCIE_DOWNSTREAM_PORT        0x06
#define PCIE_PCIE_TO_PCI_BRIDGE     0x07
#define PCIE_PCI_TO_PCIE_BRIDGE     0x08
#define PCIE_ROOT_COMPLEX_INT_EP    0x09
#define PCIE_ROOT_COMPLEX_EVENT_COLL 0x0A

//
// Maximum bus/device/function values
//
#define PCI_MAX_BUS         255
#define PCI_MAX_DEVICE      31
#define PCI_MAX_FUNCTION    7
#define PCI_CONFIG_SPACE_SIZE  256

/**
  Reads a 32-bit value from PCI configuration space using I/O ports.

  Uses the standard PCI configuration access mechanism (#CF8/#CFC).
  The address is constructed as:
    Bit 31    = Enable (1)
    Bits 23:16 = Bus number
    Bits 15:11 = Device number
    Bits 10:8  = Function number
    Bits 7:2   = Register offset (dword-aligned)

  @param[in]  Bus       PCI bus number (0-255).
  @param[in]  Device    PCI device number (0-31).
  @param[in]  Function  PCI function number (0-7).
  @param[in]  Offset    Register offset within config space (0-255, must be 4-byte aligned).

  @return The 32-bit value read from the specified config space register.
**/
UINT32
PciReadConfigDword (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function,
  IN UINT8  Offset
  );

/**
  Reads a 16-bit value from PCI configuration space.

  @param[in]  Bus       PCI bus number (0-255).
  @param[in]  Device    PCI device number (0-31).
  @param[in]  Function  PCI function number (0-7).
  @param[in]  Offset    Register offset within config space (0-255).

  @return The 16-bit value read from the specified config space register.
**/
UINT16
PciReadConfigWord (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function,
  IN UINT8  Offset
  );

/**
  Reads an 8-bit value from PCI configuration space.

  @param[in]  Bus       PCI bus number (0-255).
  @param[in]  Device    PCI device number (0-31).
  @param[in]  Function  PCI function number (0-7).
  @param[in]  Offset    Register offset within config space (0-255).

  @return The 8-bit value read from the specified config space register.
**/
UINT8
PciReadConfigByte (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function,
  IN UINT8  Offset
  );

/**
  Checks whether a PCI device exists at the given BDF.

  A device is considered present if its Vendor ID is not 0xFFFF.

  @param[in]  Bus       PCI bus number.
  @param[in]  Device    PCI device number.
  @param[in]  Function  PCI function number.

  @retval TRUE   A device exists at the given BDF.
  @retval FALSE  No device is present.
**/
BOOLEAN
PciDeviceExists (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function
  );

/**
  Determines if the device at a given BDF is a multi-function device.

  Checks bit 7 of the Header Type register.

  @param[in]  Bus       PCI bus number.
  @param[in]  Device    PCI device number.
  @param[in]  Function  PCI function number.

  @retval TRUE   The device is multi-function capable.
  @retval FALSE  The device is single-function.
**/
BOOLEAN
PciIsMultiFunction (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function
  );

#endif
