# ============================================================
#  QEMU + OVMF boot HelloWorld.efi
#  ESP with UEFI removable media layout -> auto-boot BOOTX64.EFI
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
$BgrtLogoDumpApp = "$EDK2\Build\StudyPkg\DEBUG_VS2022\X64\BgrtLogoDumpApp.efi"
$LspciApp = "$EDK2\Build\StudyPkg\DEBUG_VS2022\X64\LspciApp.efi"
$RU = "$EDK2\Build\Shell\DEBUG_VS2022\X64\RU.efi"
$AcpiDumpApp = "$EDK2\Build\StudyPkg\DEBUG_VS2022\X64\AcpiDumpApp.efi"
$SmbiosDumpApp = "$EDK2\Build\StudyPkg\DEBUG_VS2022\X64\SmbiosDumpApp.efi"
$ESP      = "$EDK2\esp_image"

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

Copy-Item $BgrtLogoDumpApp "$ESP\BgrtLogoDumpApp.efi"
Write-Host "      -> BgrtLogoDumpApp.efi (custom app)" -ForegroundColor Green

Copy-Item $ShellRtcCmd "$ESP\ShellRtcCmd.efi"
Write-Host "      -> ShellRtcCmd.efi (custom command)" -ForegroundColor Green

Copy-Item $AcpiDumpApp "$ESP\AcpiDumpApp.efi"
Write-Host "      -> AcpiDumpApp.efi (custom app)" -ForegroundColor Green

Copy-Item $PciConfigApp "$ESP\PciConfigApp.efi"
Write-Host "      -> PciConfigApp.efi (custom app)" -ForegroundColor Green

Copy-Item $LspciApp "$ESP\LspciApp.efi"
Write-Host "      -> LspciApp.efi (custom app)" -ForegroundColor Green

Copy-Item $SmbiosDumpApp "$ESP\SmbiosDumpApp.efi"
Write-Host "      -> SmbiosDumpApp.efi (custom app)" -ForegroundColor Green

Copy-Item $RU "$ESP\RU.efi"
Write-Host "      -> RU.efi (custom app)" -ForegroundColor Green

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
Write-Host "      OK" -ForegroundColor Green

# ---- Step 3: Launch QEMU ----
Write-Host "[3/4] Launching QEMU..." -ForegroundColor Cyan
Write-Host ""
Write-Host "  OVMF will auto-boot EFI\BOOT\BOOTX64.EFI (UEFI Spec section 3.4.1.1)"
Write-Host "  Manual run from UEFI Shell:"
Write-Host "    Shell> fs0:"
Write-Host "    fs0:> HelloWorld.efi"
Write-Host "    fs0:> MyDriver.efi"
Write-Host "    fs0:> ShellRtcCmd.efi"
Write-Host "    fs0:> PciConfigApp.efi"
Write-Host "    fs0:> RU.efi"
Write-Host "    fs0:> SmbiosDumpApp.efi"
Write-Host "    fs0:> BgrtLogoDumpApp.efi"
Write-Host "    fs0:> AcpiDumpApp.efi"
Write-Host "    fs0:> LspciApp.efi"
Write-Host "    fs0:> Shell.efi"
Write-Host "    fs0:> load MyWizardDriver.efi"
Write-Host "    fs0:> connect"
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
