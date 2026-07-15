/** @file
  Header for LspciApp - a UEFI Shell lspci-like utility.

  Provides PCI/PCIe device enumeration, verbose reporting, hex dump,
  and tree topology display via direct I/O port access (0xCF8/0xCFC).

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef LSPCI_APP_H_
#define LSPCI_APP_H_

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
// I/O Ports for PCI Configuration Access Mechanism #1
//
#define PCI_ADDR_PORT  0xCF8
#define PCI_DATA_PORT  0xCFC

//
// PCI Register Offsets (Standard Config Space)
//
#define PCI_VID_OFFSET       0x00
#define PCI_DID_OFFSET       0x02
#define PCI_CMD_OFFSET       0x04
#define PCI_STS_OFFSET       0x06
#define PCI_RID_OFFSET       0x08
#define PCI_CLASS_OFFSET     0x09
#define PCI_CLS_OFFSET       0x0C
#define PCI_LAT_OFFSET       0x0D
#define PCI_HTYPE_OFFSET     0x0E
#define PCI_BIST_OFFSET      0x0F
#define PCI_BAR0_OFFSET      0x10
#define PCI_BAR1_OFFSET      0x14
#define PCI_BAR2_OFFSET      0x18
#define PCI_BAR3_OFFSET      0x1C
#define PCI_BAR4_OFFSET      0x20
#define PCI_BAR5_OFFSET      0x24
#define PCI_CAP_PTR          0x34
#define PCI_INT_LINE         0x3C
#define PCI_INT_PIN          0x3D
#define PCI_BRIDGE_PBUS      0x18  // Type 1: Primary Bus
#define PCI_BRIDGE_SBUS      0x19  // Type 1: Secondary Bus
#define PCI_BRIDGE_SUBUS     0x1A  // Type 1: Subordinate Bus

//
// Header Types
//
#define PCI_HDR_DEVICE     0x00
#define PCI_HDR_BRIDGE     0x01
#define PCI_HDR_CARDBUS    0x02
#define PCI_HDR_MULTI      BIT7

//
// Capability IDs
//
#define CAP_PM              0x01
#define CAP_AGP             0x02
#define CAP_VPD             0x03
#define CAP_SLOT            0x04
#define CAP_MSI             0x05
#define CAP_CHSWP           0x06
#define CAP_PCIX            0x07
#define CAP_HT              0x08
#define CAP_VNDR            0x09
#define CAP_DBGP            0x0A
#define CAP_CPER            0x0B
#define CAP_HPCI            0x0C
#define CAP_SSVID           0x0D
#define CAP_AGP3            0x0E
#define CAP_SEC             0x0F
#define CAP_PCIE            0x10
#define CAP_MSIX            0x11
#define CAP_SATA            0x12
#define CAP_AF              0x13
#define CAP_EA              0x14

//
// Memory BAR decoding
//
#define BAR_TYPE_IO         0x01
#define BAR_MEM_32BIT       0x00
#define BAR_MEM_64BIT       0x04

//
// Limits
//
#define PCI_MAX_BUS         255
#define PCI_MAX_DEV         31
#define PCI_MAX_FUNC        7
#define PCI_CONFIG_SIZE     256

//
// PCIe Device/Port Types (from PCI Express Capability)
//
#define PCIE_DEV_TYPE_MASK  0x0F
#define PCIE_ENDPOINT       0x00
#define PCIE_LEGACY_EP      0x01
#define PCIE_ROOT_PORT      0x04
#define PCIE_UP_PORT        0x05
#define PCIE_DOWN_PORT      0x06
#define PCIE_PCIE2PCI       0x07
#define PCIE_PCI2PCIE       0x08

///
/// Entry in the PCI device list built during scan.
///
typedef struct {
  UINT8   Bus;
  UINT8   Device;
  UINT8   Function;
  UINT16  VendorId;
  UINT16  DeviceId;
  UINT16  SubVendorId;
  UINT16  SubDeviceId;
  UINT8   RevisionId;
  UINT32  ClassCode;
  UINT8   HeaderType;
  UINT8   IntPin;
  UINT8   IntLine;
  UINT32  Bar[6];
  UINT8   Capabilities[16];  // List of capability IDs
  UINT8   CapCount;
  BOOLEAN IsPcie;
  UINT8   PcieDeviceType;
  UINT8   PrimaryBus;   // Type 1 only
  UINT8   SecondaryBus; // Type 1 only
  UINT8   SubordinateBus; // Type 1 only
} PCI_DEVICE_INFO;

///
/// Node in the PCI tree topology.
///
typedef struct _PCI_TREE_NODE {
  PCI_DEVICE_INFO            *Device;
  struct _PCI_TREE_NODE      *Child;      // First child (downstream bus)
  struct _PCI_TREE_NODE      *Sibling;    // Next sibling (same bus)
} PCI_TREE_NODE;

/**
  Read a DWORD from PCI config space via I/O ports.

  @param[in]  Bus       Bus number.
  @param[in]  Device    Device number.
  @param[in]  Function  Function number.
  @param[in]  Offset    Register offset (dword-aligned).

  @return The 32-bit value read.
**/
UINT32
PciRead (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function,
  IN UINT8  Offset
  );

/**
  Read a WORD from PCI config space.

  @param[in]  Bus,Device,Function,Offset  As above.

  @return The 16-bit value read.
**/
UINT16
PciReadWord (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function,
  IN UINT8  Offset
  );

/**
  Read a BYTE from PCI config space.

  @param[in]  Bus,Device,Function,Offset  As above.

  @return The 8-bit value read.
**/
UINT8
PciReadByte (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function,
  IN UINT8  Offset
  );

/**
  Check if a PCI device exists at the given BDF.

  @retval TRUE   Vendor ID is valid (not 0xFFFF).
  @retval FALSE  No device.
**/
BOOLEAN
PciDeviceExists (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function
  );

/**
  Gather detailed information about a PCI device into a structure.

  @param[out] Info   The populated device info structure.
  @param[in]  Bus,Device,Function  BDF to query.
**/
VOID
PciGetDeviceInfo (
  OUT PCI_DEVICE_INFO  *Info,
  IN  UINT8            Bus,
  IN  UINT8            Device,
  IN  UINT8            Function
  );

/**
  Get the human-readable class name for a PCI class code.

  @param[in]  ClassCode  The 24-bit class code.

  @return A static string describing the class.
**/
CHAR16 *
PciGetClassName (
  IN UINT32  ClassCode
  );

/**
  Get the human-readable name of a capability ID.

  @param[in]  CapId  The capability ID byte.

  @return A static string.
**/
CHAR16 *
PciGetCapName (
  IN UINT8  CapId
  );

/**
  Decode and print a BAR value.

  @param[in]  BarVal  The raw 32-bit BAR register value.
  @param[in]  Index   BAR index (0-5).
**/
VOID
PciDecodeBar (
  IN UINT32  BarVal,
  IN UINTN   Index
  );

/**
  Print verbose information for a single device.

  @param[in]  Info    Device information.
  @param[in]  Level   Verbosity level (1 = -v, 2 = -vv).
**/
VOID
PciPrintVerbose (
  IN PCI_DEVICE_INFO  *Info,
  IN UINTN            Level
  );

/**
  Print the PCI device list in tree topology view.

  @param[in]  Root  Root node of the tree.
  @param[in]  Depth  Current indentation depth.
**/
VOID
PciPrintTree (
  IN PCI_TREE_NODE  *Root,
  IN UINTN          Depth
  );

/**
  Recursively build the PCI bus topology tree.

  @param[out] Root    Root node pointer.
  @param[in]  Bus     Bus number for this level.
  @param[in]  Devices  Global sorted device array.
  @param[in]  Count    Number of devices in the array.
**/
VOID
PciBuildTree (
  OUT PCI_TREE_NODE   **Root,
  IN  UINT8           Bus,
  IN  PCI_DEVICE_INFO *Devices,
  IN  UINTN           Count
  );

/**
  Print a hex dump of the standard 256-byte config space.

  @param[in]  Bus,Device,Function  BDF to dump.
**/
VOID
PciDumpHex (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function
  );

/**
  Free a PCI tree node and all its children/siblings.

  @param[in]  Node  The root node to free.
**/
VOID
PciFreeTree (
  IN PCI_TREE_NODE  *Node
  );

#endif
