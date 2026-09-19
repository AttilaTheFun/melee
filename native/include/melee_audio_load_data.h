#ifndef MELEE_NATIVE_AUDIO_LOAD_DATA_H
#define MELEE_NATIVE_AUDIO_LOAD_DATA_H
#include "melee_archive.h"
typedef struct MeleeAudioLoadData MeleeAudioLoadData;
/* Four locale tables of 30 lists, each terminated by sound id 0x83d60.
 * Owns all host pointers and endian-converted ids. */
MeleeAudioLoadData* melee_audio_load_data_decode(const MeleeArchive* archive);
int** melee_audio_load_data_group(MeleeAudioLoadData* data,unsigned group);
void melee_audio_load_data_free(MeleeAudioLoadData* data);
#endif
