#include "bootloader.h"
#include "daisy_seed.h"
#include "mux_button.h"

// The encoder click is not a GPIO, it sits on one channel of the analog control
// mux (CD405x/74HC4067). Address lines and channel index match libDubby's
// DubbyControls.h. Seed pin -> STM32 port from libDaisy's seed:: pin map.
//   PIN_MUX_ADC  = D20 -> PORTC, 1     (mux common / signal)
//   PIN_MUX_S1   = D21 -> PORTC, 4     (address bit 0, LSB)
//   PIN_MUX_S2   = D17 -> PORTB, 1     (address bit 1)
//   PIN_MUX_S3   = D0  -> PORTB, 12    (address bit 2)
//   PIN_MUX_S4   = D18 -> PORTA, 7     (address bit 3, MSB)
//   ADC_MUX_INDEX_ENCODER_CLICK = 3  (binary 0011 -> S1=1, S2=1, S3=0, S4=0)
#define DUBBY_MUX_COMMON_PIN Pin(daisy::PORTC, 1)
#define DUBBY_MUX_S1_PIN Pin(daisy::PORTC, 4)
#define DUBBY_MUX_S2_PIN Pin(daisy::PORTB, 1)
#define DUBBY_MUX_S3_PIN Pin(daisy::PORTB, 12)
#define DUBBY_MUX_S4_PIN Pin(daisy::PORTA, 7)
#define DUBBY_MUX_ENCODER_CLICK_CHANNEL 3
// Hold time before the bootloader latches into infinite DFU, matching the
// >1000 ms hold libDubby's DubbyContext::Init() requires.
#define DUBBY_ENCODER_HOLD_MS 1000

using namespace daisy;

DaisySeed hw;
Bootloader boot;

// Deinit callback for DaisySeed-based build. Stops audio and de-inits the hardware.
void DaisyDeInitCallback(void* context)
{
	DaisySeed* seed = reinterpret_cast<DaisySeed*>(context);
	if (seed)
	{
		seed->StopAudio();
		seed->DeInit();
	}
}

void AudioCallback(AudioHandle::InputBuffer  in,
				   AudioHandle::OutputBuffer out,
				   size_t                    size)
{
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = out[1][i] = 0;
	}

	boot.AudioProcess(in, out, size);
}

int main(void) {

	uint32_t timeout_ms = startup_process();

	hw.Configure();
	hw.Init(true);

	SCB_DisableDCache();

	// If the encoder is held down while the bootloader starts, stay in DFU
	// forever. UINT32_MAX is the same timeout the INF_TIMEOUT path in
	// startup_process() returns, i.e. the state libDubby's ResetToBootloader()
	// reaches. DFU / SD / USB downloads still flash and jump normally.
	if (MuxChannelHeldLow(DUBBY_MUX_COMMON_PIN,
						  DUBBY_MUX_S1_PIN,
						  DUBBY_MUX_S2_PIN,
						  DUBBY_MUX_S3_PIN,
						  DUBBY_MUX_S4_PIN,
						  DUBBY_MUX_ENCODER_CLICK_CHANNEL,
						  DUBBY_ENCODER_HOLD_MS))
	{
		timeout_ms = UINT32_MAX;
	}

	boot.Init(hw.qspi, Pin(daisy::PORTC, 7), Pin(), timeout_ms, DaisyDeInitCallback, (void*)&hw);

	hw.StartAudio(AudioCallback);

	boot.IoInit();

	while(1)
	{
		boot.LoopProcess();
	}
}
