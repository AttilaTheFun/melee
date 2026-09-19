#include "melee_ax_pool_mix.h"
#include "melee_ax_depop.h"
#include "melee_ax_voice.h"
#include <dolphin/os.h>
#include <string.h>
#include <stdio.h>

static u64 rendered_generation;
static MeleeAXDepop pending;
static AXPBDPOP last_rendered[AX_MAX_VOICES];

bool melee_ax_voice_pool_render_output(MeleeAXMixBuffers* buffers,
                                        const s32* previous_surround, unsigned mode)
{
    if (!buffers || mode > 3 || (mode < 2 && !previous_surround)) return false;
    int old = OSDisableInterrupts();
    bool success = false;
    u64 generation = melee_ax_voice_generation();
    if (!generation) goto finish;
    MeleeAXDepop next_pending = {0};
    AXPBDPOP next_last[AX_MAX_VOICES] = {0};
    AXPB next[AX_MAX_VOICES];
    AXPBITDBUFFER next_itd[AX_MAX_VOICES];
    if (rendered_generation == generation) {
        next_pending = pending;
        memcpy(next_last, last_rendered, sizeof(next_last));
    }
    /* Preserve the last actually rendered contribution even if a newly
     * acquired voice has replaced the user PB or explicitly set new dpop. */
    for (unsigned i = 0; i < AX_MAX_VOICES; ++i) {
        AXVPB* voice = melee_ax_voice_at(i);
        if (voice->priority < 0 || voice->priority >= AX_PRIORITY_STACKS) goto finish;
        if (voice->depop) melee_ax_depop_add(&next_pending, &next_last[i]);
    }
    MeleeAXMixBuffers output;
    melee_ax_depop_begin_frame(&next_pending, &output);
    if (mode < 2) {
        for (unsigned i = 0; i < MELEE_AX_FRAME_SAMPLES; ++i) {
            u32 left = (u32) previous_surround[i];
            if (mode == 1) left = 0U - left;
            memcpy(&output.main[0][i], &left, sizeof(left));
            output.main[1][i] = previous_surround[i];
            output.main[2][i] = 0;
        }
    }
    for (unsigned i = 0; i < AX_MAX_VOICES; ++i) {
        AXVPB* voice = melee_ax_voice_at(i);
        if (!voice->priority) {
            memset(&next_last[i], 0, sizeof(next_last[i]));
            continue;
        }
        next[i] = voice->pb;
        AXPBITDBUFFER* history = melee_ax_voice_itd_at(i);
        if (voice->itdBuffer != history) goto finish;
        next_itd[i] = *history;
        /* Match the original SDK sync precedence: a target-only update
         * suppresses the full ITD copy's buffer reset. */
        if ((voice->sync & AX_SYNC_FLAG_COPYITD) &&
            !(voice->sync & AX_SYNC_FLAG_COPYTSHIFT))
            memset(&next_itd[i], 0, sizeof(next_itd[i]));
        if (!(voice->sync & (AX_SYNC_FLAG_COPYDPOP | AX_SYNC_FLAG_COPYALL)))
            next[i].dpop = next_last[i];
        if (!melee_ax_mix_voice_frame_itd(&next[i], voice->updateData,
                                          voice->updateCounter, &output,
                                          &next_itd[i])) { fprintf(stderr,"AX voice %u rejected priority=%u state=%u fir=%u mixer=%x updates=%u\n",i,voice->priority,voice->pb.state,voice->pb.fir.numCoefs,voice->pb.mixerCtrl,voice->updateCounter); goto finish; }
        next_last[i] = next[i].dpop;
    }
    for (unsigned i = 0; i < AX_MAX_VOICES; ++i) {
        AXVPB* voice = melee_ax_voice_at(i);
        if (voice->priority) {
            voice->pb = next[i];
            *melee_ax_voice_itd_at(i) = next_itd[i];
            memset(voice->pb.update.updNum, 0, sizeof(voice->pb.update.updNum));
            voice->sync = 0;
            voice->updateMS = voice->updateCounter = 0;
            voice->updateWrite = voice->updateData;
        }
        voice->depop = 0;
    }
    pending = next_pending;
    memcpy(last_rendered, next_last, sizeof(last_rendered));
    rendered_generation = generation;
    *buffers = output;
    success = true;
finish:
    OSRestoreInterrupts(old);
    return success;
}

bool melee_ax_voice_pool_render(MeleeAXMixBuffers* buffers)
{
    return melee_ax_voice_pool_render_output(buffers, NULL, 2);
}
