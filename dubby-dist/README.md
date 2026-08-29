# Dubby bootloader builds — 29 Aug 2026 (spike, NOT flashed to any device)

| File | What |
|---|---|
| `dubby_bootloader_v6_4-intdfu-2000ms-uid96.bin` | DaisyBootloader v6.4 + 96-bit serial + strings "Componental"/"Dubby". Reproduce with `./build_dubby.sh`. |
| `dubby_bootloader_v6_4-extdfu-2000ms-uid96-hardening-v3.bin` (+ `.elf`) | 30 Aug, branch `dubby-bootloader-hardening`: extdfu-uid96 + stay-in-DFU-if-incomplete + encoder-hold DFU + worst-case poll timeouts. NOT flashed. Test plan: DEV-NOTES-MAREK/2026-08-30-bootloader-encoder-dfu.md §8. |
| `dubby_bootloader_v6_4-extdfu-2000ms-uid96.bin` | production candidate as flashed 29 Aug (rollback image for the hardening build) |
| `dsy_bootloader_v6_4-intdfu-2000ms-baseline-localbuild.bin` | unpatched, same toolchain — flash THIS first to prove the toolchain, then the one above |

Built with Arm GNU Toolchain 15.3.Rel1 against libDaisy 24be4af (the bootloader's own pin, left
pristine — the serial-size override lives in shared/usbd_desc.c). Electrosmith's official v6.4 is
130,188 bytes; these are ~123 KB — different compiler version, same source.

This folder is deliberately NOT dist/: ci/build_release.sh runs `rm dist/*` and would wipe it.

Flash: `dfu-util -a 0 -s 0x08000000:leave -D <file>` with the Seed in ROM DFU (hold BOOT at
power-on) — the ROM DFU is the recovery path and works whatever is in flash.
