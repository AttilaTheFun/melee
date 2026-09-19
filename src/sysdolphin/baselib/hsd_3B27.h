#ifndef SYSDOLPHIN_BASELIB_3B27_H
#define SYSDOLPHIN_BASELIB_3B27_H

#include <Runtime/platform.h>

#include <placeholder.h>

#ifdef MELEE_NATIVE
typedef intptr_t HSD_CardWord;
#else
typedef s32 HSD_CardWord;
#endif
/* Request words may contain native addresses or scalar command arguments. */
typedef struct HsdCmdEntry {
    union { HSD_CardWord type; HSD_CardWord x0; };
    union { HSD_CardWord f1; HSD_CardWord x4; };
    union { HSD_CardWord f2; HSD_CardWord x8; };
    union { HSD_CardWord f3; HSD_CardWord xC; };
    union { HSD_CardWord f4; HSD_CardWord x10; };
    union { HSD_CardWord f5; void (*x14)(s32, s32); };
} HsdCmdEntry;
#ifdef MELEE_NATIVE
extern HsdCmdEntry hsd_native_card_requests[32];
#define HSD_CARD_REQUESTS(base) hsd_native_card_requests
#else
#define HSD_CARD_REQUESTS(base) ((HsdCmdEntry*) ((base) + 0x1210))
#endif


/* 3B27F4 */ int hsd_803B27F4(const s32*, const char*, HSD_CardWord, HSD_CardWord,
                              void (*)(int, int));
/* 3B286C */ int hsd_803B286C(const s32*, UNK_T, const char*, HSD_CardWord, HSD_CardWord,
                              void (*)(int, int));
/* 3B2928 */ int hsd_803B2928(const s32*, const char*, HSD_CardWord, HSD_CardWord,
                              void (*)(int, int));
/* 3B29D8 */ int hsd_803B29D8(const s32* ctx, int channel, const u8* data,
                              UNK_T callback);
/* 3B2A4C */ int hsd_803B2A4C(const s32*, int, const u8*, void (*)(int, int));
/* 3B2ADC */ int hsd_803B2ADC(s32* ctx, UNK_T data);

#endif
