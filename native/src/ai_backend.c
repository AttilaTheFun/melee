/* Configuration used by Melee's synthesizer startup. AX output is generated
 * by the native mixer at 32 kHz; Apple owns device-rate conversion/scheduling.
 * DVD hardware streaming, DMA interrupts and stream sample counters are not
 * implemented here. These volume values mirror the separate AI stream controls,
 * not the AX voice mix (which the original synthesizer controls itself). */
#include <dolphin/ai.h>
#include <dolphin/os.h>

static BOOL initialized;
static u8 left_volume, right_volume;

void AIInit(u8* stack)
{
    (void)stack; /* Native callbacks use the host ABI's stack. */
    BOOL old = OSDisableInterrupts();
    if (!initialized) {
        left_volume = right_volume = 0;
        initialized = TRUE;
    }
    OSRestoreInterrupts(old);
}

BOOL AICheckInit(void)
{
    BOOL old = OSDisableInterrupts();
    BOOL result = initialized;
    OSRestoreInterrupts(old); return result;
}

void AIReset(void)
{
    BOOL old = OSDisableInterrupts();
    initialized = FALSE;
    OSRestoreInterrupts(old);
}

void AISetDSPSampleRate(u32 rate)
{
    if (rate != AI_SAMPLERATE_32KHZ)
        OSPanic(__FILE__, __LINE__, "Native AX output requires 32 kHz");
}
u32 AIGetDSPSampleRate(void) { return AI_SAMPLERATE_32KHZ; }

void AISetStreamVolLeft(u8 volume)
{
    BOOL old = OSDisableInterrupts(); left_volume = volume;
    OSRestoreInterrupts(old);
}
void AISetStreamVolRight(u8 volume)
{
    BOOL old = OSDisableInterrupts(); right_volume = volume;
    OSRestoreInterrupts(old);
}
u8 AIGetStreamVolLeft(void)
{
    BOOL old = OSDisableInterrupts(); u8 result = left_volume;
    OSRestoreInterrupts(old); return result;
}
u8 AIGetStreamVolRight(void)
{
    BOOL old = OSDisableInterrupts(); u8 result = right_volume;
    OSRestoreInterrupts(old); return result;
}
