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
Write-Host ""

& $QEMU `
    -bios $OVMF `
    -drive "file=fat:rw:$ESP,format=raw,media=disk" `
    -net none `
    -m 512

# ---- Step 4: Cleanup ----
Write-Host ""
Write-Host "[4/4] Cleaning up..." -ForegroundColor Cyan
Remove-Item -Recurse -Force $ESP
Write-Host "      Done." -ForegroundColor Green
