#include "melee_ax_stream.h"
#include "melee_ax_voice.h"
#include <dolphin/os.h>
#include <string.h>

bool melee_ax_stream_init(MeleeAXStream* stream)
{
    if (!stream) return false;
    /* The stopped-only contract permits initializing uninitialized storage. */
    int old = OSDisableInterrupts();
    memset(stream, 0, sizeof(*stream));
    stream->initialized = true;
    stream->position = MELEE_AX_FRAME_SAMPLES;
    OSRestoreInterrupts(old);
    return true;
}

bool melee_ax_stream_read(MeleeAXStream* stream, s16* stereo, size_t frames)
{
    if (!stream || frames > 4096 || (frames && !stereo)) return false;
    int old = OSDisableInterrupts();
    bool success = false;
    if (!stream->initialized || stream->pulling || stream->position > 160) goto finish;
    if (!frames) { success = true; goto finish; }
    stream->pulling = true;
    u64 generation = melee_ax_voice_generation();
    if (stream->generation != generation) {
        memset(&stream->output, 0, sizeof(stream->output));
        stream->position = MELEE_AX_FRAME_SAMPLES;
        stream->failed = false;
        stream->generation = generation;
    }
    size_t written = 0;
    while (written < frames && !stream->failed) {
        if (stream->position == MELEE_AX_FRAME_SAMPLES) {
            if (!melee_ax_output_frame(&stream->output, stream->pending)) {
                stream->failed = true;
                break;
            }
            stream->position = 0;
        }
        size_t count = MELEE_AX_FRAME_SAMPLES - stream->position;
        if (count > frames - written) count = frames - written;
        memcpy(stereo + written * 2, stream->pending + stream->position * 2,
               count * 2 * sizeof(s16));
        written += count;
        stream->position += count;
    }
    if (written < frames)
        memset(stereo + written * 2, 0, (frames - written) * 2 * sizeof(s16));
    success = !stream->failed;
    stream->pulling = false;
finish:
    OSRestoreInterrupts(old);
    return success;
}
