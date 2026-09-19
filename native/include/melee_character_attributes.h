#ifndef MELEE_NATIVE_CHARACTER_ATTRIBUTES_H
#define MELEE_NATIVE_CHARACTER_ATTRIBUTES_H
#include "melee_archive.h"
#include <melee/ft/types.h>
#include <melee/ft/kinds/ftMario/types.h>
#include <melee/ft/kinds/ftCaptain/types.h>
#include <melee/ft/kinds/ftDonkey/types.h>
#include <melee/ft/kinds/ftKoopa/types.h>
#include <melee/ft/kinds/ftLuigi/types.h>
#include <melee/ft/kinds/ftMars/types.h>
#include <melee/ft/kinds/ftPikachu/types.h>
#include <melee/ft/kinds/ftPurin/types.h>
#include <melee/ft/kinds/ftYoshi/types.h>
#include <melee/ft/kinds/ftZelda/types.h>
#include <melee/ft/kinds/ftSeak/types.h>
#include <melee/ft/kinds/ftLink/types.h>
#include <melee/ft/kinds/ftSamus/types.h>
#include <melee/ft/kinds/ftPeach/types.h>
#include <melee/ft/kinds/ftMewtwo/types.h>
#include <melee/ft/kinds/ftPopo/types.h>
#include <melee/ft/kinds/ftGameWatch/types.h>
#include <melee/ft/kinds/ftKirby/types.h>
#include <melee/ft/kinds/ftMasterHand/types.h>
#include <melee/ft/kinds/ftCrazyHand/types.h>
MeleeHostBool melee_masterhand_attributes_decode(const MeleeArchive*,u32,struct ftMasterHand_SpecialAttrs*);
MeleeHostBool melee_crazyhand_attributes_decode(const MeleeArchive*,u32,ftCrazyHand_DatAttrs*);
/* The common per-character parameter block at ftData entry zero. */
MeleeHostBool melee_character_attributes_decode(const MeleeArchive*,u32 ft_data,ftCo_DatAttrs*);
/* Fox and Falco share this special-move parameter schema. */
MeleeHostBool melee_fox_attributes_decode(const MeleeArchive*,u32 ft_data,struct ftFox_DatAttrs*);
MeleeHostBool melee_ness_attributes_decode(const MeleeArchive*,u32 ft_data,ftNessAttributes*);
/* Mario and Dr. Mario share the original move implementation and layout. */
MeleeHostBool melee_mario_attributes_decode(const MeleeArchive*,u32 ft_data,ftMario_DatAttrs*);
MeleeHostBool melee_captain_attributes_decode(const MeleeArchive*,u32 ft_data,struct ftCaptain_DatAttrs*);
MeleeHostBool melee_donkey_attributes_decode(const MeleeArchive*,u32 ft_data,ftDonkeyAttributes*);
MeleeHostBool melee_luigi_attributes_decode(const MeleeArchive*,u32 ft_data,ftLuigiAttributes*);
MeleeHostBool melee_koopa_attributes_decode(const MeleeArchive*,u32 ft_data,ftKoopaAttributes*);
MeleeHostBool melee_mars_attributes_decode(const MeleeArchive*,u32 ft_data,MarsAttributes*);
MeleeHostBool melee_pikachu_attributes_decode(const MeleeArchive*,u32 ft_data,ftPikachuAttributes*);
MeleeHostBool melee_purin_attributes_decode(const MeleeArchive*,u32 ft_data,ftPurinAttributes*);
MeleeHostBool melee_yoshi_attributes_decode(const MeleeArchive*,u32 ft_data,ftYoshiAttributes*);
MeleeHostBool melee_link_attributes_decode(const MeleeArchive*,u32 ft_data,struct ftLk_DatAttrs*);
MeleeHostBool melee_samus_attributes_decode(const MeleeArchive*,u32 ft_data,ftSs_DatAttrs*);
MeleeHostBool melee_iceclimbers_attributes_decode(const MeleeArchive*,u32 ft_data,ftIceClimberAttributes*);
MeleeHostBool melee_gamewatch_attributes_decode(const MeleeArchive*,u32 ft_data,ftGameWatchAttributes*);
MeleeHostBool melee_mewtwo_attributes_decode(const MeleeArchive*,u32 ft_data,ftMewtwoAttributes*);
MeleeHostBool melee_peach_attributes_decode(const MeleeArchive*,u32 ft_data,ftPe_DatAttrs*);
MeleeHostBool melee_sheik_attributes_decode(const MeleeArchive*,u32 ft_data,ftSeakAttributes*);
MeleeHostBool melee_zelda_attributes_decode(const MeleeArchive*,u32 ft_data,ftZelda_DatAttrs*);
MeleeHostBool melee_kirby_attributes_decode(const MeleeArchive*,u32 ft_data,struct ftKb_DatAttrs*);
#endif
