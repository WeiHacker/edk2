/** @file
  Implementation of the PciConfigApp - a UEFI Shell application.

  This application uses direct I/O port access (0xCF8/0xCFC) to read
  PCI configuration space. It supports two modes:
  1. Scan mode (no arguments): enumerate all PCI/PCIe devices
  2. Dump mode (B:D:F argument): dump config space and analyze device type

  Copyright (c) 2026, Study. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PciConfigApp.h"

/**
  Reads a 32-bit value from PCI configuration space using I/O ports.

  @param[in]  Bus       PCI bus number (0-255).
  @param[in]  Device    PCI device number (0-31).
  @param[in]  Function  PCI function number (0-7).
  @param[in]  Offset    Register offset (must be 4-byte aligned).

  @return The 32-bit value read from the specified config space register.
**/
UINT32
PciReadConfigDword (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function,
  IN UINT8  Offset
  )
{
  UINT32  Address;

  //
  // Construct the PCI configuration address per the x86 specification:
  //   Bit 31    = Enable (1)
  //   Bits 23:16 = Bus
  //   Bits 15:11 = Device
  //   Bits 10:8  = Function
  //   Bits 7:2   = Dword-aligned offset ((Offset >> 2) << 2)
  //   Bits 1:0   = 00 (dword alignment)
  //
  Address = (UINT32)(
    BIT31 |
    ((UINT32)Bus      << 16) |
    ((UINT32)Device   << 11) |
    ((UINT32)Function << 8)  |
    (Offset & 0xFC)
    );

  //
  // Write the address to the config address port, then read the data.
  //
  IoWrite32 (PCI_CONFIG_ADDRESS_PORT, Address);
  return IoRead32 (PCI_CONFIG_DATA_PORT);
}

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
  )
{
  UINT32  Dword;
  UINTN   ByteOffset;

  ByteOffset = (UINTN)(Offset & 0x03);
  Dword      = PciReadConfigDword (Bus, Device, Function, (UINT8)(Offset & 0xFC));

  return (UINT16)((Dword >> (ByteOffset * 8)) & 0xFFFF);
}

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
  )
{
  UINT32  Dword;
  UINTN   ByteOffset;

  ByteOffset = (UINTN)(Offset & 0x03);
  Dword      = PciReadConfigDword (Bus, Device, Function, (UINT8)(Offset & 0xFC));

  return (UINT8)((Dword >> (ByteOffset * 8)) & 0xFF);
}

/**
  Checks whether a PCI device exists at the given BDF.

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
  )
{
  UINT16  VendorId;

  VendorId = PciReadConfigWord (Bus, Device, Function, PCI_VENDOR_ID_OFFSET);

  //
  // A Vendor ID of 0xFFFF indicates no device present.
  //
  return (BOOLEAN)(VendorId != 0xFFFF);
}

/**
  Determines if the device at a given BDF is a multi-function device.

  @param[in]  Bus       PCI bus number.
  @param[in]  Device    PCI device number.
  @param[in]  Function  PCI function number (typically 0 for check).

  @retval TRUE   The device is multi-function capable.
  @retval FALSE  The device is single-function.
**/
BOOLEAN
PciIsMultiFunction (
  IN UINT8  Bus,
  IN UINT8  Device,
  IN UINT8  Function
  )
{
  UINT8  HeaderType;

  HeaderType = PciReadConfigByte (Bus, Device, Function, PCI_HEADER_TYPE_OFFSET);

  return (BOOLEAN)((HeaderType & PCI_HEADER_TYPE_MULTI) != 0);
}

/**
  Scans all PCI buses and enumerates every present device.

  For each device found, prints Bus:Dev:Fun VendorId DeviceId.
  Handles multi-function devices correctly.

  @param[in]  Shell  The shell protocol for console output.
**/
VOID
PciScanAllDevices (
  IN EFI_SHELL_PROTOCOL  *Shell
  )
{
  UINT8    Bus;
  UINT8    Device;
  UINT8    Function;
  UINT8    MaxFunction;
  UINT16   VendorId;
  UINT16   DeviceId;
  UINT8    HeaderType;
  BOOLEAN  IsPcie;
  UINTN    DeviceCount;
  UINTN    BusIndex;

  DeviceCount = 0;

  //
  // Print header.
  //
  ShellPrintEx (-1, -1, L"PCI Device Scan\n");
  ShellPrintEx (-1, -1, L"===============\n");
  ShellPrintEx (-1, -1, L"Bus Dev Fun VendorID DeviceID  Type\n");
  ShellPrintEx (-1, -1, L"--- --- --- -------- --------  ----\n");

  //
  // Scan all PCI buses using a UINTN index to prevent UINT8 wrap-around.
  //
  for (BusIndex = 0; BusIndex <= PCI_MAX_BUS; BusIndex++) {
    Bus = (UINT8)BusIndex;

    for (Device = 0; Device <= PCI_MAX_DEVICE; Device++) {
      //
      // Check if device exists at function 0.
      //
      if (!PciDeviceExists (Bus, Device, 0)) {
        continue;
      }

      //
      // Determine how many functions to scan.
      //
      if (PciIsMultiFunction (Bus, Device, 0)) {
        MaxFunction = PCI_MAX_FUNCTION;
      } else {
        MaxFunction = 0;
      }

      for (Function = 0; Function <= MaxFunction; Function++) {
        if (!PciDeviceExists (Bus, Device, Function)) {
          continue;
        }

        VendorId   = PciReadConfigWord (Bus, Device, Function, PCI_VENDOR_ID_OFFSET);
        DeviceId   = PciReadConfigWord (Bus, Device, Function, PCI_DEVICE_ID_OFFSET);
        HeaderType = PciReadConfigByte (Bus, Device, Function, PCI_HEADER_TYPE_OFFSET);

        //
        // Determine if the device is PCI or PCIe by checking for the
        // PCI Express capability (Cap ID 0x10).
        //
        IsPcie = FALSE;
        {
          UINT8  CapPtr;
          UINT8  NextCap;

          //
          // Read the Capabilities Pointer at offset 0x34.
          //
          CapPtr = PciReadConfigByte (Bus, Device, Function, PCI_CAP_PTR_OFFSET);

          while (CapPtr != 0) {
            if (CapPtr >= 0x40) {
              //
              // Read Capability ID at this pointer.
              //
              if (PciReadConfigByte (Bus, Device, Function, CapPtr) == PCI_CAP_ID_PCI_EXPRESS) {
                IsPcie = TRUE;
                break;
              }

              //
              // Read next capability pointer (at CapPtr + 1).
              //
              NextCap = PciReadConfigByte (Bus, Device, Function, (UINT8)(CapPtr + 1));
              CapPtr  = NextCap;
            } else {
              break;
            }
          }
        }

        ShellPrintEx (
          -1,
          -1,
          L"%02x %02x %02x   0x%04X  0x%04X  %s\n",
          (UINTN)Bus,
          (UINTN)Device,
          (UINTN)Function,
          (UINTN)VendorId,
          (UINTN)DeviceId,
          IsPcie ? L"PCIE" : L"PCI"
          );

        DeviceCount++;
      }
    }
  }

  ShellPrintEx (-1, -1, L"\nTotal devices found: %d\n", (UINTN)DeviceCount);
}

/**
  Dumps the first 256 bytes of PCI configuration space for the given BDF.

  Displays the hex dump in rows of 16 bytes, annotated with offset.

  @param[in]  Shell     The shell protocol for console output.
  @param[in]  Bus       PCI bus number.
  @param[in]  Device    PCI device number.
  @param[in]  Function  PCI function number.
**/
VOID
PciDumpConfigSpace (
  IN EFI_SHELL_PROTOCOL  *Shell,
  IN UINT8               Bus,
  IN UINT8               Device,
  IN UINT8               Function
  )
{
  UINT8   Buffer[PCI_CONFIG_SPACE_SIZE];
  UINTN   Offset;
  UINTN   Index;
  UINT8   ByteVal;
  UINT8   HeaderType;
  UINT8   CapPtr;
  UINT8   NextCap;
  BOOLEAN IsPcie;

  //
  // Read the entire 256-byte configuration space byte by byte.
  //
  for (Offset = 0; Offset < PCI_CONFIG_SPACE_SIZE; Offset++) {
    Buffer[Offset] = PciReadConfigByte (Bus, Device, Function, (UINT8)Offset);
  }

  //
  // Print header.
  //
  ShellPrintEx (
    -1,
    -1,
    L"PCI Configuration Space Dump - BDF %02X:%02X:%02X\n",
    (UINTN)Bus,
    (UINTN)Device,
    (UINTN)Function
    );
  ShellPrintEx (
    -1,
    -1,
    L"Vendor ID: 0x%04X  Device ID: 0x%04X  Revision: 0x%02X  Class: 0x%06X\n\n",
    (UINTN)(Buffer[0] | (Buffer[1] << 8)),
    (UINTN)(Buffer[2] | (Buffer[3] << 8)),
    (UINTN)Buffer[8],
    (UINTN)(Buffer[9] | (Buffer[0x0A] << 8) | (Buffer[0x0B] << 16))
    );

  //
  // Print hex dump: 16 bytes per line.
  //
  ShellPrintEx (-1, -1, L"       +0 +1 +2 +3 +4 +5 +6 +7  +8 +9 +A +B +C +D +E +F\n");

  for (Offset = 0; Offset < PCI_CONFIG_SPACE_SIZE; Offset += 16) {
    ShellPrintEx (-1, -1, L"0x%02X: ", (UINTN)Offset);

    for (Index = 0; Index < 8; Index++) {
      ByteVal = Buffer[Offset + Index];
      ShellPrintEx (-1, -1, L" %02X", (UINTN)ByteVal);
    }

    ShellPrintEx (-1, -1, L"  ");

    for (Index = 8; Index < 16; Index++) {
      ByteVal = Buffer[Offset + Index];
      ShellPrintEx (-1, -1, L" %02X", (UINTN)ByteVal);
    }

    ShellPrintEx (-1, -1, L"\n");
  }

  //
  // Determine the Header Type (Type 0 / Type 1 / Type 2).
  //
  HeaderType = Buffer[PCI_HEADER_TYPE_OFFSET];

  ShellPrintEx (-1, -1, L"\n");

  switch (HeaderType & 0x7F) {
  case PCI_HEADER_TYPE_DEVICE:
    ShellPrintEx (
      -1,
      -1,
      L"Header Type: 0x%02X (Type 0 - PCI Endpoint)\n",
      (UINTN)HeaderType
      );
    break;

  case PCI_HEADER_TYPE_BRIDGE:
    ShellPrintEx (
      -1,
      -1,
      L"Header Type: 0x%02X (Type 1 - PCI-to-PCI Bridge)\n",
      (UINTN)HeaderType
      );
    break;

  case PCI_HEADER_TYPE_CARDBUS:
    ShellPrintEx (
      -1,
      -1,
      L"Header Type: 0x%02X (Type 2 - CardBus Bridge)\n",
      (UINTN)HeaderType
      );
    break;

  default:
    ShellPrintEx (
      -1,
      -1,
      L"Header Type: 0x%02X (Unknown)\n",
      (UINTN)HeaderType
      );
    break;
  }

  if ((HeaderType & PCI_HEADER_TYPE_MULTI) != 0) {
    ShellPrintEx (-1, -1, L"             (Multi-Function Device)\n");
  }

  //
  // Determine if the device is PCI or PCIe by checking for Cap ID 0x10.
  //
  IsPcie = FALSE;

  //
  // Capabilities Pointer is at offset 0x34 for both Type 0 and Type 1.
  //
  CapPtr = Buffer[PCI_CAP_PTR_OFFSET];

  while (CapPtr >= 0x40) {
    if (Buffer[CapPtr] == PCI_CAP_ID_PCI_EXPRESS) {
      IsPcie = TRUE;
      break;
    }

    NextCap = Buffer[CapPtr + 1];
    CapPtr  = NextCap;
  }

  if (IsPcie) {
    ShellPrintEx (-1, -1, L"PCI/PCIe: PCIe (PCI Express Capability found at 0x%02X)\n", (UINTN)CapPtr);
  } else {
    ShellPrintEx (-1, -1, L"PCI/PCIe: PCI (No PCI Express Capability found)\n");
  }
}

/**
  Parses a BDF string in the format "B:D:F" or "BB:DD:FF".

  @param[in]   BdfStr    The BDF string to parse.
  @param[out]  Bus       Parsed bus number.
  @param[out]  Device    Parsed device number.
  @param[out]  Function  Parsed function number.

  @retval TRUE   Parsing succeeded.
  @retval FALSE  Parsing failed (invalid format or out of range).
**/
BOOLEAN
ParseBdfString (
  IN  CONST CHAR16  *BdfStr,
  OUT UINT8         *Bus,
  OUT UINT8         *Device,
  OUT UINT8         *Function
  )
{
  UINTN   BusVal;
  UINTN   DevVal;
  UINTN   FunVal;

  //
  // Parse Bus (hex).
  //
  BusVal = StrHexToUintn (BdfStr);
  if (BusVal > PCI_MAX_BUS) {
    return FALSE;
  }

  //
  // Find the first colon separator.
  //
  while ((*BdfStr != L'\0') && (*BdfStr != L':')) {
    BdfStr++;
  }

  if (*BdfStr != L':') {
    return FALSE;
  }

  BdfStr++;

  //
  // Parse Device (hex).
  //
  DevVal = StrHexToUintn (BdfStr);
  if (DevVal > PCI_MAX_DEVICE) {
    return FALSE;
  }

  //
  // Find the second colon separator.
  //
  while ((*BdfStr != L'\0') && (*BdfStr != L':')) {
    BdfStr++;
  }

  if (*BdfStr != L':') {
    return FALSE;
  }

  BdfStr++;

  //
  // Parse Function (hex).
  //
  FunVal = StrHexToUintn (BdfStr);
  if (FunVal > PCI_MAX_FUNCTION) {
    return FALSE;
  }

  *Bus      = (UINT8)BusVal;
  *Device   = (UINT8)DevVal;
  *Function = (UINT8)FunVal;

  return TRUE;
}

/**
  The main entry point for the PciConfigApp Shell application.

  Usage:
    pci              - Scan all PCI/PCIe devices and print Vendor ID & Device ID.
    pci B:D:F        - Dump 256-byte config space for the specified BDF.

  @param[in]  Argc  Number of command-line arguments.
  @param[in]  Argv  Array of command-line argument strings.

  @retval SHELL_SUCCESS       The application executed successfully.
  @retval SHELL_INVALID_PARAMETER  A parameter was invalid.
**/
INTN
EFIAPI
ShellAppMain (
  IN UINTN   Argc,
  IN CHAR16  **Argv
  )
{
  EFI_STATUS          Status;
  EFI_SHELL_PROTOCOL  *Shell;
  UINT8               Bus;
  UINT8               Device;
  UINT8               Function;

  //
  // Locate the EFI Shell Protocol for console output.
  //
  Status = gBS->LocateProtocol (
                  &gEfiShellProtocolGuid,
                  NULL,
                  (VOID **)&Shell
                  );
  if (EFI_ERROR (Status)) {
    return SHELL_ABORTED;
  }

  //
  // No arguments: scan all devices.
  //
  if (Argc == 1) {
    PciScanAllDevices (Shell);
    return SHELL_SUCCESS;
  }

  //
  // One argument: expect B:D:F format.
  //
  if (Argc == 2) {
    if (!ParseBdfString (Argv[1], &Bus, &Device, &Function)) {
      ShellPrintEx (
        -1,
        -1,
        L"Error: Invalid BDF format. Use B:D:F (hex), e.g. 00:00:00\n"
        );
      return SHELL_INVALID_PARAMETER;
    }

    //
    // Check if the device exists.
    //
    if (!PciDeviceExists (Bus, Device, Function)) {
      ShellPrintEx (
        -1,
        -1,
        L"Error: No PCI device found at %02X:%02X:%02X\n",
        (UINTN)Bus,
        (UINTN)Device,
        (UINTN)Function
        );
      return SHELL_NOT_FOUND;
    }

    PciDumpConfigSpace (Shell, Bus, Device, Function);
    return SHELL_SUCCESS;
  }

  //
  // Too many arguments.
  //
  ShellPrintEx (
    -1,
    -1,
    L"Usage:\n"
    L"  pci              - Scan all PCI/PCIe devices\n"
    L"  pci B:D:F        - Dump config space for BDF (hex)\n"
    );
  return SHELL_INVALID_PARAMETER;
}
