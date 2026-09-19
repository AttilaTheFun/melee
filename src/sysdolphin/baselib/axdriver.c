#include "axdriver.h"

#include <math.h>
#include <string.h>
#ifdef MELEE_NATIVE
#include "melee_sem.h"
#include <stdlib.h>
#include <limits.h>
static MeleeSEM* native_sem;
#endif

#include "axdriver.static.h"
#include "debug.h"
#include "synth.h"
#include <dolphin/ax.h>
#include <dolphin/axfx.h>
#include <dolphin/dvd.h>
#include <dolphin/os.h>

void* AXDriverAlloc(size_t size)
{
#ifdef MELEE_NATIVE
    if (!AXDriver_804D77D4 || axfxallocsize > axfxmaxsize ||
        size > (size_t) (axfxmaxsize - axfxallocsize)) {
        return NULL;
    }
    void* ptr = AXDriver_804D77D4 + axfxallocsize;
    axfxallocsize += (u32) size;
    return ptr;
#else
    void* ptr = &AXDriver_804D77D4[axfxallocsize];

    // increment the size by the amount we will be indexing to.
    axfxallocsize += size;

    // size exceeds the max allowed; the pointer that we return would be
    // outside the heap. Raise an assert.
    HSD_ASSERT(78, axfxallocsize < axfxmaxsize);
    return ptr;
#endif
}

void AXDriverFree(void* ptr) {}

void AXDriverUnlink(HSD_SM* v, HSD_SM** head)
{
    HSD_SM* p;
    HSD_SM* n;

    if (v != NULL) {
        p = v->prev;
        n = v->next;
        v->next = NULL;
        v->prev = NULL;
        if (p != NULL) {
            p->next = n;
        }
        if (n != NULL) {
            n->prev = p;
        }
        if (*head == v) {
            *head = n;
        }
        HSD_ASSERT(113, *head != v);
    }
}

static inline void unk_inline(HSD_SM* v, HSD_SM** head)
{
    if (v == NULL) {
        return;
    }

    HSD_ASSERT(0x7A, *head != v);

    v->prev = NULL;
    if (*head != NULL) {
        (*head)->prev = v;
    }
    v->next = *head;
    *head = v;
}

static inline bool tmp(HSD_SM* v)
{
    int idx;
    u32 state;

    state = v->flags & SMSTATE_MASK;
    HSD_ASSERTMSG(0x92, state == SMSTATE_ACTIVE || state == SMSTATE_SLEEP,
                  "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE || "
                  "(v->flags&SMSTATE_MASK) == SMSTATE_SLEEP");

    idx = v->vID;
    if (v->vID != -1) {
        AXDriver_804C5920[idx & 0x3F] = 0;
        v->vID = -1;
        HSD_SynthSFXKeyOff(idx);
        AXDriver_804D77C8--;
    }

    v->flags &= ~0x20000000;
    v->flags &= ~SMSTATE_MASK;
    return true;
}

bool AXDriverKeyOff(int vid)
{
    bool result;
    int idx;
    HSD_SM* v;
    bool enabled;
    PAD_STACK(8);

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }

    enabled = OSDisableInterrupts();
    v = &AXDriver_804C45A0[idx];

    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        OSRestoreInterrupts(enabled);
        return false;
    }

    if (v == NULL) {
        result = false;
    } else {
        result = tmp(v);
    }

    OSRestoreInterrupts(enabled);
    return result;
}

void HSD_AudioSFXKeyOffAll(void)
{
    bool enabled = OSDisableInterrupts();
    HSD_SM* v = AXDriver_804D7794;
    PAD_STACK(8);

    while (v != NULL) {
        if (v->flags & SMSTATE_MASK) {
            if (v != NULL) {
                tmp(v);
            }
        }
        v = v->next;
    }
    OSRestoreInterrupts(enabled);
}

void HSD_AudioSFXKeyOffTrack(int track)
{
    bool enabled = OSDisableInterrupts();
    HSD_SM* v = AXDriver_804D7794;

    while (v != NULL) {
        if ((v->flags & SMSTATE_MASK) && v->track == track) {
            if (v != NULL) {
                tmp(v);
            }
        }
        v = v->next;
    }
    OSRestoreInterrupts(enabled);
}

#ifdef MUST_MATCH
/// MSL sqrtf expansion (src/MSL/math_ppc.h) writing its result through a
/// caller-provided slot, as in sqrtf_store in lbcollision.c and mplib.c.
/// Evidence: retail AXDriver_8038BF6C keeps its eight sqrt results in
/// adjacent 4-byte stack temps at frame offsets 0x10..0x2C (an 8-byte
/// aligned base), one slot per call in source order, each accessed as a
/// stfs/lfs pair. The volatile-qualified accesses through the slot pointer
/// pin those pairs without changing behavior.
static inline float sqrtf_store(float x, volatile float* y)
{
    if (x > 0.0F) {
        double guess = __frsqrte((double) x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        *y = (float) (x * guess);
        return *(volatile float*) y;
    }
    return x;
}
#else
#define sqrtf_store(x, y) sqrtf(x)
#endif

void AXDriver_8038BF6C(HSD_SM* v)
{
    u32 flag;
    int i;
    /// sqrtf result slots; retail frame offsets 0x10..0x2C (base aligned 8)
    float sqrt_tmp[8] ATTRIBUTE_ALIGN(8);

    for (i = 0; i <= 9; i++) {
        flag = 1 << i;

        if (v->flags & flag) {
            switch (flag) {
            case 0x1: {
                float left_vol = (v->x26 * v->x24[0]) / 65535.0F;
                float right_vol = (v->x27 * v->x24[1]) / 65535.0F;
                float left_sqrt = sqrtf_store(left_vol, &sqrt_tmp[7]);
                float left_inv_sqrt =
                    sqrtf_store(1.0F - left_vol, &sqrt_tmp[6]);
                float right_sqrt = sqrtf_store(right_vol, &sqrt_tmp[5]);
                float right_inv_sqrt =
                    sqrtf_store(1.0F - right_vol, &sqrt_tmp[4]);
                float tmp2 = left_inv_sqrt * right_inv_sqrt;
                float pitch1 = powf(2.0F, v->x20 / 1200.0F);
                float pitch2 = powf(2.0F, v->fadetime / 1200.0F);

                v->vID = HSD_SynthSFXPlayWithGroup(
                    v->fid, v->x1A, v->volume,
                    (v->flags & 0x20000) ? v->pan : v->x1C, v->pri, v->itdflag,
                    v->track, pitch1, pitch2, left_inv_sqrt * tmp2, left_sqrt,
                    right_sqrt * left_inv_sqrt);

                if (v->vID != -1) {
                    AXDriver_804C5920[v->vID & 0x3F] = v;
                    v->flags &= 0xFFF4FF99;
                    if (v->flags & 0x40000) {
                        v->flags &= ~0x40000;
                    }
                    AXDriver_804D77C8++;
                } else {
                    HSD_ASSERT(0x13B,
                             (v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE);
                    v->flags &= ~SMSTATE_MASK;
                    return;
                }
                break;
            }
            case 0x2:
                HSD_SynthSFXSetPriority(v->vID, v->pri);
                break;
            case 0x4:
                HSD_SynthSFXSetVolumeFade(v->vID, v->x1A, 0);
                break;
            case 0x8:
                HSD_SynthSFXSetUserVol(v->vID, v->x1C);
                break;
            case 0x10:
                break;
            case 0x20:
                HSD_SynthSFXSetPitchRatio(v->vID, 0,
                                          powf(2.0F, v->x20 / 1200.0F));
                break;
            case 0x40:
                HSD_SynthSFXSetPitchRatio(v->vID, 1,
                                          powf(2.0F, v->fadetime / 1200.0F));
                break;
            case 0x80: {
                float left_vol = (v->x26 * v->x24[0]) / 65535.0F;
                float right_vol = (v->x27 * v->x24[1]) / 65535.0F;
                float left_sqrt = sqrtf_store(left_vol, &sqrt_tmp[3]);
                float left_inv_sqrt =
                    sqrtf_store(1.0F - left_vol, &sqrt_tmp[2]);
                float right_sqrt = sqrtf_store(right_vol, &sqrt_tmp[1]);
                float right_inv_sqrt =
                    sqrtf_store(1.0F - right_vol, &sqrt_tmp[0]);

                HSD_SynthSFXSetMix(
                    v->vID, left_inv_sqrt * (left_inv_sqrt * right_inv_sqrt),
                    left_sqrt, right_sqrt * left_inv_sqrt);
                break;
            }
            case 0x100:
                v->flags = (v->flags & ~SMSTATE_MASK) | SMSTATE_SLEEP;
                return;
            case 0x200:
                if (v != NULL) {
                    tmp(v);
                }
                return;
            }
        }

        v->flags &= ~flag;
    }
}

u32 AXDriver_8038C678(u32 param_type, u32 param_value)
{
    switch (param_type) {
    case 0:
        return param_value & 0xFFFFFF;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 14:
    case 15:
    case 20:
    case 21:
        return false;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 16:
    case 17:
    case 18:
    case 19:
        return param_value >> 8 & 0xFFFF;
    case 12:
    case 13:
        return param_value >> 16 & 0xFF;
    default:
        return false;
    }
}

#define MIN2(x, y) ((x) < (y) ? (x) : (y))
#define MAX2(x, y) ((x) < (y) ? (y) : (x))
#define CLAMP(min, val, max) MAX2(MIN2(val, max), min)

#ifdef MELEE_NATIVE
static void native_sound_commands(HSD_SM* v)
#else
void AXDriver_8038C6C0(HSD_SM* v)
#endif
{
    u32 cmd_type;
    u32 cmd_word;
    int cmd_size;
    int cmd_val;
    PAD_STACK(8);

    while (v->x30 == (s32) AXDriver_804D778C) {
#ifdef MELEE_NATIVE
        u32* checked;
        if (!melee_sem_command_target(native_sem, v->cmd_stream, 0, &checked))
            OSPanic(__FILE__, __LINE__, "Sound command escaped the native SEM payload");
#endif
        cmd_word = *v->cmd_stream;
        cmd_type = cmd_word >> 0x18U;

        cmd_size = AXDriver_8038C678(cmd_type, cmd_word);
        if (cmd_size != 0) {
            AXDriver_8038BF6C(v);
        }
        v->x30 += cmd_size;
        switch (cmd_type) {
        case 2:
            v->x2A = *v->cmd_stream;
            if (v->x2A == 0) {
                v->flags |= 0x100000;
            }
            break;
        case 3:
            if ((v->flags & 0x100000) || v->x2A != 0) {
#ifdef MELEE_NATIVE
                /* The console increments cmd_stream after the subtraction.
                 * Validate the actual next instruction, allowing a loop back
                 * to the first payload word without entering metadata. */
                size_t backwards = cmd_word & 0xFFFFFF;
                const u32* origin = v->cmd_stream;
                if (backwards) --backwards;
                else ++origin;
                if (!melee_sem_command_target(native_sem, origin, backwards, &checked))
                    OSPanic(__FILE__, __LINE__, "Sound command loop escaped the native SEM payload");
                v->cmd_stream = checked;
                v->x2A--;
                continue;
#else
                v->cmd_stream -= *v->cmd_stream & 0xFFFFFF;
#endif
                v->x2A--;
            }
            break;
        case 1:
            v->flags |= 1;
            v->fid = *v->cmd_stream;
            break;
        case 4:
            v->flags |= 2;
            v->pri = *v->cmd_stream;
            break;
        case 5:
            v->flags |= 2;
            cmd_val = v->pri + (s8) (u8) *v->cmd_stream;
            v->pri = CLAMP(5, cmd_val, 0x1C);
            break;
        case 6:
            v->flags |= 4;
            v->x1A = *v->cmd_stream;
            break;
        case 7:
            v->flags |= 4;
            cmd_val = v->x1A + (s8) (u8) *v->cmd_stream;
            v->x1A = CLAMP(0, cmd_val, 0xFF);
            break;
        case 8:
            v->flags |= 8;
            v->x1C = *v->cmd_stream;
            break;
        case 9:
            v->flags |= 8;
            cmd_val = v->x1C + (s8) (u8) *v->cmd_stream;
            v->x1C = CLAMP(0, cmd_val, 0xFF);
            break;
        case 10:
            v->flags |= 0x10;
            v->x1E = *v->cmd_stream;
            break;
        case 11:
            v->flags |= 0x10;
            cmd_val = v->x1E + (s8) (u8) *v->cmd_stream;
            v->x1E = CLAMP(0, cmd_val, 0xFF);
            break;
        case 12:
            v->flags |= 0x20;
            v->x20 = (s16) (u16) *v->cmd_stream;
            break;
        case 13:
            v->flags |= 0x20;
            cmd_val = v->x20 + (s16) (u16) *v->cmd_stream;
            v->x20 = CLAMP(-0x2A30, cmd_val, 0x960);
            break;
        case 16:
            if (!(AXDriver_804D603C & 1)) {
                v->flags |= 0x80;
                v->x24[0] = *v->cmd_stream;
            }
            break;
        case 20:
            if (!(AXDriver_804D603C & 1)) {
                v->x26 = *v->cmd_stream;
            }
            break;
        case 21:
            if (!((AXDriver_804D603C >> 1U) & 1)) {
                v->x27 = *v->cmd_stream;
            }
            break;
        case 17:
            if (!(AXDriver_804D603C & 1)) {
                v->flags |= 0x80;
                cmd_val = v->x24[0] + (s8) (u8) *v->cmd_stream;
                v->x24[0] = CLAMP(0, cmd_val, 0xFF);
            }
            break;
        case 18:
            if (!((AXDriver_804D603C >> 1U) & 1)) {
                v->flags |= 0x80;
                v->x24[1] = *v->cmd_stream;
            }
            break;
        case 19:
            if (!(AXDriver_804D603C >> 1 & 1)) {
                v->flags |= 0x80;
                cmd_val = v->x24[1] + (s8) (u8) *v->cmd_stream;
                v->x24[1] = CLAMP(0, cmd_val, 0xFF);
            }
            break;
        case 15:
            v->flags |= 0x200;
            AXDriver_8038BF6C(v);
            return;
        case 14:
            v->flags |= 0x100;
            AXDriver_8038BF6C(v);
            return;
        }
        v->cmd_stream++;
    }
}

#ifdef MELEE_NATIVE
void AXDriver_8038C6C0(HSD_SM* voice)
{
    int old = OSDisableInterrupts();
    native_sound_commands(voice);
    OSRestoreInterrupts(old);
}
#endif

static void fn_8038CC1C(void)
{
    HSD_SM* v;
    HSD_SM* next;
    PAD_STACK(4);

    if (AXDriver_804D77E0 != 0) {
        AXDriver_804D77E0 = 0;
        if (AXDriver_804D6038 != -1) {
            float x =
                powf(2.0F, CLAMP(-0x2A30, AXDriver_804D77E4, 0x960) / 1200.0F);
            HSD_SynthSFXSetPitchRatio(AXDriver_804D6038, 1, x);
        }
    }
    v = AXDriver_804D7794;
    AXDriver_804D778C++;
    while (v != NULL) {
        next = v->next;
        if (v->x30 == -1) {
            v->x30 = AXDriver_804D778C;
        } else if (v->flags & 0x20000000) {
            v->x30++;
        }
        switch (v->flags & SMSTATE_MASK) {
        case SMSTATE_ACTIVE:
            if (v->x30 == AXDriver_804D778C) {
                AXDriver_8038C6C0(v);
            }
            break;
        case 0:
            AXDriverUnlink(v, &AXDriver_804D7794);
            unk_inline(v, &AXDriver_804D7790);
            AXDriver_804D77D0--;
            break;
        case SMSTATE_SLEEP:
            if (v->flags & 0x40) {
                float x = powf(2.0F, v->fadetime / 1200.0F);
                HSD_SynthSFXSetPitchRatio(v->vID, 1, x);
                v->flags &= 0xFFFFFFBF;
            }
            break;
        default:
            HSD_ASSERT(0x25F, 0);
            break;
        }
        v = next;
    }
}

static void fn_8038CEA4(s32 vID)
{
    HSD_SM* v;
    int idx = vID & 0x3F;

    HSD_ASSERT(0x26D, vID > 0);

    v = AXDriver_804C5920[idx];
    if (v == NULL || v->vID != vID) {
        return;
    }

    AXDriver_804C5920[idx] = NULL;
    v->flags &= ~0x20000000;
    v->flags &= ~SMSTATE_MASK;
    AXDriver_804D77C8--;
}

static void fn_8038CF48(s32 vID)
{
    HSD_SM* v;
    int idx;
    idx = vID & 0x3F;

    HSD_ASSERT(0x288, vID > 0);

    v = AXDriver_804C5920[idx];
    HSD_ASSERT(0x28A, v && v->vID == vID);

    v->flags |= 0x20000000;
}

static inline HSD_SM* AXDriver_8038CFF4_inline(void)
{
    if (AXDriver_804D7790 == NULL) {
        return NULL;
    } else {
        HSD_SM* v;
        bool enabled = OSDisableInterrupts();
        v = AXDriver_804D7790;
        AXDriverUnlink(v, &AXDriver_804D7790);
        OSRestoreInterrupts(enabled);
        v->vID = -1;
        v->x30 = -1;
        v->flags = 0;
        v->flags &= ~SMSTATE_MASK;
        return v;
    }
}

#ifdef MELEE_NATIVE
static int native_sound_start(int sound_id, u8 volume, u8 pan, int track, int channel)
#else
int AXDriver_8038CFF4(int sound_id, u8 volume, u8 pan, int track, int channel)
#endif
{
#ifdef MELEE_NATIVE
    if (sound_id < 0) return -1;
#endif
    HSD_SM* v;
    int sample_idx;
    int bank_idx;
    int bank_mem;
    bool enabled;

    bank_idx = sound_id / 10000;
    bank_mem = sound_id % 10000;

    if (AXDriver_804D77B0 <= bank_idx) {
        return -1;
    }

    sample_idx = bank_mem + AXDriver_804D77B4[bank_idx];

    if (AXDriver_804D77B8 <= sample_idx) {
        return -1;
    }

    if (bank_idx < AXDriver_804D77B0 - 1 &&
        AXDriver_804D77B4[bank_idx + 1] <= sample_idx)
    {
        return -1;
    }

    if (track < 0 || track > 0xFF) {
        return -1;
    }

    if (channel < 0 || channel >= 0x10) {
        return -1;
    }

    if (AXDriver_804D77CC & (1 << channel)) {
        return -1;
    }

    v = AXDriver_8038CFF4_inline();

    if (v == NULL) {
        return -1;
    }

    v->x16 = sound_id;
    v->cmd_stream = AXDriver_804D77BC[sample_idx];
    v->x1A = 0xFF;
    v->volume = volume;
    v->x1C = 0x80;
    v->pan = pan;
    v->x20 = 0;
    v->fadetime = 0;
    v->x24[0] = AXDriver_804C5A20[0][channel];
    v->x24[1] = AXDriver_804C5A20[1][channel];
    v->x26 = 0xFF;
    v->x27 = 0xFF;
    v->pri = 5;
    v->track = track;
    v->itdflag = channel;
    v->flags |= 0x30000;

    if (vidhigh & 0xFE000000) {
        OSReport("vidhigh exceeds the max value\n");
        HSD_ASSERT(0x2EA, 0);
    }

    v->unk =
        (vidhigh << 7) | ((u8*) v - (u8*) AXDriver_804C45A0) / sizeof(HSD_SM);
    vidhigh++;

    enabled = OSDisableInterrupts();
    v->flags = (v->flags & ~SMSTATE_MASK) | SMSTATE_ACTIVE;
    unk_inline(v, &AXDriver_804D7794);
    AXDriver_804D77D0++;
    OSRestoreInterrupts(enabled);

    return v->unk;
}

#ifdef MELEE_NATIVE
int AXDriver_8038CFF4(int sound_id, u8 volume, u8 pan, int track, int channel)
{
    int old = OSDisableInterrupts();
    int result = native_sound_start(sound_id, volume, pan, track, channel);
    OSRestoreInterrupts(old);
    return result;
}
#endif

bool AXDriver_8038D2B4(int vid, u8 pan)
{
    int idx;
    bool enabled;
    HSD_SM* v;
    u8 clamped;

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    enabled = OSDisableInterrupts();
    if (v->vID != -1) {
        HSD_SynthSFXSetUserVol(v->vID, MIN(pan, 0xFF));
    } else {
        HSD_ASSERT(0x30B, (v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE);
        v->pan = pan;
        v->flags |= 0x20000;
    }
    OSRestoreInterrupts(enabled);
    return true;
}

bool AXDriver_8038D3B8(s32 vid, u8 volume)
{
    HSD_SM* v;
    s32 idx;
    bool enabled;
    s32 voice_id;

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    enabled = OSDisableInterrupts();
    voice_id = v->vID;
    if (voice_id != -1) {
        HSD_SynthSFXSetVolumeFade(voice_id, CLAMP(0, volume, 0xFF), 1);
    } else {
        HSD_ASSERT(0x34D, (v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE);
        v->volume = volume;
        v->flags |= 0x10000;
    }
    OSRestoreInterrupts(enabled);
    return true;
}

bool AXDriver_8038D4E4(s32 vid, s16 pitch)
{
    HSD_SM* v;
    s32 idx;
    bool enabled;
    int clamped;

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    clamped = CLAMP(-0x2A30, pitch, 0x960);
    enabled = OSDisableInterrupts();
    v->fadetime = clamped;
    v->flags |= 0x40;
    OSRestoreInterrupts(enabled);
    return true;
}

bool AXDriver_8038D5B4(s32 vid, s32 aux_bus, u8 send_level)
{
    HSD_SM* v;
    float right_vol;
    float left_vol;
    float left_inv_sqrt;
    float left_sqrt;
    float right_sqrt;
    float right_inv_sqrt;
    bool enabled;
    int idx;
    int lock_flag;
    int unused;
    int clamped;

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }
    if (aux_bus < 0 || aux_bus > 1) {
        return false;
    }
    if (aux_bus == 0) {
        lock_flag = AXDriver_804D603C & 1;
    } else {
        lock_flag = (AXDriver_804D603C >> 1) & 1;
    }
    if (lock_flag != 1) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    clamped = CLAMP(0, send_level, 0xFF);
    enabled = OSDisableInterrupts();
    if (v->vID != -1) {
        v->x24[aux_bus] = clamped;
        left_vol = (f32) (v->x26 * v->x24[0]) / 65535.0F;
        right_vol = (f32) (v->x27 * v->x24[1]) / 65535.0F;
        left_sqrt = sqrtf(left_vol);
        left_inv_sqrt = sqrtf(1.0F - left_vol);
        right_sqrt = sqrtf(right_vol);
        right_inv_sqrt = sqrtf(1.0F - right_vol);
        HSD_SynthSFXSetMix(v->vID,
                           left_inv_sqrt * (left_inv_sqrt * right_inv_sqrt),
                           left_sqrt, right_sqrt * left_inv_sqrt);
    } else {
        HSD_ASSERT(0x3AB, (v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE);
        v->x24[aux_bus] = clamped;
        v->flags |= 0x80000;
    }
    OSRestoreInterrupts(enabled);
    return true;
}

bool AXDriver_8038D914(s32 channel, s32 aux_bus, s8 send_level)
{
    bool enabled;
    HSD_SM* v;

    if (channel < 0 || channel >= 0x10) {
        return false;
    }
    if (aux_bus < 0 || aux_bus > 1) {
        return false;
    }
    enabled = OSDisableInterrupts();
    v = AXDriver_804D7794;
    while (v != NULL) {
        if ((v->flags & SMSTATE_MASK) && v->itdflag == channel) {
            AXDriver_8038D5B4(v->unk, aux_bus, (u8) send_level);
        }
        v = v->next;
    }
    AXDriver_804C5A20[aux_bus][channel] = send_level;
    OSRestoreInterrupts(enabled);
    return true;
}

bool AXDriver_8038D9D8(int vid)
{
    HSD_SM* v;
    int idx;

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    if (HSD_SynthSFXCheck(v->vID) == -1) {
        return false;
    }
    return true;
}

static void fn_8038DA5C(s32 result, DVDFileInfo* fileInfo)
{
    if (result != -1) {
        AXDriver_804D77EC = 1;
    }
}

void AXDriver_8038DA70(const char* path, void (*callback)(void))
{
#ifdef MELEE_NATIVE
    (void) callback;
    DVDFileInfo info;
    if (!DVDOpen(path, &info)) { OSReport("can not open %s\n", path); return; }
    size_t length = info.length;
    void* raw = NULL;
    MeleeSEM* loaded = NULL;
    if (length && length <= INT32_MAX - 31) {
        size_t aligned = (length + 31) & ~(size_t)31;
        if (!posix_memalign(&raw, 32, aligned) &&
            DVDReadPrio(&info, raw, aligned, 0, 2) == (long)aligned)
            loaded = melee_sem_open(raw, length);
    }
    free(raw);
    DVDClose(&info);
    if (!loaded) { OSReport("Invalid native SEM: %s\n", path); return; }
    int old = OSDisableInterrupts();
    for (HSD_SM* voice = AXDriver_804D7794; voice; voice = voice->next) {
        if (voice->flags & SMSTATE_MASK) {
            OSRestoreInterrupts(old); melee_sem_close(loaded);
            OSReport("Cannot replace SEM while sound commands are active\n"); return;
        }
    }
    melee_sem_close(native_sem);
    native_sem = loaded;
    AXDriver_804D7798 = loaded->words;
    AXDriver_804D779C = length;
    AXDriver_804D77A0 = loaded->counts[0]; AXDriver_804D77A4 = loaded->values[0];
    AXDriver_804D77A8 = loaded->counts[1]; AXDriver_804D77AC = loaded->references[1];
    AXDriver_804D77B0 = loaded->counts[2]; AXDriver_804D77B4 = loaded->values[2];
    AXDriver_804D77B8 = loaded->counts[3]; AXDriver_804D77BC = loaded->references[3];
    AXDriver_804D77C0 = loaded->counts[4]; AXDriver_804D77C4 = loaded->references[4];
    OSRestoreInterrupts(old);
#else
    DVDFileInfo fileInfo;
    s32 entrynum;
    s32 alignedSize;
    void* ptr;
    s32 offset;
    s32 count;
    s32 j;
    s32 i;

    entrynum = DVDConvertPathToEntrynum(path);
    if (entrynum == -1 || DVDFastOpen(entrynum, &fileInfo) == 0) {
        OSReport("can not open %s\n", path);
        return;
    }

    AXDriver_804D779C = fileInfo.length;
    if (AXDriver_804D779C == 0) {
        OSReport("file size of \"%s\" is 0\n", path);
        return;
    }

    alignedSize = (AXDriver_804D779C + 0x1F) & ~0x1F;
    AXDriver_804D7798 = HSD_AudioMalloc(alignedSize);
    AXDriver_804D77EC = 0;
    DVDReadAsyncPrio(&fileInfo, AXDriver_804D7798, alignedSize, 0, fn_8038DA5C,
                     2);

    while (AXDriver_804D77EC == 0) {
        callback();
    }

    DVDClose(&fileInfo);

    AXDriver_804D77A0 = ((s32*) AXDriver_804D7798)[0];
    count = AXDriver_804D77A0;
    if (count != 0) {
        ptr = (void*) ((u8*) AXDriver_804D7798 + 4);
    } else {
        ptr = NULL;
    }
    offset = count * 4 + 4;
    AXDriver_804D77A4 = ptr;

    AXDriver_804D77A8 = *(s32*) ((u8*) AXDriver_804D7798 + offset);
    offset += 4;
    if (AXDriver_804D77A8 != 0) {
        ptr = (u8*) AXDriver_804D7798 + offset;
    } else {
        ptr = NULL;
    }
    AXDriver_804D77AC = ptr;

    i = 0;
    j = i;
    while (i < AXDriver_804D77A8) {
        i++;
        *(u32*) ((u8*) AXDriver_804D77AC + j) += (u32) AXDriver_804D7798 & ~3u;
        j += 4;
    }

    offset += AXDriver_804D77A8 * 4;
    ptr = AXDriver_804D7798;
    AXDriver_804D77B0 = *(s32*) ((u8*) ptr + offset);
    offset += 4;
    count = AXDriver_804D77B0;
    AXDriver_804D77B4 = count != 0 ? (u32*) ((u8*) ptr + offset) : NULL;
    offset += count * 4;

    AXDriver_804D77B8 = *(s32*) ((u8*) ptr + offset);
    offset += 4;
    if (AXDriver_804D77B8 != 0) {
        ptr = (u8*) ptr + offset;
    } else {
        ptr = NULL;
    }
    j = 0;
    AXDriver_804D77BC = ptr;
    i = j;
    while (j < AXDriver_804D77B8) {
        j++;
        *(u32*) ((u8*) AXDriver_804D77BC + i) += (u32) AXDriver_804D7798 & ~3u;
        i += 4;
    }

    offset += AXDriver_804D77B8 * 4;
    AXDriver_804D77C0 = *(s32*) ((u8*) AXDriver_804D7798 + offset);
    offset += 4;
    if (AXDriver_804D77C0 != 0) {
        ptr = (u8*) AXDriver_804D7798 + offset;
    } else {
        ptr = NULL;
    }
    j = 0;
    AXDriver_804D77C4 = ptr;
    i = j;
    while (j < AXDriver_804D77C0) {
        j++;
        *(u32*) ((u8*) AXDriver_804D77C4 + i) += (u32) AXDriver_804D7798 & ~3u;
        i += 4;
    }
#endif
}

void AXDriver_8038DCFC(void)
{
#ifdef MELEE_NATIVE
    int old = OSDisableInterrupts();
    for (HSD_SM* voice = AXDriver_804D7794; voice; voice = voice->next) {
        if (voice->flags & SMSTATE_MASK) {
            OSRestoreInterrupts(old);
            OSReport("Cannot unload SEM while sound commands are active\n"); return;
        }
    }
    melee_sem_close(native_sem); native_sem = NULL;
    AXDriver_804D7798 = AXDriver_804D77A4 = AXDriver_804D77AC = AXDriver_804D77C4 = NULL;
    AXDriver_804D77B4 = NULL; AXDriver_804D77BC = NULL;
    AXDriver_804D779C = 0;
    AXDriver_804D77A0 = AXDriver_804D77A8 = AXDriver_804D77B0 =
        AXDriver_804D77B8 = AXDriver_804D77C0 = 0;
    OSRestoreInterrupts(old);
#else
    if (AXDriver_804D7798 != NULL) {
        HSD_AudioFree(AXDriver_804D7798);
    }
    AXDriver_804D7798 = NULL;
#endif
}

#ifdef MELEE_NATIVE
/* AX invokes a callback with two void-pointer arguments; use real adapters rather than calling
 * typed DSP functions through incompatible function pointer types. */
#define AX_NATIVE_ADAPTER(name) \
    static void name##_native(void* samples, void* effect) { \
        struct AXFX_BUFFERUPDATE channels; \
        memcpy(&channels, samples, sizeof(channels)); \
        name(&channels, effect); \
    }
AX_NATIVE_ADAPTER(AXFXReverbHiCallback)
AX_NATIVE_ADAPTER(AXFXReverbStdCallback)
AX_NATIVE_ADAPTER(AXFXChorusCallback)
AX_NATIVE_ADAPTER(AXFXDelayCallback)
#undef AX_NATIVE_ADAPTER
#define AX_EFFECT_CALLBACK(name) name##_native
#else
#define AX_EFFECT_CALLBACK(name) name
#endif

int AXDriverSetupAux(int channel, AXDriverAuxType type, void* param)
{
    struct AXFX_REVERBHI* aux_data_hi;
    struct AXFX_REVERBSTD* aux_data_std;
    struct AXFX_CHORUS* aux_data_chorus;
    struct AXFX_DELAY* aux_data_delay;

    int old_type;
    int result;
#ifdef MELEE_NATIVE
    void (*callback)(void*, void*);
    HSD_ASSERT(0x49C, channel == 0 || channel == 1);
#else
    void* callback;
#endif
    void* callback_data;

    aux_data_hi = &AXDriver_804C5A40[channel];
    aux_data_std = &AXDriver_804C5E00[channel];
    aux_data_chorus = &AXDriver_804C60A8[channel];
    aux_data_delay = &AXDriver_804C61E0[channel];

    result = 0;
    callback = NULL;
    callback_data = NULL;

    // Validate aux channel (0 or 1)
    HSD_ASSERT(0x49C, channel == 0 || channel == 1);

    // Validate effect type (0-4)
    HSD_ASSERT(0x49D, 0 <= type && type <= 4);

    // Validate param pointer when effect type is not 0 (off)
    HSD_ASSERT(0x49E, type == 0 || param != NULL);

    // Get old effect type and update with new type
    if (channel == 0) {
        old_type = AXDriver_804D603C >> 8 & 0xF;
        AXDriver_804D603C &= ~(old_type << 8);
        AXDriver_804D603C |= (type << 8);
        AXRegisterAuxACallback(NULL, NULL);
    } else {
        old_type = AXDriver_804D603C >> 12 & 0xF;
        AXDriver_804D603C &= ~(old_type << 12);
        AXDriver_804D603C |= (type << 12);
        AXRegisterAuxBCallback(NULL, NULL);
    }

    // Shutdown old effect
    switch (old_type) {
    case AXDRIVER_AUX_REVERB_HI:
        AXFXReverbHiShutdown(aux_data_hi);
        break;
    case AXDRIVER_AUX_REVERB_STD:
        AXFXReverbStdShutdown(aux_data_std);
        break;
    case AXDRIVER_AUX_CHORUS:
        AXFXChorusShutdown(aux_data_chorus);
        break;
    case AXDRIVER_AUX_DELAY:
        AXFXDelayShutdown(aux_data_delay);
        break;
    }

    // Initialize new effect
    switch (type) {
    case AXDRIVER_AUX_OFF:
        result = 1;
        break;
    case AXDRIVER_AUX_REVERB_HI:
        memcpy(aux_data_hi, param, sizeof(struct AXFX_REVERBHI));
        if (AXFXReverbHiInit(aux_data_hi) == 1) {
            callback = AX_EFFECT_CALLBACK(AXFXReverbHiCallback);
            callback_data = aux_data_hi;
            result = 1;
        }
        break;
    case AXDRIVER_AUX_REVERB_STD:
        memcpy(aux_data_std, param, sizeof(struct AXFX_REVERBSTD));
        if (AXFXReverbStdInit(aux_data_std) == 1) {
            callback = AX_EFFECT_CALLBACK(AXFXReverbStdCallback);
            callback_data = aux_data_std;
            result = 1;
        }
        break;
    case AXDRIVER_AUX_CHORUS:
        memcpy(aux_data_chorus, param, sizeof(struct AXFX_CHORUS));
        if (AXFXChorusInit(aux_data_chorus) == 1) {
            callback = AX_EFFECT_CALLBACK(AXFXChorusCallback);
            callback_data = aux_data_chorus;
            result = 1;
        }
        break;
    case AXDRIVER_AUX_DELAY:
        memcpy(aux_data_delay, param, sizeof(struct AXFX_DELAY));
        if (AXFXDelayInit(aux_data_delay) == 1) {
            callback = AX_EFFECT_CALLBACK(AXFXDelayCallback);
            callback_data = aux_data_delay;
            result = 1;
        }
        break;
    }

    // Register new effect callback
    if (channel == 0) {
        AXRegisterAuxACallback(callback, callback_data);
    } else {
        AXRegisterAuxBCallback(callback, callback_data);
    }

    return result;
}

s32 HSD_AudioGetAuxHeapSize(AXDriverAuxType type, void* param)
{
#ifdef MELEE_NATIVE
    if (type < AXDRIVER_AUX_OFF || type > AXDRIVER_AUX_DELAY ||
        (type != AXDRIVER_AUX_OFF && !param)) return 0;
    if (type == AXDRIVER_AUX_OFF) return 0;
    if (type == AXDRIVER_AUX_CHORUS) return 0x1680;
    u64 required = 0;
    if (type == AXDRIVER_AUX_DELAY) {
        struct AXFX_DELAY* delay = param;
        for (unsigned i = 0; i < 3; ++i) {
            if (delay->delay[i] < 6) return 0;
            required += (((u64) delay->delay[i] - 5) * 32 + 159) / 160 * 640;
        }
    } else {
        float pre_delay = type == AXDRIVER_AUX_REVERB_HI ?
            ((struct AXFX_REVERBHI*) param)->preDelay :
            ((struct AXFX_REVERBSTD*) param)->preDelay;
        if (!isfinite(pre_delay) || pre_delay < 0 || pre_delay > 0.1f) return 0;
        required = (u64) (32000.0f * pre_delay) * 4 * 3;
        if (type == AXDRIVER_AUX_REVERB_STD) {
            required += (1789 + 2 + 1999 + 2 + 433 + 2 + 149 + 2) * 4 * 3;
        } else {
            required += (1789 + 2 + 1999 + 2 + 2333 + 2 + 433 + 2 + 149 + 2) * 4 * 3;
            required += (47 + 2 + 73 + 2 + 67 + 2) * 4;
        }
    }
    return required <= INT32_MAX ? (s32) required : 0;
#else
    s32 result = 0;
    int i;
    int k;

    if (type < 0 || type > 4 || (type != 0 && param == NULL)) {
        return 0;
    }

    switch (type) {
    case AXDRIVER_AUX_OFF:
        break;

    case AXDRIVER_AUX_REVERB_HI: {
        s32 dims[8] = { 0x6FD, 0x7CF, 0x91D, 0x1B1, 0x95, 0x2F, 0x49, 0x43 };

        for (k = 0; k < 3; k++) {
            for (i = 0; i < 3; i++) {
                result += (dims[i] + 2) * 4;
                result += (dims[i + 3] + 2) * 4;
            }
            result += (dims[k + 5] + 2) * 4;
            result += ((s32) (32000.0F *
                              ((struct AXFX_REVERBHI*) param)->preDelay)) *
                      4;
        }
        break;
    }

    case AXDRIVER_AUX_REVERB_STD: {
        s32 dims[4] = { 0x6FD, 0x7CF, 0x1B1, 0x95 };

        for (k = 0; k < 3; k++) {
            for (i = 0; i < 2; i++) {
                result += (dims[i] + 2) * 4;
                result += (dims[i + 2] + 2) * 4;
            }
            result += ((s32) (32000.0F *
                              ((struct AXFX_REVERBSTD*) param)->preDelay)) *
                      4;
        }
        break;
    }

    case AXDRIVER_AUX_CHORUS:
        result = 0x1680;
        break;

    case AXDRIVER_AUX_DELAY: {
        struct AXFX_DELAY* delay = (struct AXFX_DELAY*) param;

        for (i = 0; i < 3; i++) {
            result += ((delay->delay[i] - 5) * 32 + 159) / 160 * 640;
        }
        break;
    }
    }

    return result;
#endif
}

bool AXDriver_8038E30C(s32 channel, s32 type, void* param, u8* heap,
                       size_t heap_size)
{
    if (channel < 0 || channel > 1) {
        return false;
    }
    if (type < 0 || type > 4 || (type != AXDRIVER_AUX_OFF && param == NULL)) {
        return false;
    }
#ifdef MELEE_NATIVE
    if (type != AXDRIVER_AUX_OFF) {
        s32 required = HSD_AudioGetAuxHeapSize(type, param);
        if (required <= 0 || !heap || ((uintptr_t) heap % _Alignof(float)) ||
            heap_size < (size_t) required || heap_size > UINT32_MAX ||
            (uintptr_t) heap > UINTPTR_MAX - heap_size) return false;
    }
    int old_interrupts = OSDisableInterrupts();
#endif
    AXDriver_804D77D4 = heap;
    axfxallocsize = 0;
    axfxmaxsize = heap_size;
#ifdef MELEE_NATIVE
    bool result = AXDriverSetupAux(channel, type, param);
    OSRestoreInterrupts(old_interrupts);
    return result;
#else
    return AXDriverSetupAux(channel, type, param);
#endif
}

bool AXDriver_8038E37C(AXDriverAuxType type, void* param)
{
    if (type < 0 || type > AXDRIVER_AUX_DELAY ||
        (type != AXDRIVER_AUX_OFF && param == NULL))
    {
        return false;
    }
    switch (type) {
    case AXDRIVER_AUX_OFF:
        break;
    case AXDRIVER_AUX_REVERB_HI:
        ((struct AXFX_REVERBHI*) param)->tempDisableFX = 0;
        ((struct AXFX_REVERBHI*) param)->time = 3.0F;
        ((struct AXFX_REVERBHI*) param)->preDelay = 0.1F;
        ((struct AXFX_REVERBHI*) param)->damping = 0.5F;
        ((struct AXFX_REVERBHI*) param)->coloration = 0.5F;
        ((struct AXFX_REVERBHI*) param)->crosstalk = 0.3F;
        ((struct AXFX_REVERBHI*) param)->mix = 0.5F;
        break;
    case AXDRIVER_AUX_REVERB_STD:
        ((struct AXFX_REVERBSTD*) param)->tempDisableFX = 0;
        ((struct AXFX_REVERBSTD*) param)->time = 1.48F;
        ((struct AXFX_REVERBSTD*) param)->preDelay = 0.002F;
        ((struct AXFX_REVERBSTD*) param)->damping = 0.64F;
        ((struct AXFX_REVERBSTD*) param)->coloration = 0.5F;
        ((struct AXFX_REVERBSTD*) param)->mix = 1.0F;
        break;
    case AXDRIVER_AUX_CHORUS:
        ((struct AXFX_CHORUS*) param)->baseDelay = 0xF;
        ((struct AXFX_CHORUS*) param)->variation = 0;
        ((struct AXFX_CHORUS*) param)->period = 0x1F4;
        break;
    case AXDRIVER_AUX_DELAY:
        ((struct AXFX_DELAY*) param)->delay[0] = 0x104;
        ((struct AXFX_DELAY*) param)->delay[1] = 0x136;
        ((struct AXFX_DELAY*) param)->delay[2] = 6;
        ((struct AXFX_DELAY*) param)->feedback[0] = 0x18;
        ((struct AXFX_DELAY*) param)->feedback[1] = 0x18;
        ((struct AXFX_DELAY*) param)->feedback[2] = 0;
        ((struct AXFX_DELAY*) param)->output[0] = 0x23;
        ((struct AXFX_DELAY*) param)->output[1] = 0x23;
        ((struct AXFX_DELAY*) param)->output[2] = 0;
        break;
    }
    return true;
}

void AXDriver_8038E498(int voices, int priority, int sample_rate,
                       int aram_size)
{
    int i;

    for (i = 0; i < 0x60; i++) {
        AXDriver_804C45A0[i].flags &= ~SMSTATE_MASK;
        unk_inline(&AXDriver_804C45A0[i], &AXDriver_804D7790);
    }

    HSD_SynthInit(voices, priority, sample_rate, aram_size);
    HSD_SynthSFXSetDriverMasterClockCallback(fn_8038CC1C);
    HSD_SynthSFXSetDriverInactivatedCallback(fn_8038CEA4);
    HSD_SynthSFXSetDriverPauseCallback(fn_8038CF48);

    axfxallocsize = 0;
    AXDriver_804D77D4 = NULL;
    axfxmaxsize = 0;
    AXDriverSetupAux(0, AXDRIVER_AUX_OFF, NULL);

    axfxallocsize = 0;
    AXDriver_804D77D4 = NULL;
    axfxmaxsize = 0;
    AXDriverSetupAux(1, AXDRIVER_AUX_OFF, NULL);
    AXFXSetHooks(AXDriverAlloc, AXDriverFree);
}

int AXDriver_8038E5D4(void)
{
    return AXDriver_804D77C8;
}

int AXDriver_8038E5DC(void)
{
    return AXDriver_804D77D0;
}

static bool AXDriver_8038E5E4(int vid)
{
    int idx;
    bool enabled;
    HSD_SM* v;

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    enabled = OSDisableInterrupts();
    if (v->vID != -1) {
        HSD_SynthSFXPause(v->vID);
    } else {
        HSD_ASSERT(0x5D6, (v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE);
        v->flags |= 0x20000000;
    }
    OSRestoreInterrupts(enabled);
    return true;
}

bool AXDriver_8038E6C0(int channel)
{
    bool enabled;
    HSD_SM* v;

    if (channel < 0 || channel >= 0x10) {
        return false;
    }
    enabled = OSDisableInterrupts();
    v = AXDriver_804D7794;
    while (v != NULL) {
        if ((v->flags & SMSTATE_MASK) && v->itdflag == channel) {
            AXDriver_8038E5E4(v->unk);
        }
        v = v->next;
    }
    AXDriver_804D77CC |= 1 << channel;
    OSRestoreInterrupts(enabled);
    return true;
}

static bool AXDriver_8038E768(int vid)
{
    int idx;
    bool enabled;
    HSD_SM* v;

    idx = vid & 0x7F;
    if ((vid < 0) || idx >= 0x60) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    enabled = OSDisableInterrupts();
    if (v->vID != -1) {
        HSD_SynthSFXResume(v->vID);
    } else {
        HSD_ASSERT(0x619, (v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE);
    }
    v->flags &= 0xDFFFFFFF;
    OSRestoreInterrupts(enabled);
    return true;
}

bool AXDriver_8038E844(int channel)
{
    bool enabled;
    HSD_SM* v;

    if ((channel < 0) || (channel >= 0x10)) {
        return false;
    }
    enabled = OSDisableInterrupts();
    v = AXDriver_804D7794;
    while (v != NULL) {
        if ((v->flags & SMSTATE_MASK) && v->itdflag == channel) {
            AXDriver_8038E768(v->unk);
        }
        v = v->next;
    }
    AXDriver_804D77CC &= ~(1 << channel);
    OSRestoreInterrupts(enabled);
    return true;
}

bool AXDriver_8038E8EC(const char* path, u8 volume, int track)
{
    int entrynum = DVDConvertPathToEntrynum(path);
    if (AXDriver_804D6038 != -1) {
        HSD_SynthSFXKeyOff(AXDriver_804D6038);
    }
    AXDriver_804D6038 = HSD_Synth_8038B5AC(entrynum, -1, volume, track);
    AXDriver_804D77E8 = AXDriver_804D778C;
    return true;
}

bool AXDriverStop(void)
{
    if (AXDriver_804D6038 == -1) {
        return false;
    }
    HSD_SynthSFXKeyOff(AXDriver_804D6038);
    AXDriver_804D6038 = -1;
    return true;
}

bool AXDriverPause(void)
{
    if (AXDriver_804D6038 == -1) {
        return false;
    }
    HSD_SynthSFXPause(AXDriver_804D6038);
    return true;
}

bool AXDriverResume(void)
{
    if (AXDriver_804D6038 == -1) {
        return false;
    }
    HSD_SynthSFXResume(AXDriver_804D6038);
    return true;
}

bool AXDriver_8038EA18(void)
{
    if (HSD_SynthSFXCheck(AXDriver_804D6038) == -1) {
        return false;
    }
    return true;
}
