#ifndef DSY_BOOTLOADER_MUX_BUTTON_H
#define DSY_BOOTLOADER_MUX_BUTTON_H

#include <cstdint>
#include "daisy_core.h"

namespace daisy
{
/** Reads a single channel of an external analog multiplexer (CD405x / 74HC4067
 *  family) as if it were a digital button, without bringing up the ADC.
 *
 *  The mux address lines are driven as plain GPIO outputs to the binary value of
 *  `channel` (sel0 = LSB ... sel3 = MSB), the common line is read as a digital
 *  input with an internal pull-up, and every address line that is present is
 *  driven to a defined level (including the 0 bits) so the mux can never route a
 *  channel other than the requested one.
 *
 *  A press is taken to pull the common line LOW (matching how the Dubby wires
 *  its encoder click through the control mux). To avoid slowing a normal boot,
 *  the function returns immediately if the line is not already LOW; only when a
 *  press is present does it wait `hold_ms` confirming the line stays LOW the
 *  whole time. All GPIO used here is de-initialised before returning.
 *
 *  \param mux_common     Pin the mux common / signal pin is wired to.
 *  \param sel0           Address line, bit 0 (LSB).
 *  \param sel1           Address line, bit 1. Pass an invalid Pin() if unused.
 *  \param sel2           Address line, bit 2. Pass an invalid Pin() if unused.
 *  \param sel3           Address line, bit 3 (MSB). Pass an invalid Pin() if unused.
 *  \param channel        Mux channel index to select (0..15).
 *  \param hold_ms        How long the channel must read LOW continuously to
 *                        count as "held". Defaults to 1000 ms to match libDubby.
 *  \param settle_ms      Settling delay after switching the address lines, before
 *                        the common line is sampled. Covers the mux channel
 *                        transition plus any RC filtering on the click line.
 *  \return true if the selected channel read LOW continuously for `hold_ms`.
 */
bool MuxChannelHeldLow(Pin      mux_common,
                       Pin      sel0,
                       Pin      sel1,
                       Pin      sel2,
                       Pin      sel3,
                       uint8_t  channel,
                       uint32_t hold_ms   = 1000,
                       uint32_t settle_ms = 5);

} // namespace daisy

#endif // DSY_BOOTLOADER_MUX_BUTTON_H
