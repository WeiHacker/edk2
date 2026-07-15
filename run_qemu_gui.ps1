# ============================================================
#  QEMU GUI mode - debug output in this terminal, UEFI console in GUI window
# ============================================================

$ErrorActionPreference = "Stop"

# ---- Paths ----
$QEMU     = "D:\qemu\qemu-system-x86_64.exe"
$OVMF     = "D:\shen_work\Uefi_Project\edk2\Build\OvmfX64\DEBUG_VS2022\FV\OVMF.fd"
$EDK2     = "D:\shen_work\Uefi_Project\edk2"
$HELLO    = "$EDK2\Build\EmulatorX64\DEBUG_VS2022\X64\HelloWorld.efi"
$MyDriver    = "$EDK2\Build\StudyPkg\DEBUG_VS2022\X64\MyDriver.efi"
$ShellRtcCmd = "$EDK2\Build\StudyPkg\DEBUG_VS2022\X64\ShellRtcCmd.efi"
$PciConfigApp = "$EDK2\Build\StudyPkg\DEBUG_VS2022\X64\PciConfigApp.efi"
$MyWizDriver = "$EDK2\Build\MyWizardDriver\DEBUG_VS2022\X64\MyWizardDriver.efi"
$Shell = "$EDK2\Build\Shell\DEBUG_VS2022\X64\ShellPkg\Application\Shell\Shell\DEBUG\Shell.efi"
$ESP      = "$EDK2\esp_image"
$DBGLOG   = "$EDK2\debug.log"

# ---- Step 1: Build ESP directory ----
Write-Host "[1/4] Building ESP directory..." -ForegroundColor Cyan

if (Test-Path $ESP) {
    Remove-Item -Recurse -Force $ESP
}
New-Item -ItemType Directory -Path "$ESP\EFI\BOOT" -Force | Out-Null

Copy-Item $HELLO "$ESP\EFI\BOOT\BOOTX64.EFI"
Write-Host "      -> EFI\BOOT\BOOTX64.EFI (auto-boot)" -ForegroundColor Green

Copy-Item $HELLO "$ESP\HelloWorld.efi"
Write-Host "      -> HelloWorld.efi (manual run)" -ForegroundColor Green

Copy-Item $MyDriver "$ESP\MyDriver.efi"
Write-Host "      -> MyDriver.efi (custom driver)" -ForegroundColor Green

Copy-Item $MyWizDriver "$ESP\MyWizardDriver.efi"
Write-Host "      -> MyWizardDriver.efi (wizard driver)" -ForegroundColor Green

Copy-Item $ShellRtcCmd "$ESP\ShellRtcCmd.efi"
Write-Host "      -> ShellRtcCmd.efi (custom command)" -ForegroundColor Green

Copy-Item $PciConfigApp "$ESP\PciConfigApp.efi"
Write-Host "      -> PciConfigApp.efi (custom app)" -ForegroundColor Green

Copy-Item $Shell "$ESP\Shell.efi"
Write-Host "      -> Shell.efi (UEFI Shell)" -ForegroundColor Green

# ---- Step 2: Verify ----
Write-Host "[2/4] Checking prerequisites..." -ForegroundColor Cyan

if (-not (Test-Path $QEMU)) {
    Write-Error "QEMU not found: $QEMU"
    exit 1
}
if (-not (Test-Path $OVMF)) {
    Write-Error "OVMF not found: $OVMF"
    Write-Host "Build first: build -p OvmfPkg\OvmfPkgX64.dsc -t VS2022 -b DEBUG -a X64"
    exit 1
}

# Remove old debug log
if (Test-Path $DBGLOG) {
    Remove-Item -Force $DBGLOG
}

Write-Host "      OK" -ForegroundColor Green

# ---- Step 3: Launch QEMU (GUI mode) ----
Write-Host "[3/4] Launching QEMU (GUI mode)..." -ForegroundColor Cyan
Write-Host ""
Write-Host "  +----------|----------+"
Write-Host "  | QEMU GUI | Terminal |"
Write-Host "  +----------|----------+"
Write-Host "  | UEFI     | DEBUG    |"
Write-Host "  | Console  | output   |"
Write-Host "  |          | (0x402)  |"
Write-Host "  +----------|----------+"
Write-Host "  | Interact | Watch    |"
Write-Host "  | with     | driver   |"
Write-Host "  | Shell    | messages |"
Write-Host "  +----------+----------+"
Write-Host ""
Write-Host "  Manual run in UEFI Shell:"
Write-Host "    Shell> fs0:"
Write-Host "    fs0:> load MyWizardDriver.efi"
Write-Host "    fs0:> reconnect -r"
Write-Host ""
Write-Host "============================================================"
Write-Host ""

& $QEMU `
    -bios $OVMF `
    -drive "file=fat:rw:$ESP,format=raw,media=disk" `
    -net none `
    -m 512 `
    -debugcon stdio `
    -global isa-debugcon.iobase=0x402

# ---- Step 4: Cleanup ----
Write-Host ""
Write-Host "[4/4] Cleaning up..." -ForegroundColor Cyan
Remove-Item -Recurse -Force $ESP
Write-Host "      Done." -ForegroundColor Green
