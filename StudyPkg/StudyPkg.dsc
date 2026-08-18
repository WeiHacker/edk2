## @file
#  Study Package - Platform Description File.
#
#  This DSC file is used to build the MyDriver UEFI Driver for study purposes.
#  It maps library classes to their instances and lists the modules to build.
#
#  Copyright (c) 2026, Study. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = StudyPkg
  PLATFORM_GUID                  = B348D7C3-4A9F-4B8C-8A1E-2F3D4E5A6B7C
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/StudyPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|AARCH64|ARM|RISCV64|LOONGARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  ShellCEntryLib|ShellPkg/Library/UefiShellCEntryLib/UefiShellCEntryLib.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf

[Components]
  ##
  # MyDriver - Standard UEFI Driver Model example.
  ##
  StudyPkg/MyDriver/MyDriver.inf

  ##
  # ShellRtcCmd - Shell dynamic command to read CMOS RTC via I/O ports.
  ##
  StudyPkg/ShellRtcCmd/ShellRtcCmd.inf

  ##
  # PciConfigApp - PCI/PCIe configuration space viewer via I/O ports.
  ##
  StudyPkg/PciConfigApp/PciConfigApp.inf

  ##
  # LspciApp - lspci-like utility with verbose, hex dump, and tree view.
  ##
  StudyPkg/LspciApp/LspciApp.inf

  ##
  # SmbiosDumpApp - SMBIOS EPS viewer with custom type installation.
  ##
  StudyPkg/SmbiosDumpApp/SmbiosDumpApp.inf

  ##
  # BgrtLogoDumpApp - ACPI XSDT walker and BGRT logo exporter.
  ##
  StudyPkg/BgrtLogoDumpApp/BgrtLogoDumpApp.inf

  ##
  # Homework4SwSmiTriggerApp - Software SMI trigger application.
  ##
  StudyPkg/Homework4SwSmiTriggerApp/Homework4SwSmiTriggerApp.inf

[BuildOptions]
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
