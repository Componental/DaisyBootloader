# Dubby bootloader builds — 29 Aug 2026 (spike, NOT flashed to any device)

| File | What |
|---|---|
| `dubby_bootloader_v6_4-intdfu-2000ms-uid96.bin` | DaisyBootloader v6.4 + 96-bit serial + strings "Componental"/"Dubby". Reproduce with `./build_dubby.sh`. |
| `dubby_bootloader_v6_4-extdfu-2000ms-uid96-hardening-v4.bin` (+ `.elf`) | 30 Aug, branch `dubby-bootloader-hardening`: extdfu-uid96 + stay-in-DFU-if-incomplete + encoder-hold DFU + worst-case poll timeouts + marker cleared in the DFU leave hook (v4 fix). Production candidate, NOT yet tested on a Dubby. Test plan §8.4, bench results §9 in DEV-NOTES-MAREK/2026-08-30-bootloader-encoder-dfu.md. |
| `dubby_bootloader_v6_4-intdfu-2000ms-uid96-hardening-v4-bench.bin` (+ `.elf`) | same hardening on the intdfu (micro-USB) variant, for the bare Seed2 bench. Passed the bench 30 Aug; on the spare Seed2 now. |
| `bench/blink/` (repo) | QSPI blink test app used for the bench (`blink_qspi_padded427k.bin` = padded to dubsiren size). |
| `*-hardening-noencoder*.bin` | 30 Aug 10:00, parallel session: same tree without DUBBY_ENCODER_DFU (contains the v4 leave fix). See that session's notes. |
| `dubby_bootloader_v6_4-extdfu-2000ms-uid96.bin` | production candidate as flashed 29 Aug (rollback image for the hardening build) |
| `dsy_bootloader_v6_4-intdfu-2000ms-baseline-localbuild.bin` | unpatched, same toolchain — flash THIS first to prove the toolchain, then the one above |

Built with Arm GNU Toolchain 15.3.Rel1 against libDaisy 24be4af (the bootloader's own pin, left
pristine — the serial-size override lives in shared/usbd_desc.c). Electrosmith's official v6.4 is
130,188 bytes; these are ~123 KB — different compiler version, same source.

This folder is deliberately NOT dist/: ci/build_release.sh runs `rm dist/*` and would wipe it.

Flash: `dfu-util -a 0 -s 0x08000000:leave -D <file>` with the Seed in ROM DFU (hold BOOT at
power-on) — the ROM DFU is the recovery path and works whatever is in flash.
