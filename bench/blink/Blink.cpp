// Bench test app for the bootloader hardening tests: fast blink on the Seed LED
// so a successful jump from the bootloader is visible (bootloader idles with a slow pulse).
#include "daisy_seed.h"
using namespace daisy;
DaisySeed hw;
int main(void)
{
    hw.Init();
    bool led = false;
    for(;;)
    {
        hw.SetLed(led);
        led = !led;
        System::Delay(100);
    }
}
