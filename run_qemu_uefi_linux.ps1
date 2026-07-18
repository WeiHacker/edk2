# ============================================================
#  QEMU + OVMF full UEFI boot flow for Linux
#
#  Firmware/BIOS phase:
#    SEC/PEI/DXE/BDS in OVMF. Press F2 or Esc during the boot
#    countdown to enter the OVMF Boot Manager / Firmware UI.
#
#  OS loader phase:
#    OVMF loads the UEFI boot option from a Linux ISO or disk image,
#    such as \EFI\BOOT\BOOTX64.EFI, grubx64.efi, shimx64.efi, or
#    systemd-bootx64.efi.
#
#  OS runtime phase:
#    The Linux loader starts the kernel/initrd and switches into Linux.
# ============================================================
<#
.SYNOPSIS
    Start a Linux ISO or Linux disk image through QEMU + OVMF UEFI.

.DESCRIPTION
    This script demonstrates a complete UEFI-style boot flow:
    QEMU -> OVMF firmware -> Boot Manager / Firmware UI -> OS loader -> Linux runtime.

    Run it from PowerShell in the edk2 workspace:
        Set-Location D:\shen_work\Uefi_Project\edk2

    If script execution is restricted, use:
        powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 <options>

.PARAMETER LinuxIso
    Path to a Linux installer/live ISO. OVMF loads the ISO's UEFI boot loader.

.PARAMETER LinuxDisk
    Path to an installed Linux disk image or a new image to create with -CreateDiskSize.

.PARAMETER CreateDiskSize
    Create LinuxDisk if it does not exist. Example: 30G.

.PARAMETER InstallUbuntu
    Boot the Ubuntu installer ISO and attach the default persistent qcow2 disk.

.PARAMETER BootUbuntuDisk
    Boot only from the default persistent Ubuntu qcow2 disk.

.PARAMETER EnterSetup
    Start OVMF with a longer boot menu timeout so you can enter firmware UI.
    This OVMF build uses F2 or Esc, not Delete.

.PARAMETER ResetNvram
    Reset the writable OVMF variable store from OVMF_VARS.fd.

.PARAMETER DryRun
    Print the generated QEMU command line without launching QEMU.

.PARAMETER Help
    Print concise usage examples.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -Help

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -EnterSetup -ResetNvram

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -InstallUbuntu

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -BootUbuntuDisk

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -LinuxIso .\ubuntu\ubuntu-20.04.6-live-server-amd64.iso -LinuxDisk .\ubuntu\ubuntu-20.04.qcow2 -CreateDiskSize 40G

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -LinuxDisk .\ubuntu\ubuntu-20.04.qcow2
#>

[CmdletBinding()]
param(
    [string]$LinuxIso = "",
    [string]$LinuxDisk = "",
    [ValidateSet("auto", "raw", "qcow2", "vhdx", "vmdk")]
    [string]$DiskFormat = "auto",
    [string]$CreateDiskSize = "",
    [int]$MemoryMB = 4096,
    [int]$CpuCount = 2,
    [switch]$InstallUbuntu,
    [switch]$BootUbuntuDisk,
    [switch]$EnterSetup,
    [switch]$ResetNvram,
    [switch]$NoNetwork,
    [switch]$NoDebugLog,
    [switch]$DryRun,
    [switch]$Help
)

$ErrorActionPreference = "Stop"

# ---- Paths ----
$QEMU         = "D:\qemu\qemu-system-x86_64.exe"
$QEMU_IMG     = "D:\qemu\qemu-img.exe"
$EDK2         = "D:\shen_work\Uefi_Project\edk2"
$OVMF_DIR     = "$EDK2\Build\OvmfX64\DEBUG_VS2022\FV"
$OVMF_CODE    = "$OVMF_DIR\OVMF_CODE.fd"
$OVMF_VARS    = "$OVMF_DIR\OVMF_VARS.fd"
$OVMF_VARS_RW = "$EDK2\ovmf_vars_linux.fd"
$DBGLOG       = "$EDK2\debug_uefi_linux.log"
$DEFAULT_ISO  = "$EDK2\ubuntu\ubuntu-20.04.6-live-server-amd64.iso"
$DEFAULT_DISK = "$EDK2\ubuntu\ubuntu-20.04.qcow2"
$DEFAULT_DISK_SIZE = "40G"

function Show-Usage {
    Write-Host @'
QEMU + OVMF Linux boot script

Run from PowerShell:
  Set-Location D:\shen_work\Uefi_Project\edk2

Show this help:
  powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -Help

Enter OVMF firmware UI / boot manager:
  powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -EnterSetup -ResetNvram

First install: boot Ubuntu installer and attach a persistent qcow2 disk:
  powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -InstallUbuntu

After installation: boot from the installed Ubuntu qcow2 disk:
  powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -BootUbuntuDisk

Default behavior:
  powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1

  If .\ubuntu\ubuntu-20.04.qcow2 does not exist, default behavior enters install mode.
  If .\ubuntu\ubuntu-20.04.qcow2 exists, default behavior boots from that disk.

Boot Ubuntu ISO explicitly:
  powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -LinuxIso .\ubuntu\ubuntu-20.04.6-live-server-amd64.iso -LinuxDisk .\ubuntu\ubuntu-20.04.qcow2 -CreateDiskSize 40G

Install Linux from ISO into a new qcow2 disk:
  powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -LinuxIso .\ubuntu\ubuntu-20.04.6-live-server-amd64.iso -LinuxDisk .\ubuntu\ubuntu-20.04.qcow2 -CreateDiskSize 40G

Boot from the installed Linux disk later:
  powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -LinuxDisk .\ubuntu\ubuntu-20.04.qcow2

Preview the QEMU command without launching:
  powershell -ExecutionPolicy Bypass -File .\run_qemu_uefi_linux.ps1 -DryRun

Notes:
  - Press F2 or Esc during the OVMF boot countdown to enter the firmware UI.
  - This OVMF build does not register Delete as the setup hotkey by default.
  - The default ISO is .\ubuntu\ubuntu-20.04.6-live-server-amd64.iso.
  - The persistent disk is .\ubuntu\ubuntu-20.04.qcow2.
  - Do not use -ResetNvram every time. It is only for intentionally clearing UEFI NVRAM.
'@
}

if ($Help) {
    Show-Usage
    exit 0
}

function Resolve-DiskFormat {
    param(
        [string]$Path,
        [string]$RequestedFormat
    )

    if ($RequestedFormat -ne "auto") {
        return $RequestedFormat
    }

    switch ([System.IO.Path]::GetExtension($Path).ToLowerInvariant()) {
        ".qcow2" { return "qcow2" }
        ".vhdx"  { return "vhdx" }
        ".vmdk"  { return "vmdk" }
        default  { return "raw" }
    }
}

function Add-IfPresent {
    param(
        [System.Collections.Generic.List[string]]$List,
        [string[]]$Values
    )

    foreach ($Value in $Values) {
        $List.Add($Value)
    }
}

if ($InstallUbuntu -and $BootUbuntuDisk) {
    Write-Error "-InstallUbuntu and -BootUbuntuDisk are mutually exclusive."
    exit 1
}

# ---- Step 1: Verify host tools and firmware ----
Write-Host "[1/4] Checking QEMU and OVMF..." -ForegroundColor Cyan

if (-not (Test-Path $QEMU)) {
    Write-Error "QEMU not found: $QEMU"
    exit 1
}
if (-not (Test-Path $OVMF_CODE)) {
    Write-Error "OVMF_CODE.fd not found: $OVMF_CODE"
    Write-Host "Build first: build -p OvmfPkg\OvmfPkgX64.dsc -t VS2022 -b DEBUG -a X64"
    exit 1
}
if (-not (Test-Path $OVMF_VARS)) {
    Write-Error "OVMF_VARS.fd not found: $OVMF_VARS"
    Write-Host "Build first: build -p OvmfPkg\OvmfPkgX64.dsc -t VS2022 -b DEBUG -a X64"
    exit 1
}

if (-not $EnterSetup) {
    if ($InstallUbuntu) {
        if ([string]::IsNullOrWhiteSpace($LinuxIso)) {
            $LinuxIso = $DEFAULT_ISO
        }
        if ([string]::IsNullOrWhiteSpace($LinuxDisk)) {
            $LinuxDisk = $DEFAULT_DISK
        }
        if ([string]::IsNullOrWhiteSpace($CreateDiskSize)) {
            $CreateDiskSize = $DEFAULT_DISK_SIZE
        }
    } elseif ($BootUbuntuDisk) {
        if ([string]::IsNullOrWhiteSpace($LinuxDisk)) {
            $LinuxDisk = $DEFAULT_DISK
        }
        $LinuxIso = ""
    } elseif ([string]::IsNullOrWhiteSpace($LinuxIso) -and [string]::IsNullOrWhiteSpace($LinuxDisk)) {
        if (Test-Path $DEFAULT_DISK) {
            $LinuxDisk = $DEFAULT_DISK
            Write-Host "      Using default installed disk: $LinuxDisk" -ForegroundColor Yellow
        } elseif (Test-Path $DEFAULT_ISO) {
            $LinuxIso        = $DEFAULT_ISO
            $LinuxDisk       = $DEFAULT_DISK
            $CreateDiskSize  = $DEFAULT_DISK_SIZE
            Write-Host "      No default disk found. Entering Ubuntu install mode." -ForegroundColor Yellow
            Write-Host "      Default ISO : $LinuxIso" -ForegroundColor Yellow
            Write-Host "      Default disk: $LinuxDisk ($CreateDiskSize)" -ForegroundColor Yellow
        } else {
            Write-Error "Provide -LinuxIso or -LinuxDisk. Use -EnterSetup when you only want to enter OVMF setup/boot manager."
            Write-Host "Default ISO was not found: $DEFAULT_ISO"
            exit 1
        }
    }
}

if (-not [string]::IsNullOrWhiteSpace($LinuxIso)) {
    if (-not (Test-Path $LinuxIso)) {
        Write-Error "Linux ISO not found: $LinuxIso"
        exit 1
    }
    $LinuxIso = (Resolve-Path -LiteralPath $LinuxIso).Path
}

if (-not [string]::IsNullOrWhiteSpace($LinuxDisk)) {
    if (-not (Test-Path $LinuxDisk)) {
        if ([string]::IsNullOrWhiteSpace($CreateDiskSize)) {
            Write-Error "Linux disk not found: $LinuxDisk"
            Write-Host "To create one, add for example: -CreateDiskSize 30G"
            exit 1
        }
        if ((-not $DryRun) -and (-not (Test-Path $QEMU_IMG))) {
            Write-Error "qemu-img not found: $QEMU_IMG"
            exit 1
        }

        $NewDiskFormat = Resolve-DiskFormat -Path $LinuxDisk -RequestedFormat $DiskFormat
        if ($DryRun) {
            Write-Host "      Would create $NewDiskFormat disk: $LinuxDisk ($CreateDiskSize)" -ForegroundColor Yellow
        } else {
            Write-Host "      Creating $NewDiskFormat disk: $LinuxDisk ($CreateDiskSize)" -ForegroundColor Yellow
            & $QEMU_IMG create -f $NewDiskFormat $LinuxDisk $CreateDiskSize
        }
    }

    if (Test-Path $LinuxDisk) {
        $LinuxDisk = (Resolve-Path -LiteralPath $LinuxDisk).Path
    } elseif ([System.IO.Path]::IsPathRooted($LinuxDisk)) {
        $LinuxDisk = [System.IO.Path]::GetFullPath($LinuxDisk)
    } else {
        $LinuxDisk = [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $LinuxDisk))
    }
}

Write-Host "      OK" -ForegroundColor Green

# ---- Step 2: Prepare writable UEFI variable store ----
Write-Host "[2/4] Preparing UEFI variable store..." -ForegroundColor Cyan

if ($ResetNvram -or -not (Test-Path $OVMF_VARS_RW)) {
    Copy-Item -LiteralPath $OVMF_VARS -Destination $OVMF_VARS_RW -Force
    Write-Host "      Reset NVRAM from OVMF_VARS.fd" -ForegroundColor Yellow
} else {
    Write-Host "      Reusing existing NVRAM: $OVMF_VARS_RW" -ForegroundColor Green
}

if ((Test-Path $DBGLOG) -and -not $NoDebugLog) {
    Remove-Item -LiteralPath $DBGLOG -Force
}

# ---- Step 3: Build QEMU arguments ----
Write-Host "[3/4] Building QEMU command line..." -ForegroundColor Cyan

$Args = [System.Collections.Generic.List[string]]::new()
Add-IfPresent $Args @(
    "-machine", "q35,smm=on",
    "-m", $MemoryMB.ToString(),
    "-smp", $CpuCount.ToString(),
    "-drive", "if=pflash,format=raw,unit=0,readonly=on,file=$OVMF_CODE",
    "-drive", "if=pflash,format=raw,unit=1,file=$OVMF_VARS_RW",
    "-device", "qemu-xhci",
    "-device", "usb-kbd",
    "-device", "usb-tablet"
)

if ($EnterSetup) {
    Add-IfPresent $Args @("-boot", "menu=on,splash-time=10000")
} elseif (-not [string]::IsNullOrWhiteSpace($LinuxIso)) {
    Add-IfPresent $Args @("-boot", "order=d,menu=on,splash-time=5000")
} else {
    Add-IfPresent $Args @("-boot", "order=c,menu=on,splash-time=5000")
}

if (-not [string]::IsNullOrWhiteSpace($LinuxDisk)) {
    $ResolvedFormat = Resolve-DiskFormat -Path $LinuxDisk -RequestedFormat $DiskFormat
    Add-IfPresent $Args @("-drive", "file=$LinuxDisk,if=ide,format=$ResolvedFormat,media=disk")
    Write-Host "      Linux disk : $LinuxDisk ($ResolvedFormat, IDE/AHCI)" -ForegroundColor Green
}

if (-not [string]::IsNullOrWhiteSpace($LinuxIso)) {
    Add-IfPresent $Args @("-drive", "file=$LinuxIso,if=ide,media=cdrom,readonly=on")
    Write-Host "      Linux ISO  : $LinuxIso" -ForegroundColor Green
}

if (-not $NoNetwork) {
    Add-IfPresent $Args @("-netdev", "user,id=net0", "-device", "virtio-net-pci,netdev=net0")
}

if (-not $NoDebugLog) {
    Add-IfPresent $Args @("-debugcon", "file:$DBGLOG", "-global", "isa-debugcon.iobase=0x402")
}

Write-Host ""
Write-Host "  UEFI boot flow:" -ForegroundColor Cyan
Write-Host "    1. OVMF firmware SEC/PEI/DXE/BDS"
Write-Host "    2. Optional setup/boot manager: press F2 or Esc during countdown"
Write-Host "    3. OS loader from Linux ISO/disk: BOOTX64.EFI / GRUB / shim / systemd-boot"
Write-Host "    4. Linux kernel + initrd -> OS runtime"
Write-Host ""
Write-Host "  Note: this OVMF build registers F2/Esc for firmware UI; Delete is not registered by default." -ForegroundColor Yellow
Write-Host ""

# ---- Step 4: Launch QEMU ----
if ($DryRun) {
    Write-Host "[4/4] Dry run. QEMU was not launched." -ForegroundColor Yellow
    Write-Host ""
    Write-Host $QEMU
    foreach ($Arg in $Args) {
        Write-Host "  $Arg"
    }
    exit 0
}

Write-Host "[4/4] Launching QEMU..." -ForegroundColor Cyan
& $QEMU @Args

Write-Host ""
Write-Host "QEMU exited." -ForegroundColor Green
if (-not $NoDebugLog) {
    Write-Host "Debug log saved to: $DBGLOG" -ForegroundColor Yellow
}
