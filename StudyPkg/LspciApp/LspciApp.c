/** @file
  LspciApp - A UEFI Shell lspci-like utility.

  Enumerates PCI/PCIe devices and displays information in multiple formats:
    lspci           - Basic device list
    lspci -v        - Verbose (class, revision, BARs, capabilities)
    lspci -vv       - Very verbose (detailed capability decode)
    lspci -x        - 256-byte config space hex dump
    lspci -tv       - PCI bus topology tree

  All PCI config space access uses I/O ports 0xCF8/0xCFC.

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "LspciApp.h"

//
// -----------------------------------------------------------------------------
// PCI Config Space I/O Primitives
// -----------------------------------------------------------------------------

/**
  Read a DWORD from PCI config space via I/O ports.
**/
UINT32
PciRead (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function,
  IN UINT8  Offset
  )
{
  UINT32  Address;

  Address = (UINT32)(
    BIT31 |
    ((UINT32)Bus      << 16) |
    ((UINT32)Device   << 11) |
    ((UINT32)Function << 8)  |
    (Offset & 0xFC)
    );

  IoWrite32 (PCI_ADDR_PORT, Address);
  return IoRead32 (PCI_DATA_PORT);
}

/**
  Read a WORD from PCI config space.
**/
UINT16
PciReadWord (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function,
  IN UINT8  Offset
  )
{
  UINT32  Dword;
  UINTN   ByteOff;

  ByteOff = (UINTN)(Offset & 3);
  Dword   = PciRead (Bus, Device, Function, (UINT8)(Offset & 0xFC));

  return (UINT16)((Dword >> (ByteOff * 8)) & 0xFFFF);
}

/**
  Read a BYTE from PCI config space.
**/
UINT8
PciReadByte (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function,
  IN UINT8  Offset
  )
{
  UINT32  Dword;
  UINTN   ByteOff;

  ByteOff = (UINTN)(Offset & 3);
  Dword   = PciRead (Bus, Device, Function, (UINT8)(Offset & 0xFC));

  return (UINT8)((Dword >> (ByteOff * 8)) & 0xFF);
}

/**
  Check if a PCI device exists at the given BDF.
**/
BOOLEAN
PciDeviceExists (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function
  )
{
  return (BOOLEAN)(PciReadWord (Bus, Device, Function, PCI_VID_OFFSET) != 0xFFFF);
}

//
// -----------------------------------------------------------------------------
// Device Information Gathering
// -----------------------------------------------------------------------------

/**
  Gather detailed information about a PCI device.
**/
VOID
PciGetDeviceInfo (
  OUT PCI_DEVICE_INFO  *Info,
  IN  UINT8            Bus,
  IN  UINT8            Device,
  IN  UINT8            Function
  )
{
  UINTN   Index;
  UINT8   CapPtr;
  UINT32  Dword;
  UINT8   Idx;

  SetMem (Info, sizeof (PCI_DEVICE_INFO), 0);

  Info->Bus       = Bus;
  Info->Device    = Device;
  Info->Function  = Function;

  Info->VendorId  = PciReadWord (Bus, Device, Function, PCI_VID_OFFSET);
  Info->DeviceId  = PciReadWord (Bus, Device, Function, PCI_DID_OFFSET);

  Dword               = PciRead (Bus, Device, Function, PCI_RID_OFFSET);
  Info->RevisionId    = (UINT8)((Dword >> 0) & 0xFF);
  Info->ClassCode     = (Dword >> 8) & 0xFFFFFF;

  Info->HeaderType    = PciReadByte (Bus, Device, Function, PCI_HTYPE_OFFSET);
  Info->IntPin        = PciReadByte (Bus, Device, Function, PCI_INT_PIN);
  Info->IntLine       = PciReadByte (Bus, Device, Function, PCI_INT_LINE);

  //
  // Read BARs.
  //
  for (Index = 0; Index < 6; Index++) {
    Info->Bar[Index] = PciRead (Bus, Device, Function, (UINT8)(PCI_BAR0_OFFSET + (UINT8)(Index * 4)));
  }

  //
  // Subsystem Vendor/Device ID (Type 0, offset 0x2C).
  //
  Info->SubVendorId  = PciReadWord (Bus, Device, Function, 0x2C);
  Info->SubDeviceId  = PciReadWord (Bus, Device, Function, 0x2E);

  //
  // Bridge bus numbers (Type 1 only).
  //
  if ((Info->HeaderType & 0x7F) == PCI_HDR_BRIDGE) {
    Info->PrimaryBus    = PciReadByte (Bus, Device, Function, PCI_BRIDGE_PBUS);
    Info->SecondaryBus  = PciReadByte (Bus, Device, Function, PCI_BRIDGE_SBUS);
    Info->SubordinateBus = PciReadByte (Bus, Device, Function, PCI_BRIDGE_SUBUS);
  }

  //
  // Walk the Capabilities List.
  //
  CapPtr = PciReadByte (Bus, Device, Function, PCI_CAP_PTR);
  Idx    = 0;

  while ((CapPtr != 0) && (Idx < 16)) {
    if (CapPtr >= 0x40) {
      Info->Capabilities[Idx] = PciReadByte (Bus, Device, Function, CapPtr);

      //
      // Check for PCI Express Capability.
      //
      if (Info->Capabilities[Idx] == CAP_PCIE) {
        Info->IsPcie = TRUE;
        Dword        = PciRead (Bus, Device, Function, (UINT8)(CapPtr + 2));
        Info->PcieDeviceType = (UINT8)(Dword & PCIE_DEV_TYPE_MASK);
      }

      Idx++;

      CapPtr = PciReadByte (Bus, Device, Function, (UINT8)(CapPtr + 1));
    } else {
      break;
    }
  }

  Info->CapCount = Idx;
}

//
// -----------------------------------------------------------------------------
// Class Name Lookup Table
// -----------------------------------------------------------------------------

typedef struct {
  UINT8   BaseClass;
  UINT8   SubClass;
  CHAR16  *Name;
} PCI_CLASS_ENTRY;

GLOBAL_REMOVE_IF_UNREFERENCED PCI_CLASS_ENTRY  mClassNameTable[] = {
  { 0x00, 0x00, L"Legacy device" },
  { 0x01, 0x00, L"SCSI bus controller" },
  { 0x01, 0x01, L"IDE controller" },
  { 0x01, 0x02, L"Floppy disk controller" },
  { 0x01, 0x03, L"IPI controller" },
  { 0x01, 0x04, L"RAID controller" },
  { 0x01, 0x05, L"ATA controller" },
  { 0x01, 0x06, L"SATA controller" },
  { 0x01, 0x07, L"SAS controller" },
  { 0x01, 0x08, L"NVM Express controller" },
  { 0x01, 0x80, L"Mass storage controller" },
  { 0x02, 0x00, L"Ethernet controller" },
  { 0x02, 0x01, L"Token ring controller" },
  { 0x02, 0x02, L"FDDI controller" },
  { 0x02, 0x03, L"ATM controller" },
  { 0x02, 0x04, L"ISDN controller" },
  { 0x02, 0x05, L"WorldFip controller" },
  { 0x02, 0x06, L"PICMG controller" },
  { 0x02, 0x07, L"Infiniband controller" },
  { 0x02, 0x08, L"Fabric controller" },
  { 0x02, 0x80, L"Network controller" },
  { 0x03, 0x00, L"VGA compatible controller" },
  { 0x03, 0x01, L"XGA controller" },
  { 0x03, 0x02, L"3D controller" },
  { 0x03, 0x80, L"Display controller" },
  { 0x04, 0x00, L"Video device" },
  { 0x04, 0x01, L"Audio device" },
  { 0x04, 0x02, L"Computer telephony device" },
  { 0x04, 0x80, L"Multimedia controller" },
  { 0x05, 0x00, L"RAM controller" },
  { 0x05, 0x01, L"Flash controller" },
  { 0x05, 0x80, L"Memory controller" },
  { 0x06, 0x00, L"Host bridge" },
  { 0x06, 0x01, L"ISA bridge" },
  { 0x06, 0x02, L"EISA bridge" },
  { 0x06, 0x03, L"MCA bridge" },
  { 0x06, 0x04, L"PCI-PCI bridge" },
  { 0x06, 0x05, L"PCMCIA bridge" },
  { 0x06, 0x06, L"NuBus bridge" },
  { 0x06, 0x07, L"CardBus bridge" },
  { 0x06, 0x08, L"RACEway bridge" },
  { 0x06, 0x09, L"PCI-PCI bridge (Transparent)" },
  { 0x06, 0x0A, L"InfiniBand-PCI bridge" },
  { 0x06, 0x80, L"Bridge" },
  { 0x07, 0x00, L"Serial controller (8250)" },
  { 0x07, 0x01, L"Parallel controller (SPP)" },
  { 0x07, 0x02, L"Parallel controller (Bidirectional)" },
  { 0x07, 0x03, L"Serial controller (16750)" },
  { 0x07, 0x80, L"Communication controller" },
  { 0x08, 0x00, L"PIC (8259)" },
  { 0x08, 0x01, L"DMA controller (8237)" },
  { 0x08, 0x02, L"System timer (8254)" },
  { 0x08, 0x03, L"RTC controller" },
  { 0x08, 0x04, L"Generic PCI Hot-Plug controller" },
  { 0x08, 0x05, L"SD controller" },
  { 0x08, 0x06, L"IOMMU" },
  { 0x08, 0x80, L"System peripheral" },
  { 0x09, 0x00, L"Keyboard controller" },
  { 0x09, 0x01, L"Digitizer (Pen)" },
  { 0x09, 0x02, L"Mouse controller" },
  { 0x09, 0x03, L"Scanner controller" },
  { 0x09, 0x04, L"Gameport controller" },
  { 0x09, 0x80, L"Input device controller" },
  { 0x0A, 0x00, L"Docking station" },
  { 0x0B, 0x00, L"386 Processor" },
  { 0x0B, 0x01, L"486 Processor" },
  { 0x0B, 0x02, L"Pentium Processor" },
  { 0x0B, 0x03, L"Pentium Pro Processor" },
  { 0x0B, 0x10, L"Alpha Processor" },
  { 0x0B, 0x20, L"PowerPC Processor" },
  { 0x0B, 0x30, L"MIPS Processor" },
  { 0x0B, 0x40, L"Co-processor" },
  { 0x0C, 0x00, L"FireWire (IEEE 1394)" },
  { 0x0C, 0x01, L"ACCESS Bus" },
  { 0x0C, 0x02, L"SSA" },
  { 0x0C, 0x03, L"USB controller" },
  { 0x0C, 0x04, L"Fibre Channel" },
  { 0x0C, 0x05, L"System Management Bus (SMBus)" },
  { 0x0C, 0x06, L"InfiniBand" },
  { 0x0C, 0x07, L"IPMI Interface" },
  { 0x0C, 0x08, L"SERCOS Interface" },
  { 0x0C, 0x09, L"CANBUS controller" },
  { 0x0C, 0x80, L"Serial bus controller" },
  { 0x0D, 0x00, L"Wifi controller" },
  { 0x0D, 0x01, L"Satellite controller" },
  { 0x0D, 0x80, L"Wireless controller" },
  { 0x0E, 0x00, L"I2O" },
  { 0x0F, 0x00, L"Satellite TV controller" },
  { 0x10, 0x00, L"Encryption/Decryption controller" },
  { 0x11, 0x00, L"Data Acquisition/Signal Processing" },
  { 0x12, 0x00, L"Processing Accelerator" },
  { 0x12, 0x01, L"AI Inference Accelerator" },
  { 0x13, 0x00, L"Non-Essential Instrumentation" },
  { 0x40, 0x00, L"Co-Processor" },
  { 0xFF, 0x00, L"Unassigned class" },
  { 0,    0,    NULL }
};

/**
  Look up the class name for a PCI class code.
**/
CHAR16 *
PciGetClassName (
  IN UINT32  ClassCode
  )
{
  UINT8   BaseClass;
  UINT8   SubClass;
  UINTN   Index;

  BaseClass = (UINT8)((ClassCode >> 16) & 0xFF);
  SubClass  = (UINT8)((ClassCode >> 8) & 0xFF);

  for (Index = 0; mClassNameTable[Index].Name != NULL; Index++) {
    if ((mClassNameTable[Index].BaseClass == BaseClass) &&
        (mClassNameTable[Index].SubClass == SubClass))
    {
      return mClassNameTable[Index].Name;
    }
  }

  //
  // Return base class generic name if subclass not found.
  //
  for (Index = 0; mClassNameTable[Index].Name != NULL; Index++) {
    if ((mClassNameTable[Index].BaseClass == BaseClass) &&
        (mClassNameTable[Index].SubClass == 0x80))
    {
      return mClassNameTable[Index].Name;
    }
  }

  return L"Unknown device";
}

//
// -----------------------------------------------------------------------------
// Capability Name Lookup
// -----------------------------------------------------------------------------

typedef struct {
  UINT8   CapId;
  CHAR16  *Name;
} PCI_CAP_ENTRY;

GLOBAL_REMOVE_IF_UNREFERENCED PCI_CAP_ENTRY  mCapNameTable[] = {
  { 0x01, L"PM"     },
  { 0x02, L"AGP"    },
  { 0x03, L"VPD"    },
  { 0x04, L"SLOT"   },
  { 0x05, L"MSI"    },
  { 0x06, L"CHSWP"  },
  { 0x07, L"PCI-X"  },
  { 0x08, L"HT"     },
  { 0x09, L"VNDR"   },
  { 0x0A, L"DBGP"   },
  { 0x0B, L"CPER"   },
  { 0x0C, L"HPCI"   },
  { 0x0D, L"SSVID"  },
  { 0x0E, L"AGP3"   },
  { 0x0F, L"SEC"    },
  { 0x10, L"PCIE"   },
  { 0x11, L"MSI-X"  },
  { 0x12, L"SATA"   },
  { 0x13, L"AF"     },
  { 0x14, L"EA"     },
  { 0,    NULL      }
};

/**
  Get the name of a capability by its ID.
**/
CHAR16 *
PciGetCapName (
  IN UINT8  CapId
  )
{
  UINTN  Index;

  for (Index = 0; mCapNameTable[Index].Name != NULL; Index++) {
    if (mCapNameTable[Index].CapId == CapId) {
      return mCapNameTable[Index].Name;
    }
  }

  return L"UNKN";
}

//
// -----------------------------------------------------------------------------
// BAR Decoding
// -----------------------------------------------------------------------------

/**
  Decode and print a Base Address Register.
**/
VOID
PciDecodeBar (
  IN UINT32  BarVal,
  IN UINTN   Index
  )
{
  if (BarVal == 0) {
    return;
  }

  if ((BarVal & BAR_TYPE_IO) != 0) {
    //
    // I/O BAR.
    //
    ShellPrintEx (
      -1, -1,
      L"        Region %d: I/O ports at 0x%04x\n",
      (UINTN)Index,
      (UINTN)(BarVal & 0xFFFFFFFC)
      );
  } else {
    //
    // Memory BAR.
    //
    ShellPrintEx (
      -1, -1,
      L"        Region %d: Memory at 0x%08x (%d-bit, %s",
      (UINTN)Index,
      (UINTN)(BarVal & 0xFFFFFFF0),
      (UINTN)(((BarVal & BAR_MEM_64BIT) != 0) ? 64 : 32),
      ((BarVal & BIT3) != 0) ? L"prefetchable" : L"non-prefetchable"
      );

    //
    // Decode memory type.
    //
    switch ((BarVal >> 1) & 0x03) {
    case 0:
      ShellPrintEx (-1, -1, L", 32-bit)\n");
      break;
    case 2:
      ShellPrintEx (-1, -1, L", 64-bit)\n");
      break;
    default:
      ShellPrintEx (-1, -1, L")\n");
      break;
    }
  }
}

//
// -----------------------------------------------------------------------------
// Verbose Printing
// -----------------------------------------------------------------------------

GLOBAL_REMOVE_IF_UNREFERENCED CHAR16 *mPcieTypeNames[] = {
  L"PCI Express Endpoint",
  L"Legacy PCI Express Endpoint",
  L"Unknown",
  L"Unknown",
  L"Root Port of PCI Express Root Complex",
  L"Upstream Port of PCI Express Switch",
  L"Downstream Port of PCI Express Switch",
  L"PCI Express to PCI/PCI-X Bridge",
  L"PCI/PCI-X to PCI Express Bridge",
  L"Root Complex Integrated Endpoint",
  L"Root Complex Event Collector"
};

/**
  Print verbose information for a single device.
**/
VOID
PciPrintVerbose (
  IN PCI_DEVICE_INFO  *Info,
  IN UINTN            Level
  )
{
  UINTN   Index;
  UINT8   CapId;
  UINT8   HType;

  HType = (UINT8)(Info->HeaderType & 0x7F);

  //
  // Line 1: BDF, VID:DID, Class.
  //
  ShellPrintEx (
    -1, -1,
    L"%02x:%02x.%02x %04x:%04x %s (rev %02x)\n",
    (UINTN)Info->Bus,
    (UINTN)Info->Device,
    (UINTN)Info->Function,
    (UINTN)Info->VendorId,
    (UINTN)Info->DeviceId,
    PciGetClassName (Info->ClassCode),
    (UINTN)Info->RevisionId
    );

  //
  // Line 2: Header Type, Subsystem.
  //
  ShellPrintEx (-1, -1, L"  Header Type: %02x (", (UINTN)Info->HeaderType);

  switch (HType) {
  case PCI_HDR_DEVICE:
    ShellPrintEx (-1, -1, L"Type 0 - Endpoint");
    break;
  case PCI_HDR_BRIDGE:
    ShellPrintEx (-1, -1, L"Type 1 - PCI-PCI Bridge");
    break;
  case PCI_HDR_CARDBUS:
    ShellPrintEx (-1, -1, L"Type 2 - CardBus");
    break;
  default:
    ShellPrintEx (-1, -1, L"Unknown");
    break;
  }

  if ((Info->HeaderType & PCI_HDR_MULTI) != 0) {
    ShellPrintEx (-1, -1, L", Multi-Function");
  }

  ShellPrintEx (-1, -1, L")\n");

  ShellPrintEx (
    -1, -1,
    L"  Subsystem: %04x:%04x\n",
    (UINTN)Info->SubVendorId,
    (UINTN)Info->SubDeviceId
    );

  //
  // Interrupt info.
  //
  if (Info->IntPin != 0) {
    ShellPrintEx (
      -1, -1,
      L"  Interrupt: pin %c, line 0x%02x\n",
      (UINTN)(L'A' + Info->IntPin - 1),
      (UINTN)Info->IntLine
      );
  }

  //
  // BARs (-v and above).
  //
  if ((Level >= 1) && (HType == PCI_HDR_DEVICE)) {
    for (Index = 0; Index < 6; Index++) {
      PciDecodeBar (Info->Bar[Index], Index);
    }
  }

  //
  // Bridge bus numbers (-v and above).
  //
  if (HType == PCI_HDR_BRIDGE) {
    ShellPrintEx (
      -1, -1,
      L"  Bus: primary=%02x, secondary=%02x, subordinate=%02x\n",
      (UINTN)Info->PrimaryBus,
      (UINTN)Info->SecondaryBus,
      (UINTN)Info->SubordinateBus
      );
  }

  //
  // PCIe information.
  //
  if (Info->IsPcie) {
    ShellPrintEx (-1, -1, L"  PCIe Device Type: ");
    if (Info->PcieDeviceType < sizeof (mPcieTypeNames) / sizeof (CHAR16 *)) {
      ShellPrintEx (-1, -1, L"%s\n", mPcieTypeNames[Info->PcieDeviceType]);
    } else {
      ShellPrintEx (-1, -1, L"Unknown (0x%x)\n", (UINTN)Info->PcieDeviceType);
    }
  }

  //
  // Capabilities list (-v: names only; -vv: with basic decode).
  //
  if (Info->CapCount > 0) {
    ShellPrintEx (-1, -1, L"  Capabilities: [");

    for (Index = 0; Index < Info->CapCount; Index++) {
      CapId = Info->Capabilities[Index];
      ShellPrintEx (-1, -1, L" %s", PciGetCapName (CapId));
    }

    ShellPrintEx (-1, -1, L" ]\n");
  }

  ShellPrintEx (-1, -1, L"\n");
}

//
// -----------------------------------------------------------------------------
// Hex Dump
// -----------------------------------------------------------------------------

/**
  Print a hex dump of the 256-byte PCI config space.
**/
VOID
PciDumpHex (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function
  )
{
  UINT8   Buffer[PCI_CONFIG_SIZE];
  UINTN   Offset;
  UINTN   Index;
  UINT8   Val;

  //
  // Read the full 256-byte config space.
  //
  for (Offset = 0; Offset < PCI_CONFIG_SIZE; Offset++) {
    Buffer[Offset] = PciReadByte (Bus, Device, Function, (UINT8)Offset);
  }

  ShellPrintEx (
    -1, -1,
    L"Config space dump for %02x:%02x.%02x:\n",
    (UINTN)Bus, (UINTN)Device, (UINTN)Function
    );

  //
  // 16 bytes per line.
  //
  for (Offset = 0; Offset < PCI_CONFIG_SIZE; Offset += 16) {
    ShellPrintEx (-1, -1, L"%02x: ", (UINTN)Offset);

    for (Index = 0; Index < 8; Index++) {
      Val = Buffer[Offset + Index];
      ShellPrintEx (-1, -1, L"%02x ", (UINTN)Val);
    }

    ShellPrintEx (-1, -1, L" ");

    for (Index = 8; Index < 16; Index++) {
      Val = Buffer[Offset + Index];
      ShellPrintEx (-1, -1, L"%02x ", (UINTN)Val);
    }

    ShellPrintEx (-1, -1, L"\n");
  }
}

//
// -----------------------------------------------------------------------------
// Tree Topology
// -----------------------------------------------------------------------------

/**
  Free a PCI tree node and all its children/siblings.
**/
VOID
PciFreeTree (
  IN PCI_TREE_NODE  *Node
  )
{
  if (Node == NULL) {
    return;
  }

  PciFreeTree (Node->Child);
  PciFreeTree (Node->Sibling);

  FreePool (Node);
}

/**
  Recursively build the PCI bus topology tree.

  @param[out] Root     Root node for this bus level.
  @param[in]  Bus      Bus number to scan.
  @param[in]  Devices  Array of all devices.
  @param[in]  Count    Device count.
**/
VOID
PciBuildTree (
  OUT PCI_TREE_NODE   **Root,
  IN  UINT8           Bus,
  IN  PCI_DEVICE_INFO *Devices,
  IN  UINTN           Count
  )
{
  PCI_TREE_NODE  *Node;
  PCI_TREE_NODE  *Tail;
  PCI_TREE_NODE  *ChildRoot;
  UINTN          Index;

  *Root = NULL;
  Tail  = NULL;

  //
  // Iterate all devices and pick those on this bus.
  //
  for (Index = 0; Index < Count; Index++) {
    if (Devices[Index].Bus != Bus) {
      continue;
    }

    Node = AllocateZeroPool (sizeof (PCI_TREE_NODE));
    if (Node == NULL) {
      continue;
    }

    Node->Device = &Devices[Index];

    //
    // Append to the sibling list.
    //
    if (*Root == NULL) {
      *Root = Node;
    } else {
      Tail->Sibling = Node;
    }

    Tail = Node;

    //
    // If this is a PCI-PCI bridge, recursively scan downstream bus.
    //
    if (((Devices[Index].HeaderType & 0x7F) == PCI_HDR_BRIDGE) &&
        (Devices[Index].SecondaryBus != 0))
    {
      ChildRoot = NULL;
      PciBuildTree (&ChildRoot, Devices[Index].SecondaryBus, Devices, Count);
      Node->Child = ChildRoot;
    }
  }
}

/**
  Print the PCI bus topology tree.
**/
VOID
PciPrintTree (
  IN PCI_TREE_NODE  *Root,
  IN UINTN          Depth
  )
{
  PCI_TREE_NODE  *Node;
  UINTN          Index;

  for (Node = Root; Node != NULL; Node = Node->Sibling) {
    //
    // Print indentation and connector.
    //
    for (Index = 0; Index < Depth; Index++) {
      ShellPrintEx (-1, -1, L"  ");
    }

    if (Node->Sibling != NULL) {
      ShellPrintEx (-1, -1, L"+-");
    } else {
      ShellPrintEx (-1, -1, L"\\-");
    }

    //
    // Print device summary: BDF + VID:DID + Class name.
    //
    ShellPrintEx (
      -1, -1,
      L"%02x:%02x.%02x  %04x:%04x  %s",
      (UINTN)Node->Device->Bus,
      (UINTN)Node->Device->Device,
      (UINTN)Node->Device->Function,
      (UINTN)Node->Device->VendorId,
      (UINTN)Node->Device->DeviceId,
      PciGetClassName (Node->Device->ClassCode)
      );

    if (Node->Device->IsPcie) {
      ShellPrintEx (-1, -1, L" (PCIE)");
    }

    ShellPrintEx (-1, -1, L"\n");

    //
    // Recursively print children (downstream buses).
    //
    if (Node->Child != NULL) {
      PciPrintTree (Node->Child, Depth + 1);
    }
  }
}

//
// -----------------------------------------------------------------------------
// Main Entry Point
// -----------------------------------------------------------------------------

/**
  The main entry point for the LspciApp Shell application.

  Supported usage:
    lspci             - Basic device list
    lspci -v          - Verbose output
    lspci -vv         - Very verbose output
    lspci -x          - Hex dump for all devices
    lspci -tv         - Tree topology view

  @param[in]  Argc  Number of arguments.
  @param[in]  Argv  Argument array.

  @retval SHELL_SUCCESS         Command succeeded.
  @retval SHELL_INVALID_PARAMETER  Unrecognized option.
**/
INTN
EFIAPI
ShellAppMain (
  IN UINTN   Argc,
  IN CHAR16  **Argv
  )
{
  UINTN           MaxDevices;
  PCI_DEVICE_INFO *Devices;
  UINTN           DeviceCount;
  UINTN           BusIndex;
  UINT8           Bus;
  UINT8           Device;
  UINT8           Function;
  UINT8           MaxFunction;
  UINTN           Index;
  BOOLEAN         FlagV;
  BOOLEAN         FlagVV;
  BOOLEAN         FlagX;
  BOOLEAN         FlagTv;
  CHAR16          *CmdName;
  UINTN           VerboseLevel;
  PCI_TREE_NODE   *TreeRoot;

  FlagV  = FALSE;
  FlagVV = FALSE;
  FlagX  = FALSE;
  FlagTv = FALSE;

  CmdName = (Argc > 0) ? Argv[0] : L"lspci";

  //
  // Parse arguments.
  //
  for (Index = 1; Index < Argc; Index++) {
    if (StrCmp (Argv[Index], L"-v") == 0) {
      FlagV = TRUE;
    } else if (StrCmp (Argv[Index], L"-vv") == 0) {
      FlagVV = TRUE;
    } else if (StrCmp (Argv[Index], L"-x") == 0) {
      FlagX = TRUE;
    } else if (StrCmp (Argv[Index], L"-tv") == 0) {
      FlagTv = TRUE;
    } else {
      ShellPrintEx (-1, -1, L"Usage: %s [-v|-vv|-x|-tv]\n", CmdName);
      return SHELL_INVALID_PARAMETER;
    }
  }

  VerboseLevel = FlagVV ? 2 : (FlagV ? 1 : 0);

  //
  // First pass: count devices for allocation.
  //
  MaxDevices = 0;

  for (BusIndex = 0; BusIndex <= PCI_MAX_BUS; BusIndex++) {
    Bus = (UINT8)BusIndex;

    for (Device = 0; Device <= PCI_MAX_DEV; Device++) {
      if (!PciDeviceExists (Bus, Device, 0)) {
        continue;
      }

      MaxFunction = PciReadByte (Bus, Device, 0, PCI_HTYPE_OFFSET) & PCI_HDR_MULTI;
      MaxFunction = (MaxFunction != 0) ? PCI_MAX_FUNC : 0;

      for (Function = 0; Function <= MaxFunction; Function++) {
        if (PciDeviceExists (Bus, Device, Function)) {
          MaxDevices++;
        }
      }
    }
  }

  if (MaxDevices == 0) {
    ShellPrintEx (-1, -1, L"No PCI devices found.\n");
    return SHELL_SUCCESS;
  }

  //
  // Allocate device array.
  //
  Devices = AllocateZeroPool (MaxDevices * sizeof (PCI_DEVICE_INFO));
  if (Devices == NULL) {
    return SHELL_OUT_OF_RESOURCES;
  }

  //
  // Second pass: gather info.
  //
  DeviceCount = 0;

  for (BusIndex = 0; BusIndex <= PCI_MAX_BUS; BusIndex++) {
    Bus = (UINT8)BusIndex;

    for (Device = 0; Device <= PCI_MAX_DEV; Device++) {
      if (!PciDeviceExists (Bus, Device, 0)) {
        continue;
      }

      MaxFunction = PciReadByte (Bus, Device, 0, PCI_HTYPE_OFFSET) & PCI_HDR_MULTI;
      MaxFunction = (MaxFunction != 0) ? PCI_MAX_FUNC : 0;

      for (Function = 0; Function <= MaxFunction; Function++) {
        if (!PciDeviceExists (Bus, Device, Function)) {
          continue;
        }

        PciGetDeviceInfo (&Devices[DeviceCount], Bus, Device, Function);
        DeviceCount++;
      }
    }
  }

  //
  // Tree view mode (-tv).
  //
  if (FlagTv) {
    TreeRoot = NULL;
    PciBuildTree (&TreeRoot, 0, Devices, DeviceCount);

    ShellPrintEx (-1, -1, L"-[0000:00]-\n");
    if (TreeRoot != NULL) {
      PciPrintTree (TreeRoot, 1);
    }

    PciFreeTree (TreeRoot);
    FreePool (Devices);
    return SHELL_SUCCESS;
  }

  //
  // Standard list mode (with optional -v, -vv, -x).
  //
  for (Index = 0; Index < DeviceCount; Index++) {
    if (VerboseLevel > 0) {
      //
      // Verbose mode.
      //
      PciPrintVerbose (&Devices[Index], VerboseLevel);
    } else {
      //
      // Compact mode: BDF VID:DID Class.
      //
      if (Devices[Index].IsPcie) {
        ShellPrintEx (
          -1, -1,
          L"%02x:%02x.%02x %04x:%04x %s (PCIE)\n",
          (UINTN)Devices[Index].Bus,
          (UINTN)Devices[Index].Device,
          (UINTN)Devices[Index].Function,
          (UINTN)Devices[Index].VendorId,
          (UINTN)Devices[Index].DeviceId,
          PciGetClassName (Devices[Index].ClassCode)
          );
      } else {
        ShellPrintEx (
          -1, -1,
          L"%02x:%02x.%02x %04x:%04x %s\n",
          (UINTN)Devices[Index].Bus,
          (UINTN)Devices[Index].Device,
          (UINTN)Devices[Index].Function,
          (UINTN)Devices[Index].VendorId,
          (UINTN)Devices[Index].DeviceId,
          PciGetClassName (Devices[Index].ClassCode)
          );
      }
    }

    //
    // Hex dump for each device (-x).
    //
    if (FlagX) {
      PciDumpHex (
        Devices[Index].Bus,
        Devices[Index].Device,
        Devices[Index].Function
        );
    }
  }

  FreePool (Devices);
  return SHELL_SUCCESS;
}
