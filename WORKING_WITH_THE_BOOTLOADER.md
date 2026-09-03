# Working with the Dubby bootloader

This is the Componental fork of DaisyBootloader. It carries the Dubby-specific
changes on top of upstream electro-smith: a full 96-bit UID serial, and a set
of DFU hardening fixes for reliable field firmware updates.

## Remotes

- `origin` → `electro-smith/DaisyBootloader` — upstream. Never push here.
- `componental` → `Componental/DaisyBootloader` — our fork. All branches and
  PRs live here.

## libDaisy

Plain, unforked electro-smith submodule (`libDaisy/`, currently pinned at
`v8.1.0-10-g24be4afe`). All Dubby-specific logic lives in this repo
(`shared/`, `bootloader/`), not in libDaisy. After cloning or switching
branches:

```sh
git submodule update --init --recursive
make -C libDaisy -j8
```

If you get stale/weird link errors, this is the first thing to check —
building against a `libdaisy.a` older than the submodule pointer is the
most common cause.

## Getting the code

```sh
git clone --recursive git@github.com:Componental/DaisyBootloader.git
cd DaisyBootloader
# or, in an existing clone:
git fetch componental
git checkout <branch-or-PR-branch>
git submodule update --init --recursive
```

**Before building, check what your branch is based on** (see "Branch
hygiene" below) — `componental/main` does not always have the latest
hardening merged in.

## Building

```sh
./build_dubby.sh
```

This builds the Componental/Dubby variants and drops them in `dubby-dist/`:

| Variant | What | Use |
|---|---|---|
| `*-extdfu-*` | DFU over Dubby's USB-C (external USB pins) | **Production** — this is what ships |
| `*-intdfu-*` | DFU over the Seed's own micro-USB | Bench-testing a bare Seed2 DFM |
| `*-intdfu-*-bench` | intdfu + hardening defines | Testing hardening on a bare Seed2 before touching a Dubby |

The hardening defines (`DUBBY_STAY_IN_DFU_IF_INCOMPLETE`,
`DUBBY_ENCODER_DFU`, `DUBBY_DFU_POLL_TIMEOUTS`, `USBD_DFU_VENDOR_EXIT_ENABLED`)
are applied to the extdfu variant only — see `shared/dubby_hardening.h` for
what each one does. Upstream/default builds are unaffected.

VS Code: `.vscode/tasks.json` has a default "Build Bootloader" task, but it's
plain `make clean && make` in `bootloader/` with none of the Dubby defines —
**it will not produce a uid96/hardening build.** Use the integrated terminal
and run `./build_dubby.sh` directly until we add a proper task for it.

## Flashing

Always via ROM DFU (the STM32's built-in system bootloader) — the recovery
path, works no matter what's currently in flash:

```sh
# hold BOOT, tap RESET to enter ROM DFU
dfu-util -l | grep 0483:df11        # confirm the device is there

./dubby-dist/flash_bootloader.sh dubby-dist/<file>.bin
# equivalent to:
dfu-util -a 0 -s 0x08000000:leave -D dubby-dist/<file>.bin
```

`dubby-dist/check_seed.sh` is handy afterwards to confirm the device came
back up correctly.

**Flash safety rules (learned the hard way, keep following these):**
- SD card must be inserted *before* entering DFU.
- If a download fails: retry from DFU, do **not** reset — resetting mid-fix
  can leave the device in a worse state.
- `.bin` on the SD card root + RESET is the safest, button-free recovery
  path if DFU access is ever lost.
- Only one WebUSB/dfu-util session/window open at a time.
- The bootloader itself is flashed **only** via the Seed's own micro-USB
  ROM-DFU — never via both USB cables at once.

## DFU download speed — measured, and what actually costs time

Measured 3 Sep 2026 on the rev11 demo Dubby over USB-C, dfu-util 0.11,
dubsiren 382 kB, SD card inserted:

| Bootloader | Result | Notes |
|---|---|---|
| upstream timeouts (no pacing) | **fails at 16 KB**, `errVENDOR` | host sends 1 KB every ~12 ms, a 1 KB QSPI write takes ~24 ms, the 2-slot queue fills after 16 blocks |
| adaptive pacing (`DUBBY_DFU_POLL_TIMEOUTS`) | 50.8 s / 53.0 s | 117 ms between writes for 27 ms of work |
| pacing + status fix (`fix/dfu-status-pacing`) | 30.9 s / 29.9 s | 78 ms between writes, same write timeout |
| + `USBD_DFU_XFER_SIZE=4096` | **10.7 s / 10.0 s** | 96 writes of 4 KB, a 4 KB write costs the same ~36 ms as a 1 KB one |

So the pacing is **not optional** with dfu-util as the host: without it the
transfer collapses deterministically on this hardware. "We never saw it
fail" almost certainly means Max / the web flasher poll more slowly than
dfu-util and hide it. Do not shrink the write timeout — that is the exact
failure above.

How the DFU protocol makes the host wait: after every block the
bootloader's `GETSTATUS` reply carries a `bwPollTimeout`, and the host must
wait it out before it may poll again. So whatever `MemoryStatus`
(`shared/dfu.cpp`) advertises is mandatory dead time per block:

- **Write** (`DFU_MEDIA_PROGRAM`): `last_write_ms × 1.5 + 4`, capped at
  500 ms. `last_write_ms` is the real measured QSPI time of the previous
  block (~24-27 ms; libDaisy's `QSPIHandle::Write` switches
  INDIRECT↔MEMORY_MAPPED on every call, which is where the time goes).
- **Erase**: 2× the last measured 64 KB erase + 50 ms (measured 117-141 ms);
  1100 ms (datasheet max) until one has been measured.
- **Address pointer**: dfu-util sends `SET_ADDRESS` before every chunk and
  the ST class answers it with `GetStatus(DFU_MEDIA_PROGRAM)` too
  (`usbd_dfu.c` ~line 1114). Nothing was queued, so it is answered with
  1 ms. Before this fix every chunk paid the write wait twice.

Per-chunk cost is ~40 ms advertised write wait plus ~30 ms of USB
round-trips (five transactions per chunk), and a QSPI write costs about
the same whether it is 1 KB or 4 KB (the INDIRECT↔MEMORY_MAPPED switch
dominates). So `USBD_DFU_XFER_SIZE=4096` (set in `build_dubby.sh`) cuts
the chunk count 4× for the same per-chunk cost: 50 s → 10 s. The 8 KB DFU
slot buffers in `shared/dfu.cpp` leave room for 8192 if ever needed; bench
it the same way first (two timed runs, pull the event log, busy must be 0).

To measure a change: flash the app **without** `:leave`, pull the DTCM log
(next section), then check `busy` = 0 and the write-to-write gap. The
scratch parser used on 3 Sep is 40 lines of Python: 16-byte header
(`buffer_`, capacity, `bufferIn_`, `bufferOut_`), then 20-byte events
`{type, timestamp, addr, len, duration}`, type 0..5 =
Status/Read/Write/Erase/WriteBusy/EraseBusy.

## The DFU diagnostic log

`shared/dfu_log.h` (`DfuLogger`) is a 512-entry ring buffer of
`{type, timestamp, start_addr, length, duration}` for every
Erase/Write/Read/Busy/Status event during a DFU session. **It lives in
DTCM SRAM only** — not SD, not flash, gone on power loss, and cleared at
the start of every new DFU session.

To pull it after a failed session (before resetting the device), the DFU
`Read`/upload path in `MemoryRead` special-cases any address inside the
DTCM range (0x20000000–0x2001FFFF) and returns raw memory instead of QSPI
flash content — so a normal DFU upload command can dump it, no debugger
needed:

```sh
dfu-util -a 0 -s <dfu_log-address>:10276:force -U log.bin
```

The address is wherever the linker placed the static `dfu_log` in
`.dtcmram_bss` for that specific build (check the `.elf`/map file — it's
not pinned) and `10276` bytes is `sizeof(FIFO<Event,512>)`.

## Branch hygiene

Check before building/testing on top of `componental/main` —
it does not always have every merged fix. As of writing:

- PR #1 (`dubby-uid96`) — merged into main.
- PR #2 (`dubby-bootloader-hardening`) — merged as its own PR, but that
  merge is **not** in `main`'s history (main got a separate, later commit
  with just the USB string defines).
- `fix/dfu-status-pacing` (local, on top of #3) — the status/erase timeout
  fix and `USBD_DFU_XFER_SIZE=4096` above, plus this guide.
- PR #3 (`fix/dfu-hardening` → `main`) — brings the actual hardening
  (stay-in-DFU marker, `MEM_If_Read` fix, DFU pacing/queue) into `main`.
  **Check its merge status before assuming `main` has the hardening.**

If you're building new bootloader work, branch from whichever ref actually
contains the hardening (verify with
`git merge-base --is-ancestor e6b28a3 <your-branch>` or by grepping for
`DUBBY_STAY_IN_DFU_IF_INCOMPLETE` in `shared/dubby_hardening.h`), not
blindly from `main`. Building on an un-hardened base will bring back the
`MEM_If_Read` stall and the write-starvation collapse.

## Hard rules

- Work only in the `componental` fork. Never push to `origin`
  (electro-smith).
- Never push to GitHub without an explicit go-ahead.
- Always test the actual hardware scenario (real Dubby, not just a bare
  Seed2 bench) before pushing or opening a PR.
