/**
 * Componental Dubby bootloader hardening helpers.
 *
 * Everything in here is behind compile-time defines that build_dubby.sh
 * applies to the Dubby "extdfu" variant only. Upstream / default builds
 * do not define them and are unaffected.
 *
 *   DUBBY_STAY_IN_DFU_IF_INCOMPLETE
 *       A DFU download that started (first erase or write to QSPI) but never
 *       reached the manifest stage leaves a marker in backup SRAM. On the
 *       next reset the bootloader sees the marker and stays in DFU with an
 *       infinite timeout (same latch as the BOOT button) instead of jumping
 *       into the half-written image. The marker is cleared when the host
 *       issues the DFU leave request after a complete download (LeaveDFU
 *       hook, requires USBD_DFU_VENDOR_EXIT_ENABLED=1) and before any
 *       bootloader-initiated jump (SD/USB load). It is NOT cleared in
 *       enable_jump(): nothing in the bootloader calls that function.
 */
#ifndef DUBBY_HARDENING_H
#define DUBBY_HARDENING_H

#include <cstdint>

#ifdef DUBBY_STAY_IN_DFU_IF_INCOMPLETE

/* The marker lives at the top of the 4 KB backup SRAM (0x38800000..0x38800FFF).
 * boot_info (libDaisy, 12 bytes) is placed at the bottom of the same region by
 * the linker; a fixed address at the top keeps the marker independent of link
 * order and of the application's own .backup_sram layout.
 *
 * Backup SRAM belongs to the backup domain: it is NOT cleared by
 * HAL_NVIC_SystemReset (only by a backup-domain reset or loss of VBAT/VDD).
 * The Seed has no battery, so after a full power cycle the content is
 * undefined and the marker is lost; a random word matching the magic is a
 * 1 in 2^32 event and would only cost one extra DFU session. */
#define DUBBY_DFU_MARKER_ADDR 0x38800FF8U
#define DUBBY_DFU_MARKER_MAGIC 0xDF0BAD01U

static inline void dubby_dfu_marker_set()
{
    *(volatile uint32_t*)DUBBY_DFU_MARKER_ADDR = DUBBY_DFU_MARKER_MAGIC;
}

static inline void dubby_dfu_marker_clear()
{
    *(volatile uint32_t*)DUBBY_DFU_MARKER_ADDR = 0U;
}

static inline bool dubby_dfu_marker_is_set()
{
    return *(volatile uint32_t*)DUBBY_DFU_MARKER_ADDR == DUBBY_DFU_MARKER_MAGIC;
}

#endif // DUBBY_STAY_IN_DFU_IF_INCOMPLETE

#endif // DUBBY_HARDENING_H
