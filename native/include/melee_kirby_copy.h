#ifndef MELEE_NATIVE_KIRBY_COPY_H
#define MELEE_NATIVE_KIRBY_COPY_H
#include "melee_archive.h"
#include <melee/ft/types.h>
typedef struct MeleeKirbyCopy MeleeKirbyCopy;
/* Fox, Ness, Mario, Dr. Mario and Luigi copy hats/articles. Requires initialized HSD pools. */
MeleeKirbyCopy* melee_kirby_copy_decode(const MeleeArchive*,FighterKind);
MeleeKirbyCopy* melee_kirby_copy_ness_decode(const MeleeArchive*);
KirbyHatStruct* melee_kirby_copy_descriptor(MeleeKirbyCopy*);
void melee_kirby_copy_free(MeleeKirbyCopy*);
#endif
