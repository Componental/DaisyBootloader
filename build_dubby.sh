#!/bin/sh
# Build the Componental-branded Dubby bootloader (internal-USB DFU, 2000 ms boot wait).
# Output: dubby-dist/dubby_bootloader_v6_4-intdfu-2000ms-uid96.bin
set -e
cd "$(dirname "$0")"
make -C libDaisy -j8
make -C bootloader clean
make -C bootloader TARGET_SUFFIX="-intdfu-2000ms" \
  EXTRA_C_DEFS='-DDSY_USB_DESC_MFR_STR=\"Componental\" -DDSY_USB_DESC_PRODUCT_STR=\"Dubby\"'
mkdir -p dubby-dist
cp bootloader/build/dsy_bootloader_v6_4-intdfu-2000ms.bin dubby-dist/dubby_bootloader_v6_4-intdfu-2000ms-uid96.bin
ls -l dubby-dist/
