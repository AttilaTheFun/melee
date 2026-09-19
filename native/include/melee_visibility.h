#ifndef MELEE_NATIVE_VISIBILITY_H
#define MELEE_NATIVE_VISIBILITY_H
#include "melee_archive.h"
typedef struct MeleeVisibility MeleeVisibility;
struct HSD_DObj;
/* Borrow a real scene's drawable list. Only controlled hidden bits are copied;
 * transparency/pass flags and uncontrolled objects are preserved. Objects must
 * outlive this visibility owner. */
MeleeHostBool melee_visibility_bind(MeleeVisibility*,struct HSD_DObj* const*,size_t count);
/* Decode one FtPartsDesc table. Drawable indices must follow fighter preorder,
 * including drawables with no polygon stream. Table 2 uses its own low-poly
 * drawable list. Costume entries fall back to costume zero, as in ftParts. */
MeleeVisibility* melee_visibility_decode(const MeleeArchive*,uint32_t descriptor,
    size_t costume,unsigned table,size_t drawable_count);
/* Decode an explicit lookup array, including Game & Watch's fifth table from
 * item data. UINT32_MAX denotes an absent lookup. */
MeleeVisibility* melee_visibility_decode_lookup(const MeleeArchive*,uint32_t groups,
    uint32_t lookup,size_t drawable_count);
void melee_visibility_free(MeleeVisibility*);
size_t melee_visibility_group_count(const MeleeVisibility*);
size_t melee_visibility_choice_count(const MeleeVisibility*,size_t group);
/* Selectors use the original signed-byte range. -1 (or any absent choice)
 * hides every entry in that group. Uses original ftParts hide/select routines.
 * A selector may exist in another table but be absent from this table. */
MeleeHostBool melee_visibility_select(MeleeVisibility*,const int* choices,size_t count);
MeleeHostBool melee_visibility_hidden(const MeleeVisibility*,size_t drawable);
MeleeHostBool melee_visibility_controls(const MeleeVisibility*,size_t drawable);
/* Set initialization defaults through the original ftParts default setter.
 * Command 32 restores them; command 33 clears all selections. */
MeleeHostBool melee_visibility_defaults(MeleeVisibility*,const int*,size_t count);
/* Execute packed model command 31/32/33 through the original ftAction handlers. */
MeleeHostBool melee_visibility_command(MeleeVisibility*,uint32_t word);
/* Run supported original fighter reset callbacks, then restore their defaults.
 * kind uses FighterKind numbering. Mario, Fox, Peach, Game & Watch; Kirby uses the model-default portion only. */
MeleeHostBool melee_visibility_init_fighter(MeleeVisibility*,uint32_t kind);
#endif
