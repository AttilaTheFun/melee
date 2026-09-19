#ifndef MELEE_NATIVE_PIKACHU_ITEMS_H
#define MELEE_NATIVE_PIKACHU_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleePikachuItems MeleePikachuItems;
/* Thunder, ground jolt, air jolt. Requires initialized HSD pools. */
MeleePikachuItems* melee_pikachu_items_decode(const MeleeArchive*,u32,MeleeHostBool pichu);
void** melee_pikachu_items_entries(MeleePikachuItems*);
void melee_pikachu_items_free(MeleePikachuItems*);
#endif
