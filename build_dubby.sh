#!/bin/sh
# Build the Componental-branded Dubby bootloaders with the 96-bit serial.
#   extdfu = DFU over the external USB pins (D29/D30) — what Dubby's USB-C is wired to; THIS is the production variant
#   intdfu = DFU over the Seed's own micro-USB — for bench testing a bare Seed2 DFM
#
# The Dubby defines are applied to the extdfu variant and the intdfu bench variant
# ONLY; upstream builds are unchanged. Encoder-hold DFU, adaptive DFU pacing, 4 KB chunks.
# The stay-in-DFU-after-interrupted-download marker (DUBBY_STAY_IN_DFU_IF_INCOMPLETE,
# see shared/dubby_hardening.h) is deliberately OFF: it left units stuck in DFU, and the
# encoder hold is the recovery path after an interrupted download.
set -e
cd "$(dirname "$0")"
STR='-DDSY_USB_DESC_MFR_STR=\"Componental\" -DDSY_USB_DESC_PRODUCT_STR=\"Dubby\"'
# USBD_DFU_XFER_SIZE=4096: a QSPI write costs the same ~36 ms for 1 KB or 4 KB, so
# 4x fewer chunks is 4x faster (382 kB: 30 s -> 10 s, measured 3 Sep 2026). Slot buffers are 8 KB.
HARDENING='-DDUBBY_ENCODER_DFU=1 -DDUBBY_DFU_POLL_TIMEOUTS=1 -DUSBD_DFU_XFER_SIZE=4096'
make -C libDaisy -j8
mkdir -p dubby-dist
make -C bootloader clean
make -C bootloader TARGET_SUFFIX="-extdfu-2000ms" EXTRA_C_DEFS="-DDSY_BOOT_TIMEOUT_MS=2000 -DDSY_DFU_USE_EXT_USB $STR $HARDENING"
cp bootloader/build/dsy_bootloader_v6_4-extdfu-2000ms.bin dubby-dist/dubby_bootloader_v6_4-extdfu-2000ms-uid96-encoder-pacing-xfer4096.bin
cp bootloader/build/dsy_bootloader_v6_4-extdfu-2000ms.elf dubby-dist/dubby_bootloader_v6_4-extdfu-2000ms-uid96-encoder-pacing-xfer4096.elf
make -C bootloader clean
make -C bootloader TARGET_SUFFIX="-intdfu-2000ms" EXTRA_C_DEFS="$STR"
cp bootloader/build/dsy_bootloader_v6_4-intdfu-2000ms.bin dubby-dist/dubby_bootloader_v6_4-intdfu-2000ms-uid96.bin
# Bench variant: intdfu + the same Dubby defines, for testing the hardening
# on a bare Seed2 DFM over its micro-USB before touching a Dubby. Not for production.
make -C bootloader clean
make -C bootloader TARGET_SUFFIX="-intdfu-2000ms" EXTRA_C_DEFS="$STR $HARDENING"
cp bootloader/build/dsy_bootloader_v6_4-intdfu-2000ms.bin dubby-dist/dubby_bootloader_v6_4-intdfu-2000ms-uid96-encoder-pacing-xfer4096-bench.bin
cp bootloader/build/dsy_bootloader_v6_4-intdfu-2000ms.elf dubby-dist/dubby_bootloader_v6_4-intdfu-2000ms-uid96-encoder-pacing-xfer4096-bench.elf
ls -l dubby-dist/*.bin
