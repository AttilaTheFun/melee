#ifndef MELEE_NATIVE_LINK_ITEMS_H
#define MELEE_NATIVE_LINK_ITEMS_H
#include "melee_item_article.h"
typedef struct MeleeLinkItems MeleeLinkItems;
/* Full seven-entry Link/Young Link item table; requires initialized HSD pools. */
MeleeLinkItems* melee_link_items_decode(const MeleeArchive*,u32,int young);
void** melee_link_items_entries(MeleeLinkItems*);
void melee_link_items_free(MeleeLinkItems*);
#endif
