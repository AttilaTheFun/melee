#ifndef MELEE_NATIVE_KIRBY_COMPOSITE_COPY_H
#define MELEE_NATIVE_KIRBY_COMPOSITE_COPY_H
#include "melee_archive.h"
#include <melee/ft/types.h>
#include <melee/it/types.h>
typedef struct MeleeKirbyCompositeCopy MeleeKirbyCompositeCopy;
typedef struct {
 FtPartsDesc parts;struct ftData_x8_x8 textures;u32 replacement_mask;
 HSD_Joint* shared_joint;Article* laser;Article* blaster;
 /* Base Kirby has two model groups; the copied outline supplies only the
  * first. Keep remaining groups empty through the visibility system limit. */
 FtPartsVisLookup outline[11];float model_depth;u8 fill_rgba[4],outline_rgba[4];
 ftDynamics* dynamics;
 HSD_Joint* costume_joints[6];HSD_MatAnimJoint* costume_materials[6];
} MeleeKirbyCompositeCopyDesc;
/* Composite costume metadata and articles; separate costume archives are
 * loaded independently. Requires initialized HSD pools. */
MeleeKirbyCompositeCopy* melee_kirby_composite_copy_decode(const MeleeArchive*, FighterKind);
/* Transactionally installs one owned costume and its material animation. */
MeleeHostBool melee_kirby_composite_copy_bind_costume(MeleeKirbyCompositeCopy*,const MeleeArchive*,unsigned costume);
MeleeKirbyCompositeCopyDesc* melee_kirby_composite_copy_descriptor(MeleeKirbyCompositeCopy*);
void melee_kirby_composite_copy_free(MeleeKirbyCompositeCopy*);
#endif
