#ifndef MELEE_NATIVE_BATTLE_STAGE_H
#define MELEE_NATIVE_BATTLE_STAGE_H
#include "melee_archive.h"
#include <sysdolphin/baselib/archive.h>
HSD_Archive* melee_battle_stage_decode(const MeleeArchive*);
HSD_Archive* melee_homerun_stage_decode(const MeleeArchive*);
HSD_Archive* melee_target_stage_decode(const MeleeArchive*,const char* filename);
HSD_Archive* melee_stadium_stage_decode(const MeleeArchive*,MeleeHostBool transformation);
HSD_Archive* melee_bigblue_stage_decode(const MeleeArchive*);
HSD_Archive* melee_mutecity_stage_decode(const MeleeArchive*);
HSD_Archive* melee_corneria_stage_decode(const MeleeArchive*);
HSD_Archive* melee_venom_stage_decode(const MeleeArchive*);
HSD_Archive* melee_shrine_stage_decode(const MeleeArchive*);
HSD_Archive* melee_greens_stage_decode(const MeleeArchive*);
HSD_Archive* melee_yorster_stage_decode(const MeleeArchive*);
HSD_Archive* melee_castle_stage_decode(const MeleeArchive*);
HSD_Archive* melee_brinstar_stage_decode(const MeleeArchive*);
HSD_Archive* melee_japes_stage_decode(const MeleeArchive*);
HSD_Archive* melee_kongo_stage_decode(const MeleeArchive*);
HSD_Archive* melee_greatbay_stage_decode(const MeleeArchive*);
HSD_Archive* melee_story_stage_decode(const MeleeArchive*);
HSD_Archive* melee_fountain_stage_decode(const MeleeArchive*);
HSD_Archive* melee_dreamland_stage_decode(const MeleeArchive*);
/* These stages share the owned archive schema and particle registration. */
HSD_Archive* melee_final_stage_decode(const MeleeArchive*);
MeleeHostBool melee_battle_stage_register_particles(HSD_Archive*,int bank);
HSD_Archive* melee_fourside_stage_decode(const MeleeArchive*);
HSD_Archive* melee_icemt_stage_decode(const MeleeArchive*);
HSD_Archive* melee_flatzone_stage_decode(const MeleeArchive*);
HSD_Archive* melee_kraid_stage_decode(const MeleeArchive*);
HSD_Archive* melee_rcruise_stage_decode(const MeleeArchive*);
HSD_Archive* melee_figureget_stage_decode(const MeleeArchive*);
HSD_Archive* melee_target_fox_stage_decode(const MeleeArchive*);
HSD_Archive* melee_pura_stage_decode(const MeleeArchive*);
HSD_Archive* melee_zebes_route_stage_decode(const MeleeArchive*);
HSD_Archive* melee_bigblue_route_stage_decode(const MeleeArchive*);
HSD_Archive* melee_maze_stage_decode(const MeleeArchive*);
HSD_Archive* melee_kinoko_route_stage_decode(const MeleeArchive*);
HSD_Archive* melee_pushon_stage_decode(const MeleeArchive*);
HSD_Archive* melee_heal_stage_decode(const MeleeArchive*);
HSD_Archive* melee_inishie1_stage_decode(const MeleeArchive*);
HSD_Archive* melee_inishie2_stage_decode(const MeleeArchive*);
#endif
