#include "mux_button.h"

#include "per/gpio.h"
#include "sys/system.h"

namespace daisy
{
bool MuxChannelHeldLow(Pin      mux_common,
                       Pin      sel0,
                       Pin      sel1,
                       Pin      sel2,
                       Pin      sel3,
                       uint8_t  channel,
                       uint32_t hold_ms,
                       uint32_t settle_ms)
{
    const Pin sel_pins[4] = {sel0, sel1, sel2, sel3};

    // Drive each present address line to its bit of `channel`. The 0 bits are
    // written explicitly too, so an unused-but-wired line can't float and route
    // a different channel.
    GPIO sel_gpio[4];
    for(uint8_t i = 0; i < 4; i++)
    {
        if(!sel_pins[i].IsValid())
            continue;
        sel_gpio[i].Init(sel_pins[i], GPIO::Mode::OUTPUT);
        sel_gpio[i].Write(((channel >> i) & 0x01) != 0);
    }

    GPIO common;
    common.Init(mux_common, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);

    // Let the mux channel settle (plus any RC on the click line) before sampling.
    System::Delay(settle_ms);

    bool held = false;
    // Only pay the hold cost when a press is actually present.
    if(!common.Read())
    {
        held                = true;
        const uint32_t start = System::GetNow();
        while(System::GetNow() - start < hold_ms)
        {
            if(common.Read())
            {
                held = false;
                break;
            }
            System::Delay(1);
        }
    }

    common.DeInit();
    for(uint8_t i = 0; i < 4; i++)
    {
        if(sel_pins[i].IsValid())
            sel_gpio[i].DeInit();
    }

    return held;
}

} // namespace daisy
