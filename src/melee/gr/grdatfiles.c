#include "grdatfiles.h"
#ifdef MELEE_NATIVE
#include "melee_onett_stage.h"
#include "melee_battle_stage.h"
#include <melee/lb/lbfile.h>
#include <melee/lb/lbdvd.h>
#include <dolphin/dvd.h>
#include <string.h>
#endif

#include "ground.h"
#include "types.h"
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbarchive.h>
#include <melee/lb/lbheap.h>
#include <sysdolphin/baselib/archive.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/particle.h>
#include <sysdolphin/baselib/psstructs.h>

#ifdef MELEE_NATIVE
static HSD_Archive* native_stage_archives[4];
bool grDatFiles_NativeRelease(HSD_Archive* archive)
{
    for(unsigned i=0;i<4;i++)if(archive&&native_stage_archives[i]==archive){
        native_stage_archives[i]=NULL;archive->native_destroy(archive);return true;
    }
    return false;
}

#endif

/* 1C6228 */ static void grDatFiles_801C6228(UnkStageDat*);
/* 1C62B4 */ static UnkArchiveStruct* grDatFiles_801C62B4(void);

/// @todo Merge declaration and definition
/* static */ extern GroundParam grDatFiles_803E0848;

/// @todo Merge declaration and definition
/* static */ extern UnkStageDat grDatFiles_803E0924;

void grDatFiles_801C5FC0(HSD_Archive* archive, void* data, size_t length)
{
    HSD_Archive* map_ptcl;
    HSD_Archive* map_texg;
    lbArchive_InitializeDAT(archive, data, length);
    map_ptcl = HSD_ArchiveGetPublicAddress(archive, "map_ptcl");
    map_texg = HSD_ArchiveGetPublicAddress(archive, "map_texg");

    if (map_ptcl != NULL && map_texg != NULL) {
        psInitDataBankLocate(map_ptcl, map_texg, NULL);
    }
}

void grDatFiles_801C6038(void* arg0, s32 arg1, s32 arg2)
{
    UnkArchiveStruct* temp_r3 = grDatFiles_801C62B4();
    if (arg0 != NULL) {
#ifdef MELEE_NATIVE
        OSReport("Native stage archive: %s\n", (const char*)arg0);
#endif
        HSD_Archive* sp14;
        s32 phi_r28;
        void* r4 = arg0;
#ifdef MELEE_NATIVE
        bool homerun=!strcmp(arg0,"/GrHr")||!strcmp(arg0,"GrHr.dat")||!strcmp(arg0,"GrHr.usd");
        bool bigblue_route=!strcmp(arg0,"/GrNBr.dat")||!strcmp(arg0,"GrNBr.dat");
        bool zebes_route=!strcmp(arg0,"/GrNZr.dat")||!strcmp(arg0,"GrNZr.dat");
        bool maze=!strcmp(arg0,"/GrNSr.dat")||!strcmp(arg0,"GrNSr.dat");
        bool kinoko_route=!strcmp(arg0,"/GrNKr.dat")||!strcmp(arg0,"GrNKr.dat");
        bool inishie2=!strcmp(arg0,"/GrI2.dat")||!strcmp(arg0,"GrI2.dat");
        bool inishie1=!strcmp(arg0,"/GrI1.dat")||!strcmp(arg0,"GrI1.dat");
        bool heal=!strcmp(arg0,"/GrHe.dat")||!strcmp(arg0,"GrHe.dat");
        bool pushon=!strcmp(arg0,"/GrNPo.dat")||!strcmp(arg0,"GrNPo.dat");
        bool figureget=!strcmp(arg0,"/GrNFg.dat")||!strcmp(arg0,"GrNFg.dat");
        const char* target_name=(const char*)arg0;if(*target_name=='/')target_name++;
        bool target_stage=strlen(target_name)==9&&!strncmp(target_name,"GrT",3)&&!strcmp(target_name+5,".dat");
        bool pura=!strcmp(arg0,"/GrPu.dat")||!strcmp(arg0,"GrPu.dat");
        bool rcruise=!strcmp(arg0,"/GrRc.dat")||!strcmp(arg0,"GrRc.dat");
        bool kraid=!strcmp(arg0,"/GrKr.dat")||!strcmp(arg0,"GrKr.dat");
        bool flatzone=!strcmp(arg0,"/GrFz.dat")||!strcmp(arg0,"GrFz.dat");
        bool icemt=!strcmp(arg0,"/GrIm.dat")||!strcmp(arg0,"GrIm.dat");
        bool fourside=!strcmp(arg0,"/GrFs.dat")||!strcmp(arg0,"GrFs.dat");
        bool bigblue=!strcmp(arg0,"/GrBb.dat")||!strcmp(arg0,"GrBb.dat");
        bool mutecity=!strcmp(arg0,"/GrMc.dat")||!strcmp(arg0,"GrMc.dat");
        bool corneria=!strcmp(arg0,"/GrCn")||!strcmp(arg0,"GrCn.usd")||!strcmp(arg0,"GrCn.dat");
        bool venom=!strcmp(arg0,"/GrVe")||!strcmp(arg0,"GrVe.usd")||!strcmp(arg0,"GrVe.dat");
        bool shrine=!strcmp(arg0,"/GrSh.dat")||!strcmp(arg0,"GrSh.dat");
        bool greens=!strcmp(arg0,"/GrGr.dat")||!strcmp(arg0,"GrGr.dat");
        bool yorster=!strcmp(arg0,"/GrYt.dat")||!strcmp(arg0,"GrYt.dat");
        bool castle=!strcmp(arg0,"/GrCs.dat")||!strcmp(arg0,"GrCs.dat");
        bool brinstar=!strcmp(arg0,"/GrZe.dat")||!strcmp(arg0,"GrZe.dat");
        bool japes=!strcmp(arg0,"/GrGd.dat")||!strcmp(arg0,"GrGd.dat");
        bool kongo=!strcmp(arg0,"/GrKg.dat")||!strcmp(arg0,"GrKg.dat");
        bool greatbay=!strcmp(arg0,"/GrGb.dat")||!strcmp(arg0,"GrGb.dat");
        bool stadium=!strcmp(arg0,"/GrPs")||!strcmp(arg0,"GrPs.usd")||!strcmp(arg0,"GrPs.dat");
        bool story=!strcmp(arg0,"/GrSt.dat")||!strcmp(arg0,"GrSt.dat");
        bool fountain=!strcmp(arg0,"/GrIz.dat")||!strcmp(arg0,"GrIz.dat");
        bool dreamland=!strcmp(arg0,"/GrOp.dat")||!strcmp(arg0,"GrOp.dat");
        bool battlefield=!strcmp(arg0,"/GrNBa")||!strcmp(arg0,"/GrNBa.dat")||!strcmp(arg0,"GrNBa.dat")||!strcmp(arg0,"GrNBa.usd");
        bool final_destination=!strcmp(arg0,"/GrNLa")||!strcmp(arg0,"/GrNLa.dat")||!strcmp(arg0,"GrNLa.dat")||!strcmp(arg0,"GrNLa.usd");
        if(homerun||inishie2||inishie1||heal||bigblue_route||zebes_route||maze||kinoko_route||pushon||figureget||target_stage||pura||rcruise||kraid||flatzone||icemt||fourside||bigblue||mutecity||corneria||venom||shrine||greens||yorster||castle||brinstar||japes||kongo||greatbay||stadium||story||fountain||dreamland||final_destination||battlefield||!strcmp(arg0,"/GrOt")||!strcmp(arg0,"GrOt.usd")||!strcmp(arg0,"GrOt.dat")){
            size_t length=0;void* owned=NULL;
            const void* bytes=lbDvd_NativeGetRawData(DVDConvertPathToEntrynum(lbFileGetFullName(arg0)),&length);
            if(!bytes){lbFile_80016760(arg0,&owned,&length);bytes=owned;}
            MeleeArchive raw;HSD_ASSERT(__LINE__,melee_archive_open(&raw,bytes,length));
            sp14=homerun?melee_homerun_stage_decode(&raw):inishie2?melee_inishie2_stage_decode(&raw):inishie1?melee_inishie1_stage_decode(&raw):heal?melee_heal_stage_decode(&raw):bigblue_route?melee_bigblue_route_stage_decode(&raw):zebes_route?melee_zebes_route_stage_decode(&raw):maze?melee_maze_stage_decode(&raw):kinoko_route?melee_kinoko_route_stage_decode(&raw):pushon?melee_pushon_stage_decode(&raw):figureget?melee_figureget_stage_decode(&raw):target_stage?melee_target_stage_decode(&raw,target_name):pura?melee_pura_stage_decode(&raw):rcruise?melee_rcruise_stage_decode(&raw):kraid?melee_kraid_stage_decode(&raw):flatzone?melee_flatzone_stage_decode(&raw):icemt?melee_icemt_stage_decode(&raw):fourside?melee_fourside_stage_decode(&raw):bigblue?melee_bigblue_stage_decode(&raw):mutecity?melee_mutecity_stage_decode(&raw):corneria?melee_corneria_stage_decode(&raw):venom?melee_venom_stage_decode(&raw):shrine?melee_shrine_stage_decode(&raw):greens?melee_greens_stage_decode(&raw):yorster?melee_yorster_stage_decode(&raw):castle?melee_castle_stage_decode(&raw):brinstar?melee_brinstar_stage_decode(&raw):japes?melee_japes_stage_decode(&raw):kongo?melee_kongo_stage_decode(&raw):greatbay?melee_greatbay_stage_decode(&raw):stadium?melee_stadium_stage_decode(&raw,false):story?melee_story_stage_decode(&raw):fountain?melee_fountain_stage_decode(&raw):dreamland?melee_dreamland_stage_decode(&raw):final_destination?melee_final_stage_decode(&raw):battlefield?melee_battle_stage_decode(&raw):melee_onett_stage_decode(&raw);HSD_ASSERT(__LINE__,sp14);
            unsigned owner_slot=0;while(owner_slot<4&&native_stage_archives[owner_slot])owner_slot++;
            HSD_ASSERT(__LINE__,owner_slot<4);native_stage_archives[owner_slot]=sp14;
            if(owned)lbHeap_80015CA8(0,owned);
            temp_r3->unk4=HSD_ArchiveGetPublicAddress(sp14,"map_head");phi_r28=0;
        }else
#endif
        if (arg2 != 0) {
            phi_r28 =
                lbArchive_800171CC(&sp14, r4, &temp_r3->unk4, "map_head", 0);
        } else {
            sp14 =
                lbArchive_80016DBC(r4, (void**) &temp_r3->unk4, "map_head", 0);
            phi_r28 = 0;
        }
        temp_r3->unk8 = 0;
        if (arg1 == 0) {
            stage_info.coll_data =
                HSD_ArchiveGetPublicAddress(sp14, "coll_data");
            stage_info.param =
                HSD_ArchiveGetPublicAddress(sp14, "grGroundParam");
            stage_info.itemdata =
                HSD_ArchiveGetPublicAddress(sp14, "itemdata");
            stage_info.ald_yaku_all =
                HSD_ArchiveGetPublicAddress(sp14, "ALDYakuAll");
            stage_info.map_ptcl =
                HSD_ArchiveGetPublicAddress(sp14, "map_ptcl");
            stage_info.map_texg =
                HSD_ArchiveGetPublicAddress(sp14, "map_texg");
            stage_info.yakumono_param =
                HSD_ArchiveGetPublicAddress(sp14, "yakumono_param");
            stage_info.map_plit =
                HSD_ArchiveGetPublicAddress(sp14, "map_plit");
            stage_info.quake_model_set =
                HSD_ArchiveGetPublicAddress(sp14, "quake_model_set");
        }
        temp_r3->unk0 = sp14;
        if (stage_info.map_ptcl != NULL && stage_info.map_texg != NULL) {
#ifdef MELEE_NATIVE
            if(sp14->flags&HSD_ARCHIVE_NATIVE){
                if(!melee_battle_stage_register_particles(sp14,0x40))melee_onett_stage_register_particles(sp14,0x40);
            }
            else
#endif
            if (phi_r28 != 0) {
                psInitDataBankLoad(0x40, stage_info.map_ptcl,
                                   stage_info.map_texg, 0, 0);
            } else {
                psInitDataBank(0x40, stage_info.map_ptcl, stage_info.map_texg,
                               0, 0);
            }
        }
        grDatFiles_801C6228(temp_r3->unk4);
    } else {
        temp_r3->unk4 = &grDatFiles_803E0924;
        if (arg1 == 0) {
            stage_info.coll_data = NULL;
            stage_info.param = &grDatFiles_803E0848;
            stage_info.itemdata = NULL;
            stage_info.ald_yaku_all = NULL;
            stage_info.map_ptcl = NULL;
            stage_info.map_texg = NULL;
            stage_info.yakumono_param = NULL;
            stage_info.map_plit = NULL;
            stage_info.x6C8 = NULL;
        }
        temp_r3->unk0 = (void*) -1;
    }
}

void grDatFiles_801C6228(UnkStageDat* arg0)
{
    if (arg0 != NULL && arg0->unk28 != NULL && arg0->unk2C != 0) {
        s32 i;
        for (i = 0; i < arg0->unk2C; i++) {
            UnkStageDatInternal* temp_r4 = arg0->unk28[i];
            if (temp_r4 != NULL) {
                temp_r4->unk4 |= 0x4000000;
            }
        }
    }
}

static UnkArchiveStruct grDatFiles_8049EE10[4];

void grDatFiles_801C6288(void)
{
#ifdef MELEE_NATIVE
    for(unsigned i=0;i<4;i++){
        HSD_Archive* archive=native_stage_archives[i];
        if(archive)archive->native_destroy(archive);
        native_stage_archives[i]=NULL;
    }
#endif
    memzero(&grDatFiles_8049EE10, sizeof(grDatFiles_8049EE10));
}

UnkArchiveStruct* grDatFiles_801C62B4(void)
{
    s32 i;
    for (i = 0; i < 4; i++) {
        if (grDatFiles_8049EE10[i].unk0 == NULL) {
            return &grDatFiles_8049EE10[i];
        }
    }
    HSD_ASSERT(229, 0);
}

UnkArchiveStruct* grDatFiles_GetArchive(void)
{
    return grDatFiles_8049EE10;
}

UnkArchiveStruct* grDatFiles_801C6330(s32 arg0)
{
    if (arg0 >= 0) {
        s32 i;
        for (i = 0; i < 4; i++) {
            if (grDatFiles_8049EE10[i].unk0 != NULL) {
                UnkStageDat* temp_r7 = grDatFiles_8049EE10[i].unk4;
                if (temp_r7 != NULL && temp_r7->unkC > arg0 &&
                    temp_r7->unk8[arg0].unk0 != 0)
                {
                    return &grDatFiles_8049EE10[i];
                }
            }
        }
    }
    return NULL;
}

UnkArchiveStruct* grDatFiles_801C6478(void* data, s32 length)
{
    UnkArchiveStruct* arc;

#ifdef MELEE_NATIVE
    HSD_Archive* archive;
    if(stage_info.grkind==Gr_Kind_PStadium){
        MeleeArchive raw;HSD_ASSERT(__LINE__,length>0&&melee_archive_open(&raw,data,length));
        archive=melee_stadium_stage_decode(&raw,true);HSD_ASSERT(__LINE__,archive);
        unsigned i=0;while(i<4&&native_stage_archives[i])i++;
        HSD_ASSERT(__LINE__,i<4);native_stage_archives[i]=archive;
    }else{
        archive=lbHeap_80015BD0(0,sizeof(HSD_Archive));lbArchive_InitializeDAT(archive,data,length);
    }
#else
    HSD_Archive* archive = lbHeap_80015BD0(0, sizeof(HSD_Archive));
    lbArchive_InitializeDAT(archive, data, length);
#endif
    arc = grDatFiles_801C62B4();
    HSD_ASSERT(290, arc);
    arc->unk0 = archive;
    arc->unk4 = HSD_ArchiveGetPublicAddress(archive, "map_head");
    arc->unk8 = 1;

    grDatFiles_801C6228(arc->unk4);

    return arc;
}

static StageParam grDatFiles_803E07E4 = {
    0, -1, -1, 0, 0, 0, 0, 0, { 0 },
};

GroundParam grDatFiles_803E0848 = {
    1,  0x80, { 0 }, 0x1E, 0,  1,     0x8000, 10,
    0,  0,    1,     1,    1,  { 0 }, 40,     10,
    50, 100,  10,    10,   10, 10,    false,  0,
    0,  0,    30,    10,   0,  0,     { 0 },  &grDatFiles_803E07E4,
    1,
};

UnkStageDat grDatFiles_803E0924 = { 0 };
