#!/bin/sh
# Build the Componental-branded Dubby bootloaders with the 96-bit serial.
#   extdfu = DFU over the external USB pins (D29/D30) — what Dubby's USB-C is wired to; THIS is the production variant
#   intdfu = DFU over the Seed's own micro-USB — for bench testing a bare Seed2 DFM
set -e
cd "$(dirname "$0")"
STR='-DDSY_USB_DESC_MFR_STR=\"Componental\" -DDSY_USB_DESC_PRODUCT_STR=\"Dubby\"'
make -C libDaisy -j8
mkdir -p dubby-dist
make -C bootloader clean
make -C bootloader TARGET_SUFFIX="-extdfu-2000ms" EXTRA_C_DEFS="-DDSY_BOOT_TIMEOUT_MS=2000 -DDSY_DFU_USE_EXT_USB $STR"
cp bootloader/build/dsy_bootloader_v6_4-extdfu-2000ms.bin dubby-dist/dubby_bootloader_v6_4-extdfu-2000ms-uid96.bin
make -C bootloader clean
make -C bootloader TARGET_SUFFIX="-intdfu-2000ms" EXTRA_C_DEFS="$STR"
cp bootloader/build/dsy_bootloader_v6_4-intdfu-2000ms.bin dubby-dist/dubby_bootloader_v6_4-intdfu-2000ms-uid96.bin
ls -l dubby-dist/*.bin
