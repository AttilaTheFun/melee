#include "melee_item_special.h"
#include <melee/it/itCommonItems.h>
#include <melee/it/itCharItems.h>
#include <melee/it/kinds/itdosei.h>
#include <melee/it/kinds/itlipstickspore.h>
#include <melee/it/itPKFlash.h>
#include <melee/it/itPKThunder.h>
#include <melee/it/kinds/itkirbycutterbeam.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef struct {size_t size;uint64_t floats,raw;} Schema;
#define CHECK(type,bytes) _Static_assert(sizeof(type)==bytes,"Scalar special layout: " #type)
CHECK(ItCapsuleAttr,8);CHECK(itBoxAttributes,32);CHECK(itTaruAttributes,56);
CHECK(itKusudamaAttributes,48);CHECK(itTaruCann_DatAttrs,84);CHECK(itBombHeiAttributes,44);
CHECK(itBatAttributes,16);CHECK(ItLGunAttr,16);CHECK(itFlipper_DatAttrs,40);
CHECK(itSScopeAttributes,64);CHECK(itLipstickAttributes,16);CHECK(itHarisen_DatAttrs,4);
CHECK(FFlowerAttr,24);CHECK(itMBallAttributes,8);
_Static_assert(offsetof(itPokemonSpawn_DatAttrs,_pad)==180,"Pokemon spawn parameter prefix");CHECK(ItLGunRayAttr,12);
CHECK(ItLGunBeamAttr,20);CHECK(itHammerheadAttributes,8);
CHECK(itDoseiAttributes,24);CHECK(HeartContainerAttr,28);CHECK(MaximTomatoSpecialAttr,24);
CHECK(itStar_ItemVars,24);CHECK(itCommonGShellAttributes,64);CHECK(itRShell_Attrs,88);
CHECK(itMsBomb_Attrs,24);CHECK(StarRodAttributes,16);CHECK(itHammerData,12);
CHECK(StarRodStarAttrs,32);CHECK(itLipstickSporeAttributes,28);CHECK(ScopeBeamAttrs,128);
CHECK(ItEggAttributes,8);CHECK(itSword_UnkArticle1,48);
_Static_assert(offsetof(itSword_UnkArticle1,x1C)==28,"Sword color record offset");
CHECK(itMasterHandLaserAttributes,8);CHECK(itMasterHandBulletAttributes,4);CHECK(itCrazyHandBombAttributes,12);
CHECK(FoxLaserAttr,40);CHECK(FoxBlasterAttr,40);CHECK(FoxIllusionAttr,8);
CHECK(itUnkAttributes,20);CHECK(itDrMarioPillAttributes,20);
CHECK(itFlashAttributes,44);CHECK(itFlashExplAttributes,20);CHECK(itPKThunderAttributes,20);
CHECK(itTosakinto_Attrs,12);CHECK(MewVars,16);CHECK(itPokemonAttributes,28);CHECK(itLuckyAttributes,28);CHECK(itHitodemanAttributes,92);CHECK(itMarilAttributes,24);CHECK(itFushigibanaAttributes,12);CHECK(itHinoarashiAttributes,24);CHECK(itHououAttr,32);CHECK(itLugiaAttributes,68);
_Static_assert(offsetof(itPokemonAttributes,max)==8,"Pokemon scale/timer prefix");CHECK(itsonansAttributes,40);CHECK(itkireihanaAttributes,16);CHECK(itFireAttributes,16);CHECK(itThunderPokemonAttributes,16);CHECK(itFreezerAttributes,16);CHECK(itKamexAttributes,32);CHECK(itLizardonAttributes,48);CHECK(itMatadogasAttributes,16);
CHECK(itChicoritaAttr,20);CHECK(itChicoritaLeafAttr,20);CHECK(itKabigonAttributes,28);
_Static_assert(offsetof(itHassam_ItemVars,x20)==32,"Scizor attribute prefix");
CHECK(itYoshiEggThrowAttributes,8);
CHECK(itPikachutJoltGroundAttributes,16);CHECK(itPikachuthunderAttributes,12);
CHECK(itZeldaDinFireExplodeAttributes,20);
CHECK(itLinkBombAttributes,52);
CHECK(itSamusBombAttributes,16);CHECK(itSamusChargeShot_Attributes,32);CHECK(itSamusMissileAttributes,56);
_Static_assert(offsetof(itSamusGrappleAttributes,x60)==96,"Samus grapple scalar prefix");
_Static_assert(offsetof(itSeakChain_Attrs,x60)==0x60,"Sheik chain scalar prefix");
CHECK(itClimbersIceAttributes,52);CHECK(itClimbersBlizzardAttributes,20);
_Static_assert(offsetof(itClimbersStringAttributes,x20)==32,"Climbers rope scalar prefix");
CHECK(itMDisableAttributes,8);CHECK(itMewtwoShadowball_DatAttrs,48);
CHECK(itPeachToadSporeAttributes,16);
_Static_assert(offsetof(itPeachTurnipAttributes,x8)==8,"Peach turnip odds table");
CHECK(itKirbyCutterBeamAttributes,16);
CHECK(itTincleAttributes,88);
CHECK(itGreatFoxLaser_Attrs,16);
CHECK(itToolsAttributes,156);
CHECK(itCoinAttributes,76);
static const Schema schemas[]={
    [It_Kind_MasterHand_Laser]={8,3,0},
    [It_Kind_CrazyHand_Laser]={8,3,0},
    [It_Kind_MasterHand_Bullet]={4,1,0},
    [It_Kind_CrazyHand_Bullet]={4,1,0},
    [It_Kind_CrazyHand_Bomb]={12,7,0},
    [It_Kind_Coin]={76,0x7ffff,0},
    [It_Kind_Tools]={156,((UINT64_C(1)<<39)-1)&~(UINT64_C(1)<<3),0},
    [It_Kind_GreatFox_Laser]={16,0xf,0},
    [It_Kind_Tincle]={88,0x1e63f9,UINT64_C(1)<<21},
    [It_Kind_Kirby_CBeam]={16,0xf,0},
    /* Only the first lifetime float of itUnkAttributes exists in this article. */
    [It_Kind_Unk1]={4,1,0},
    [It_Kind_IceClimber_Ice]={52,0x6ff,0x100},
    [It_Kind_Kirby_IceClimberIce]={52,0x6ff,0x100},
    [It_Kind_IceClimber_Blizzard]={24,0x3f,0},
    [It_Kind_IceClimber_GumStrings]={36,0x2c,0x10},
    [It_Kind_Mewtwo_Disable]={8,3,0},
    [It_Kind_Kirby_MewtwoShadowBall]={48,0xeff,0},
    [It_Kind_Mewtwo_ShadowBall]={48,0xeff,0},
    [It_Kind_Peach_Turnip]={72,1,0},
    [It_Kind_Peach_Parasol]={4,0,0},
    [It_Kind_Peach_Toad]={4,0,0},
    [It_Kind_Kirby_PeachToad]={4,0,0},
    [It_Kind_Peach_ToadSpore]={16,0xf,0},
    [It_Kind_Kirby_PeachToadSpore]={16,0xf,0},
    [It_Kind_Samus_GBeam]={100,0x1ffdff7,0},
    [It_Kind_Samus_Bomb]={16,0xf,0},
    [It_Kind_Samus_Charge]={32,0xfd,0},
    [It_Kind_Kirby_SamusCharge]={32,0xfd,0},
    [It_Kind_Samus_Missile]={56,0x3fff,0},
    [It_Kind_Link_Boomerang]={68,0x1fff8,0},
    [It_Kind_CLink_Boomerang]={68,0x1fff8,0},
    [It_Kind_Kirby_LinkArrow]={36,0x1ff,0},
    [It_Kind_Kirby_CLinkArrow]={36,0x1ff,0},
    [It_Kind_Kirby_LinkBow]={8,0,0},
    [It_Kind_Kirby_CLinkBow]={8,0,0},
    [It_Kind_Link_Arrow]={36,0x1ff,0},
    [It_Kind_CLink_Arrow]={36,0x1ff,0},
    [It_Kind_Link_HShot]={84,0x1ff7f7,0},
    [It_Kind_CLink_HShot]={84,0x1ff7f7,0},
    [It_Kind_Link_Bomb]={52,0x1fe0,0},
    [It_Kind_CLink_Bomb]={52,0x1fe0,0},
    /* Bow and milk parameter blocks are unused scalar words. */
    [It_Kind_Link_Bow]={8,0,0},
    [It_Kind_CLink_Bow]={8,0,0},
    [It_Kind_CLink_Milk]={4,0,0},
    [It_Kind_Seak_NeedleThrow]={12,7,0},
    [It_Kind_Kirby_SeakNeedleThrow]={12,7,0},
    [It_Kind_Seak_NeedleHeld]={4,0,0},
    [It_Kind_Kirby_SeakNeedleHeld]={4,0,0},
    [It_Kind_Seak_Chain]={100,0x1e7fff2,0x18000c},
    [It_Kind_Zelda_DinFire]={48,0xfff,0},
    [It_Kind_Zelda_DinFire_Explode]={20,0x1f,0},
    [It_Kind_Yoshi_EggThrow]={8,3,0},
    [It_Kind_Yoshi_Star]={8,3,0},
    [It_Kind_Kirby_KoopaFlame]={24,0x3f,0},
    [It_Kind_Koopa_Flame]={24,0x3f,0},
    [It_Kind_Pikachu_Thunder]={12,7,0},
    [It_Kind_Pichu_Thunder]={12,7,0},
    [It_Kind_Kirby_PikachuTJolt_Ground]={16,15,0},
    [It_Kind_Kirby_PichuTJolt_Ground]={16,15,0},
    [It_Kind_Kirby_PikachuTJolt_Air]={4,0,0},
    [It_Kind_Kirby_PichuTJolt_Air]={4,0,0},
    [It_Kind_Pikachu_TJolt_Ground]={16,15,0},
    [It_Kind_Pichu_TJolt_Ground]={16,15,0},
    /* Air jolt has an unused scalar word. */
    [It_Kind_Pikachu_TJolt_Air]={4,0,0},
    [It_Kind_Pichu_TJolt_Air]={4,0,0},
    [It_Kind_Luigi_Fire]={16,0xf,0},
    [It_Kind_Mario_Fire]={20,0x1f,0},
    [It_Kind_DrMario_Vitamin]={20,0x1f,0},
    /* Cape/sheet share an unused single-word attribute block. */
    [It_Kind_Mario_Cape]={4,0,0},
    [It_Kind_DrMario_Sheet]={4,0,0},
    [It_PKind_Tosakinto]={12,7,0},
    [It_PKind_Chicorita]={20,0x1d,0},
    [It_Kind_Chicorita_Leaf]={4,1,0},
    [It_PKind_Kabigon]={28,0x4f,0},
    [It_PKind_Hassam]={36,0x3f,0},
    [It_PKind_Kamex]={32,0xfd,0},
    [It_Kind_Kamex_HydroPump]={4,1,0},
    [It_PKind_Pippi]={24,1,0},
    [It_PKind_Togepy]={28,1,0},
    [It_PKind_Lucky]={28,0xf,0},
    [It_Kind_Lucky_Egg]={8,1,0},
    [It_PKind_Hitodeman]={92,0x7c7fff,0},
    [It_Kind_Hitodeman_Star]={4,1,0},
    [It_PKind_Maril]={24,0x3f,0},
    [It_PKind_Fushigibana]={12,7,0},
    [It_PKind_Hinoarashi]={12,7,0},
    [It_Kind_Hinoarashi_Flame]={24,0x3f,0},
    [It_PKind_Mew]={16,0xf,0},
    [It_PKind_Cerebi]={16,0xf,0},
    [It_PKind_Houou]={32,0xdd,2},
    [It_Kind_Houou_SacredFire]={4,1,0},
    [It_PKind_Lugia]={68,0x1ffdf,0},
    [It_Kind_Lugia_Aeroblast]={8,3,0},
    [It_Kind_Lugia_Aeroblast2]={12,7,0},
    [It_Kind_Lugia_Aeroblast3]={16,0xf,0},
    [It_PKind_Unknown]={36,0x3f,0},
    [It_Kind_Unknown_Swarm]={36,0x1fe,0},
    [It_PKind_Marumine]={16,1,0},
    [It_PKind_Entei]={8,1,0},
    [It_PKind_Raikou]={8,1,0},
    [It_PKind_Suikun]={8,1,0},
    [It_PKind_Sonans]={40,0x1ff,0},
    [It_PKind_Kireihana]={16,1,0},
    [It_PKind_Fire]={16,0xf,0},
    [It_PKind_Thunder]={16,7,0},
    [It_PKind_Freezer]={16,7,0},
    [It_PKind_Lizardon]={48,0x1d,0},
    [It_Kind_Lizardon_Flame1]={8,3,0},
    [It_Kind_Lizardon_Flame2]={8,3,0},
    [It_Kind_Lizardon_Flame3]={8,3,0},
    [It_Kind_Lizardon_Flame4]={8,3,0},
    [It_PKind_Matadogas]={16,0xf,0},
    [It_Kind_Matadogas_Gas1]={8,3,0},
    [It_Kind_Matadogas_Gas2]={12,7,0},
    /* PK Fire consumes two floats; its pillar consumes a third scale. */
    [It_Kind_Ness_PKFire]={8,3,0},
    [It_Kind_Ness_PKFire_Flame]={12,7,0},
    [It_Kind_Kirby_MarioFire]={20,0x1f,0},
    [It_Kind_Kirby_DrMarioVitamin]={20,0x1f,0},
    [It_Kind_Kirby_LuigiFire]={16,0xf,0},
    [It_Kind_Kirby_NessPKFlush]={44,0x7ff,0},
    [It_Kind_Kirby_NessPKFlush_Explode]={20,0x1f,0},
    [It_Kind_Ness_PKFlush]={44,0x7ff,0},
    [It_Kind_Ness_PKFlush_Explode]={20,0x1f,0},
    [It_Kind_Ness_PKThunder]={20,0x1f,0},
    [It_Kind_Ness_PKThunder1]={4,1,0},
    [It_Kind_Ness_PKThunder2]={4,1,0},
    [It_Kind_Ness_PKThunder3]={4,1,0},
    [It_Kind_Ness_PKThunder4]={4,1,0},
    [It_Kind_Ness_Bat]={4,0,0},
    [It_Kind_Kirby_FalcoLaser]={40,0x3ff,0},
    [It_Kind_Kirby_FalcoBlaster]={40,0x3ff,0},
    [It_Kind_Kirby_FoxLaser]={40,0x3ff,0},
    [It_Kind_Kirby_FoxBlaster]={40,0x3ff,0},
    [It_Kind_Fox_Laser]={40,0x3ff,0},
    [It_Kind_Falco_Laser]={40,0x3ff,0},
    [It_Kind_Fox_Blaster]={40,0x3ff,0},
    [It_Kind_Falco_Blaster]={40,0x3ff,0},
    [It_Kind_Fox_Illusion]={8,3,0},
    [It_Kind_Falco_Phantasm]={8,3,0},
    [It_Kind_EvYoshiEgg]={sizeof(itEvYoshiEgg_DatAttrs),0,0},
    [It_Kind_Egg]={8,0,0},
    [It_Kind_Sword]={48,0x1b8,0xe00},
    [It_Kind_Freeze]={44,0x7ff,0},
    [It_Kind_ScBall]={4,0,0},
    [It_Kind_RabbitC]={4,1,0},
    [It_Kind_MetalB]={12,7,0},
    [It_Kind_Spycloak]={4,1,0},
    [It_Kind_Dosei]={24,0x5,0},
    [It_Kind_Heart]={28,0x60,0},
    [It_Kind_Tomato]={24,0x20,0},
    [It_Kind_Star]={24,0x3f,0},
    [It_Kind_Parasol]={16,0xf,0},
    [It_Kind_ZGShell]={72,0x3ffff,0},
    [It_Kind_ZRShell]={72,0x3ffff,0},
    [It_Kind_G_Shell]={64,0xffbf,0x40},
    [It_Kind_R_Shell]={88,0x1fcfff,0x3000},
    [It_Kind_MSBomb]={24,0x3f,0},
    [It_Kind_StarRod]={16,0xe,0},
    [It_Kind_Hammer]={12,0x4,0},
    [It_Kind_StarRod_Star]={32,0x9f,0},
    [It_Kind_LipStick_Spore]={28,0x1f,0},
    [It_Kind_S_Scope_Beam]={128,0xc7ffffff,0x38000000},
    [It_Kind_Capsule]={8,0,0},
    [It_Kind_Box]={32,0xe0,0},
    [It_Kind_Taru]={56,0x3ffc,0},
    [It_Kind_Kusudama]={48,0xdc0,0x200},
    [It_Kind_TaruCann]={84,0x1c03fd,0x2},
    [It_Kind_BombHei]={44,0x7ff,0},
    [It_Kind_Bat]={16,0x8,0},
    [It_Kind_L_Gun]={16,0xe,0},
    [It_Kind_Flipper]={40,0x3d8,0},
    /* Scope's first word is named padding, but its spawn code reads ammo there. */
    [It_Kind_S_Scope]={64,0xfff8,0},
    [It_Kind_LipStick]={16,0xe,0},
    [It_Kind_Harisen]={4,0x1,0},
    [It_Kind_F_Flower]={24,0x30,0},
    [It_Kind_M_Ball]={180,0x3fff,0},
    [It_Kind_L_Gun_Ray]={12,0x7,0},
    [It_Kind_L_Gun_Beam]={20,0x1f,0},
    [It_Kind_Hammer_Head]={8,0x3,0},
    [It_Kind_F_Flower_Flame]={4,0x1,0},
};
size_t melee_item_special_size(unsigned kind){return kind<sizeof(schemas)/sizeof(*schemas)?schemas[kind].size:0;}
void* melee_item_special_decode(const MeleeArchive* a,unsigned kind,uint32_t at){
    if(!a||kind>=sizeof(schemas)/sizeof(*schemas))return NULL;
    if(kind==It_Kind_EvYoshiEgg){
        u32 count,target;MeleeHostBool present;
        if((uint64_t)at+8>a->data_size||!melee_archive_u32(a,at,&count)||
            !melee_archive_pointer(a,at+4,&target,&present)||present)return NULL;
        itEvYoshiEgg_DatAttrs* attrs=calloc(1,sizeof(*attrs));if(!attrs)return NULL;
        memcpy(&attrs->x0,&count,4);return attrs;
    }
    if(kind==It_Kind_Peach_Turnip){u32 count;if(!melee_archive_u32(a,at+4,&count)||count!=8)return NULL;}
    Schema s=schemas[kind];
    if(!s.size||at>a->data_size||s.size>a->data_size-at)return NULL;
    u8* result=calloc(1,kind==It_Kind_M_Ball?sizeof(itPokemonSpawn_DatAttrs):s.size);if(!result)return NULL;
    for(unsigned i=0;i<s.size/4;i++){
        if(s.raw&(UINT64_C(1)<<i)){memcpy(result+4*i,a->bytes+32+at+4*i,4);continue;}
        u32 bits;if(!melee_archive_u32(a,at+4*i,&bits))goto fail;
        if(s.floats&(UINT64_C(1)<<i)){float v;memcpy(&v,&bits,4);if(!isfinite(v))goto fail;}
        memcpy(result+4*i,&bits,4);
    }
    return result;
fail:free(result);return NULL;
}
