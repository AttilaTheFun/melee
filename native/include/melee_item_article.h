#ifndef MELEE_NATIVE_ITEM_ARTICLE_H
#define MELEE_NATIVE_ITEM_ARTICLE_H
#include "melee_archive.h"
#include <melee/it/types.h>
typedef struct MeleeItemArticle MeleeItemArticle;
/* Caller supplies the state count from its item-kind schema, not adjacent
 * archive offsets. Supports common kinds 0..42 and Fox/Falco laser, blaster and
 * illusion/phantasm articles. Owns all returned data. */
MeleeItemArticle* melee_item_article_decode(const MeleeArchive*,unsigned kind,uint32_t offset,unsigned states);
Article* melee_item_article_descriptor(MeleeItemArticle*);
void melee_item_article_free(MeleeItemArticle*);
#endif
