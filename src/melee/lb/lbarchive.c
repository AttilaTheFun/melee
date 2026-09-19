#include "lbarchive.h"

#include <stdarg.h>
#include <string.h>

#include "lbdvd.h"
#include "lbfile.h"
#include "lbheap.h"
#include <dolphin/os.h>
#include <sysdolphin/baselib/archive.h>
#include <sysdolphin/baselib/debug.h>

#ifdef MELEE_NATIVE
#include "melee_menu.h"
#include "melee_scene_desc.h"
#include "melee_hud.h"
#include "melee_character_select.h"
static HSD_Archive* native_gameover_archives[4];
static HSD_Archive* native_intro_archives[6];
static HSD_Archive* native_score_archive;
static HSD_Archive* native_trophy_files;
static HSD_Archive* load_native_gameover(const char* filename)
{
    static const char* names[]={"GmGover.dat","GmGoCoin.dat","GmGoAnim.dat","GmRgStnd.dat"};
    HSD_Archive** cache=native_gameover_archives;
    for(unsigned i=0;i<4;i++)if(!strcmp(filename,names[i])){
        if(!cache[i]){
            void* bytes;size_t size;lbFile_80016760(filename,&bytes,&size);
            MeleeArchive view;HSD_ASSERT(__LINE__,melee_archive_open(&view,bytes,size));
            cache[i]=melee_single_scene_decode(&view,i==3?"standScene":"ScGamRegGover_scene_data");
            lbHeap_80015CA8(0,bytes);HSD_ASSERT(__LINE__,cache[i]);
        }
        return cache[i];
    }
    return NULL;
}
static struct {char name[64];HSD_Archive* archive;} native_cutscene_archives[32];
static HSD_Archive* load_native_cutscene(const char* filename)
{
    static const char* const scene_names[]={"Vi0401.dat","Vi0402.dat",
        "Vi0501.dat","Vi0502.dat","Vi0601.dat","Vi0801.dat",
        "Vi1101.dat","Vi1201v1.dat","Vi1201v2.dat","Vi1202.dat"};
    int scene=0;
    for(unsigned i=0;i<sizeof(scene_names)/sizeof(*scene_names);i++)
        if(!strcmp(filename,scene_names[i])){scene=1;break;}
    int hud=!strcmp(filename,"IfAll.dat");
    int intro=!strcmp(filename,"IrAls.dat");
    int result_motion=!strncmp(filename,"GmRstM",6)&&strstr(filename,".dat");
    int motion=!strncmp(filename,"Pl",2)&&strstr(filename,"DViWaitA");
    if(!scene&&!hud&&!motion&&!intro&&!result_motion)return NULL;
    unsigned slot;
    for(slot=0;slot<32;slot++)if(native_cutscene_archives[slot].archive&&!strcmp(filename,native_cutscene_archives[slot].name))return native_cutscene_archives[slot].archive;
    for(slot=0;slot<32;slot++)if(!native_cutscene_archives[slot].archive)break;
    HSD_ASSERT(__LINE__,slot<32&&strlen(filename)<64);
    void* bytes;size_t size;lbFile_80016760(filename,&bytes,&size);
    MeleeArchive a;HSD_ASSERT(__LINE__,melee_archive_open(&a,bytes,size));
    HSD_Archive* result=scene?melee_cutscene_decode(&a):hud?melee_hud_decode(&a):intro?melee_intro_decode(&a):result_motion?melee_demo_result_decode(&a):melee_demo_wait_decode(&a);
    lbHeap_80015CA8(0,bytes);HSD_ASSERT(__LINE__,result);
    strcpy(native_cutscene_archives[slot].name,filename);native_cutscene_archives[slot].archive=result;
    return result;
}
static HSD_Archive* native_menu_archive;
static HSD_Archive* load_native_menu(void)
{
    size_t length=0;void* owned=NULL;
    const char* filename=lbFileGetFullName("MnMaAll");
    const void* bytes=lbDvd_NativeGetRawData(DVDConvertPathToEntrynum(filename),&length);
    if(!bytes){lbFile_80016760("MnMaAll",&owned,&length);bytes=owned;}
    MeleeArchive view;HSD_ASSERT(__LINE__, melee_archive_open(&view,bytes,length));
    HSD_Archive* fresh=melee_menu_decode(&view);HSD_ASSERT(__LINE__, fresh);
    if(owned)lbHeap_80015CA8(0,owned);
    if(native_menu_archive)native_menu_archive->native_destroy(native_menu_archive);
    native_menu_archive=fresh;return fresh;
}
#endif

#ifdef MUST_MATCH
#pragma push
#pragma dont_inline on
#endif
void lbArchive_InitializeDAT(HSD_Archive* archive, void* data, size_t length)
{
    const char* symbol;
    int i = 0;

    if (HSD_ArchiveParse(archive, data, length) == -1) {
        OSReport("HSD_ArchiveParse error!\n");
        HSD_ASSERT(73, 0);
    }

    while (true) {
        symbol = HSD_ArchiveGetExtern(archive, i++);
        if (symbol != NULL) {
            HSD_ArchiveLocateExtern(archive, symbol, NULL);
        }
        if (symbol == NULL) {
            return;
        }
    }
}
#ifdef MUST_MATCH
#pragma pop
#endif

void lbArchive_LoadSections(HSD_Archive* archive, void** symbol, ...)
{
    const char* symbol_name;
    va_list symbols;

    va_start(symbols, symbol);
    for (; symbol != NULL; symbol = va_arg(symbols, void**)) {
        symbol_name = va_arg(symbols, const char*);
        *symbol = NULL;
        *symbol = HSD_ArchiveGetPublicAddress(archive, symbol_name);
        if (*symbol == NULL) {
            OSReport("Cannot find symbol %s.\n", symbol_name);
        }
    }
    va_end(symbols);
}

static inline HSD_Archive* lbArchive_LoadArchive_inline(const char* filename)
{
    HSD_Archive* archive;
    void* data;
    size_t length;

    data = lbHeap_80015BD0(0, OSRoundUp32B(lbFileGetSize(filename)));
    archive = lbHeap_80015BD0(0, sizeof(HSD_Archive));
    lbFile_8001668C(filename, data, &length);
    lbArchive_InitializeDAT(archive, data, length);
    return archive;
}

HSD_Archive* lbArchive_LoadArchive(const char* filename)
{
#ifdef MELEE_NATIVE
    bool css=!strcmp(filename,"MnSlChr.dat")||!strcmp(filename,"MnSlChr.usd");
    bool extra=!strcmp(filename,"MnExtAll.dat")||!strcmp(filename,"MnExtAll.usd");
    bool stage=!strcmp(filename,"MnSlMap.dat")||!strcmp(filename,"MnSlMap.usd");
    if(css||extra||stage){
        size_t length=0;void* owned=NULL;
        const void* bytes=lbDvd_NativeGetRawData(DVDConvertPathToEntrynum(filename),&length);
        if(!bytes){lbFile_80016760(filename,&owned,&length);bytes=owned;}
        MeleeArchive view;HSD_ASSERT(__LINE__, melee_archive_open(&view,bytes,length));
        HSD_Archive* result=css?melee_character_select_decode(&view):stage?melee_stage_selection_decode(&view):melee_menu_decode(&view);
        if(owned)lbHeap_80015CA8(0,owned);
        HSD_ASSERT(__LINE__, result);return result;
    }
#endif
    return lbArchive_LoadArchive_inline(filename);
}

static inline void lbArchive_vLoadSectionsFatal(HSD_Archive* archive,
                                                void** symbol, va_list symbols)
{
    const char* symbol_name;

    for (; symbol != NULL; symbol = va_arg(symbols, void**)) {
        symbol_name = va_arg(symbols, const char*);
        *symbol = NULL;
        *symbol = HSD_ArchiveGetPublicAddress(archive, symbol_name);
        if (*symbol == NULL) {
            OSReport("Cannot find symbol %s.\n", symbol_name);
            HSD_ASSERT(112, 0);
        }
    }
}

static inline void lbArchive_vLoadSections(HSD_Archive* archive, void** symbol,
                                           va_list symbols)
{
    const char* symbol_name;

    for (; symbol != NULL; symbol = va_arg(symbols, void**)) {
        symbol_name = va_arg(symbols, const char*);
        *symbol = NULL;
        *symbol = HSD_ArchiveGetPublicAddress(archive, symbol_name);
        if (*symbol == NULL) {
            OSReport("Cannot find symbol %s.\n", symbol_name);
        }
    }
}

HSD_Archive* lbArchive_LoadSymbols(const char* filename, void* symbols, ...)
{
    va_list sections;
    HSD_Archive* archive;
    void* data;
    size_t length;
    u8 _[8];

    va_start(sections, symbols);
#ifdef MELEE_NATIVE
    archive=load_native_cutscene(filename);
    if(archive){lbArchive_vLoadSectionsFatal(archive,symbols,sections);va_end(sections);return archive;}
    archive=load_native_gameover(filename);
    if(archive){lbArchive_vLoadSectionsFatal(archive,symbols,sections);va_end(sections);return archive;}

    if(!strcmp(filename,"GmRegEnd")){
        lbFile_80016760(filename,&data,&length);MeleeArchive view;
        HSD_ASSERT(__LINE__,melee_archive_open(&view,data,length));
        archive=melee_ending_decode(&view);lbHeap_80015CA8(0,data);HSD_ASSERT(__LINE__,archive);
        lbArchive_vLoadSectionsFatal(archive,symbols,sections);va_end(sections);return archive;
    }
    if(!strcmp(filename,"TyDataf.dat")){
        lbFile_80016760(filename,&data,&length);MeleeArchive view;
        HSD_ASSERT(__LINE__,melee_archive_open(&view,data,length));
        archive=melee_trophy_files_decode(&view);native_trophy_files=archive;lbHeap_80015CA8(0,data);HSD_ASSERT(__LINE__,archive);
        lbArchive_vLoadSectionsFatal(archive,symbols,sections);va_end(sections);return archive;
    }
    if (!strcmp(filename,"TyKoopa.dat")||!strcmp(filename,"TyKoopaR.dat")||
        melee_trophy_files_contains(native_trophy_files,filename)||!strncmp(filename,"TyMyc",5)||!strncmp(filename,"TyMap",5)||
        !strncmp(filename,"TySeri",6)||!strncmp(filename,"TyEtc",5)||
        !strncmp(filename,"TyPoke",6)||!strncmp(filename,"TyItem",6)||
        !strcmp(filename,"TyMcCmDs.dat")||!strcmp(filename,"TyMcR1Ds.dat")||
        !strcmp(filename,"TyMcR2Ds.dat")||!strcmp(filename,"TyStandD.dat")||!strcmp(filename,"TyQuesD.dat")) {
        lbFile_80016760(filename,&data,&length);
        MeleeArchive view;
        HSD_ASSERT(__LINE__,melee_archive_open(&view,data,length));
        archive=melee_trophy_decode(&view);
        lbHeap_80015CA8(0,data);
        HSD_ASSERT(__LINE__,archive);
        lbArchive_vLoadSectionsFatal(archive,symbols,sections);
        va_end(sections);return archive;
    }
    if (!strcmp(filename,"MnMaAll")) {
        archive=load_native_menu();
        lbArchive_vLoadSectionsFatal(archive,symbols,sections);
        va_end(sections);return archive;
    }
#endif

    data = lbHeap_80015BD0(0, OSRoundUp32B(lbFileGetSize(filename)));
    archive = lbHeap_80015BD0(0, sizeof(HSD_Archive));
    lbFile_8001668C(filename, data, &length);
    lbArchive_InitializeDAT(archive, data, length);
    lbArchive_vLoadSectionsFatal(archive, symbols, sections);

    va_end(sections);
    return archive;
}

HSD_Archive* lbArchive_80016DBC(const char* filename, void* symbols, ...)
{
    va_list sections;
    HSD_Archive* archive;
    void* data;
    size_t length;
    u8 _[8];

    va_start(sections, symbols);
#ifdef MELEE_NATIVE
    if(!strcmp(filename,"GmTrain")||!strcmp(filename,"IfHrNoCn")||!strcmp(filename,"IfHrReco")){
        lbFile_80016760(filename,&data,&length);MeleeArchive view;
        HSD_ASSERT(__LINE__,melee_archive_open(&view,data,length));
        archive=!strcmp(filename,"GmTrain")?melee_training_decode(&view):melee_homerun_hud_decode(&view);lbHeap_80015CA8(0,data);
        HSD_ASSERT(__LINE__,archive);
        lbArchive_vLoadSections(archive,symbols,sections);va_end(sections);return archive;
    }
    if(!strcmp(filename,"GmStRoll.dat")||!strcmp(filename,"NtAppro")){
        lbFile_80016760(filename,&data,&length);MeleeArchive view;
        HSD_ASSERT(__LINE__,melee_archive_open(&view,data,length));
        archive=!strcmp(filename,"NtAppro")?melee_approach_decode(&view):melee_staffroll_decode(&view);lbHeap_80015CA8(0,data);HSD_ASSERT(__LINE__,archive);
        lbArchive_vLoadSections(archive,symbols,sections);va_end(sections);return archive;
    }
    archive=load_native_gameover(filename);
    if(archive){lbArchive_vLoadSectionsFatal(archive,symbols,sections);va_end(sections);return archive;}

    if(!strcmp(filename,"GmRegClr")){
        HSD_Archive** score_slot=&native_score_archive;
        HSD_Archive* score=*score_slot;
        if(!score){
            lbFile_80016760(filename,&data,&length);
            MeleeArchive view;
            HSD_ASSERT(__LINE__,melee_archive_open(&view,data,length));
            score=melee_single_scene_decode(&view,"ScGamRegClear_scene_data");
            *score_slot=score;
            lbHeap_80015CA8(0,data);HSD_ASSERT(__LINE__,score);
        }
        lbArchive_vLoadSections(score,symbols,sections);
        va_end(sections);return score;
    }
    static const char* names[]={"IrAls","IrEzTarg","IrEzTuki","IrEzFigG","IrRdMap","IrNml"};
    HSD_Archive** cached=native_intro_archives;
    for(unsigned i=0;i<6;i++)if(!strcmp(filename,names[i])){
        if(!cached[i]){
            lbFile_80016760(filename,&data,&length);
            MeleeArchive view;
            HSD_ASSERT(__LINE__,melee_archive_open(&view,data,length));
            cached[i]=i==5?melee_adventure_intro_decode(&view):melee_intro_decode(&view);
            lbHeap_80015CA8(0,data);
            HSD_ASSERT(__LINE__,cached[i]);
        }
        archive=cached[i];
        lbArchive_vLoadSections(archive,symbols,sections);
        va_end(sections);return archive;
    }
#endif


    data = lbHeap_80015BD0(0, OSRoundUp32B(lbFileGetSize(filename)));
    archive = lbHeap_80015BD0(0, sizeof(HSD_Archive));
    lbFile_8001668C(filename, data, &length);
    lbArchive_InitializeDAT(archive, data, length);
    lbArchive_vLoadSections(archive, symbols, sections);

    va_end(sections);
    return archive;
}

void lbArchive_80016EFC(HSD_Archive* archive)
{
    HSD_ASSERT(0xFC, archive);
#ifdef MELEE_NATIVE
    if (archive->flags & HSD_ARCHIVE_NATIVE) {
        if(archive==native_trophy_files)native_trophy_files=NULL;
        if(archive==native_menu_archive)native_menu_archive=NULL;
        if(archive==native_score_archive)native_score_archive=NULL;
        for(unsigned i=0;i<6;i++)if(archive==native_intro_archives[i])native_intro_archives[i]=NULL;
        for(unsigned i=0;i<4;i++)if(archive==native_gameover_archives[i])native_gameover_archives[i]=NULL;
        for(unsigned i=0;i<32;i++)if(native_cutscene_archives[i].archive==archive)native_cutscene_archives[i].archive=NULL;
        archive->native_destroy(archive);return;
    }
#endif
    HSD_ASSERT(0xFD, archive->flags & HSD_ARCHIVE_DONT_FREE);
    lbHeap_80015CA8(0, (u32*) (archive->data - 0x20));
    lbHeap_80015CA8(0, (u32*) archive);
}

bool lbArchive_80016F80(HSD_Archive** archive, const char* filename)
{
    void* data;
    size_t length;
    HSD_Archive* var_r3;
    bool result;
    u8 _[8];

    var_r3 = lbDvd_8001819C(filename);
    if (var_r3 != NULL) {
        result = true;
    } else {
        HSD_Archive* tmp;
        data = lbHeap_80015BD0(0, OSRoundUp32B(lbFileGetSize(filename)));
        tmp = lbHeap_80015BD0(0, sizeof(HSD_Archive));
        lbFile_8001668C(filename, data, &length);
        lbArchive_InitializeDAT(tmp, data, length);
        var_r3 = tmp;
        result = false;
    }
    if (archive != NULL) {
        *archive = var_r3;
    }
    return result;
}

bool lbArchive_80017040(HSD_Archive** dst, const char* filename, void* symbols,
                        ...)
{
    void* tmp;
    HSD_Archive* archive2;
    HSD_Archive* archive;
    bool preloaded;
    va_list args;

    va_start(args, symbols);

    archive = lbDvd_8001819C(filename);
    if (archive != NULL) {
        preloaded = true;
    } else {
        // Inlined lbArchive_LoadArchive
        {
            void* data;
            size_t length;
            u32 pad;
            u32 pad2;
            data = lbHeap_80015BD0(0, OSRoundUp32B(lbFileGetSize(filename)));
            tmp = data;
            archive2 = lbHeap_80015BD0(0, sizeof(HSD_Archive));
            lbFile_8001668C(filename, tmp, &length);
            lbArchive_InitializeDAT(archive2, tmp, length);
            archive = archive2;
        }
        preloaded = false;
    }

    lbArchive_vLoadSectionsFatal(archive, symbols, args);

    va_end(args);

    if (dst != NULL) {
        *dst = archive;
    }
    return preloaded;
}

bool lbArchive_800171CC(HSD_Archive** dst, const char* filename, void* symbols,
                        ...)
{
    void* tmp;
    HSD_Archive* archive2;
    HSD_Archive* archive;
    bool preloaded;
    va_list args;

    va_start(args, symbols);

    archive = lbDvd_8001819C(filename);
    if (archive != NULL) {
        preloaded = true;
    } else {
        // Inlined lbArchive_LoadArchive
        {
            void* data;
            size_t length;
            u32 pad;
            u32 pad2;
            data = lbHeap_80015BD0(0, OSRoundUp32B(lbFileGetSize(filename)));
            tmp = data;
            archive2 = lbHeap_80015BD0(0, sizeof(HSD_Archive));
            lbFile_8001668C(filename, tmp, &length);
            lbArchive_InitializeDAT(archive2, tmp, length);
            archive = archive2;
        }
        preloaded = false;
    }

    lbArchive_vLoadSections(archive, symbols, args);

    va_end(args);

    if (dst != NULL) {
        *dst = archive;
    }
    return preloaded;
}

static inline void Locate(HSD_Archive* archive, intptr_t base_addr)
{
    u32 i;
    u32* ptr;

    for (i = 0; i < archive->header.nb_reloc; i++) {
        ptr = (u32*) archive->reloc_info[i].offset;
        *(intptr_t*) (archive->data + (u32) ptr) += base_addr;
    }
}

int lbArchiveRelocate(HSD_Archive* archive, u8* src, size_t file_size,
                      intptr_t base_addr)
{
    size_t file_offset;

    if (archive == NULL) {
        return -1;
    }
    memset(archive, 0, sizeof(HSD_Archive));
    archive->flags |= 1;
    memcpy(archive, src, sizeof(HSD_ArchiveHeader));

    if (archive->header.file_size != file_size) {
        OSReport("lbArchiveRelocate: byte-order mismatch! "
                 "Please check data format %x %x\n",
                 archive->header.file_size, file_size);
        return -1;
    }

    file_offset = sizeof(HSD_ArchiveHeader);
    if (archive->header.data_size != 0) {
        archive->data = src + file_offset;
        file_offset = archive->header.data_size + sizeof(HSD_ArchiveHeader);
    }
    if (archive->header.nb_reloc != 0) {
        archive->reloc_info = (HSD_ArchiveRelocationInfo*) (src + file_offset);
        file_offset +=
            archive->header.nb_reloc * sizeof(HSD_ArchiveRelocationInfo);
    }
    if (archive->header.nb_public != 0) {
        archive->public_info = (HSD_ArchivePublicInfo*) (src + file_offset);
        file_offset +=
            archive->header.nb_public * sizeof(HSD_ArchivePublicInfo);
    }
    if (archive->header.nb_extern != 0) {
        archive->extern_info = (HSD_ArchiveExternInfo*) (src + file_offset);
        file_offset +=
            archive->header.nb_extern * sizeof(HSD_ArchiveExternInfo);
    }
    if (file_offset < archive->header.file_size) {
        archive->symbols = (char*) (src + file_offset);
    }

    Locate(archive, base_addr);

    return 0;
}

#ifdef MELEE_NATIVE
SceneDesc* lbArchive_NativeLoadScene(const char* basename,MeleeSceneDesc** owner)
{
    size_t length = 0;
    void* owned = NULL;
    const char* filename = lbFileGetFullName(basename);
    const void* bytes = lbDvd_NativeGetRawData(
        DVDConvertPathToEntrynum(filename), &length);
    if (!bytes) {
        lbFile_80016760(basename, &owned, &length);
        bytes = owned;
    }
    MeleeArchive archive;
    uint32_t root;
    HSD_ASSERT(__LINE__, owner);
    HSD_ASSERT(__LINE__, melee_archive_open(&archive, bytes, length));
    HSD_ASSERT(__LINE__, melee_archive_find(&archive,
        "ScNtcCommon_scene_data", &root));
    MeleeSceneDesc* fresh = melee_scene_desc_decode(&archive, root);
    HSD_ASSERT(__LINE__, fresh);
    if (owned) lbHeap_80015CA8(0, owned);
    melee_scene_desc_free(*owner);
    *owner = fresh;
    return melee_scene_desc_data(fresh);
}
#endif
