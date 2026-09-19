#include "melee_wireframe_data.h"
#include "melee_sandbag_data.h"
#include <melee/gr/grshrineroute.h>
#include "melee_hand_data.h"
#include "melee_hand_items.h"
#include "melee_character_attributes.h"
#include <melee/gr/grpushon.h>
#include "melee_item_special.h"
#include <melee/gr/gricemt.h>
#include <melee/gr/grmutecity.h>
#include <melee/gr/grcorneria.h>
#include <melee/gr/grvenom.h>
#include <melee/it/kinds/itarwinglaser.h>
#include <melee/gr/grcastle.h>
#include "melee_samus_data.h"
#include "melee_peach_data.h"
#include "melee_mewtwo_data.h"
#include "melee_gamewatch_data.h"
#include "melee_kirby_data.h"
#include "melee_kirby_copy.h"
#include "melee_kirby_composite_copy.h"
#include "melee_iceclimbers_data.h"
#include "melee_samus_items.h"
#include "melee_link_data.h"
#include "melee_link_items.h"
#include "melee_sheik_data.h"
#include "melee_sheik_items.h"
#include "melee_zelda_data.h"
#include "melee_pikachu_data.h"
#include "melee_yoshi_data.h"
#include "melee_pikachu_items.h"
#include "melee_zelda_items.h"
#include "melee_peach_items.h"
#include "melee_gamewatch_items.h"
#include "melee_mewtwo_items.h"
#include "melee_kirby_items.h"
#include "melee_iceclimbers_items.h"
#include <melee/it/kinds/itpeachturnip.h>
#include <sysdolphin/baselib/random.h>
#include "melee_yoshi_items.h"
#include "melee_mars_data.h"
#include "melee_purin_data.h"
#include <sysdolphin/baselib/psdisptev.h>
#include "melee_fighter_data.h"
#include "melee_fighter_models.h"
#include "melee_character_models.h"
#include "melee_fox_items.h"
#include "melee_ness_items.h"
#include <melee/it/itYoyo.h>
#include "melee_fox_data.h"
#include "melee_ness_data.h"
#include "melee_mario_items.h"
#include "melee_koopa_items.h"
#include "melee_luigi_items.h"
#include "melee_mario_data.h"
#include "melee_captain_data.h"
#include "melee_donkey_data.h"
#include "melee_koopa_data.h"
#include "melee_luigi_data.h"
#include <melee/ft/kinds/ftCommon/ftCo_09F4.h>
#include "melee_character_motions.h"
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/robj.h>
#include <sysdolphin/baselib/bytecode.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/list.h>
#include <sysdolphin/baselib/initialize.h>
#include <dolphin/os/OSAlloc.h>
#include "melee_scene.h"
#include "melee_joint.h"
#include "melee_scene_desc.h"
#include "melee_title.h"
#include "melee_menu.h"
#include "melee_hud.h"
#include <melee/if/ifstatus.h>
#include "melee_effects.h"
#include "melee_stage_models.h"
#include "melee_dynamic_model.h"
#include "melee_onett_stage.h"
#include "melee_battle_stage.h"
#include <melee/gr/grkongo.h>
#include <melee/gr/grzebes.h>
#include "melee_camera.h"
#include <melee/gr/granime.h>
#include "melee_item_model.h"
#include "melee_item_animation.h"
#include "melee_item_foods.h"
#include "melee_item_wstar.h"
#include "melee_item_mushroom.h"
#include "melee_item_article.h"
#include "melee_items_data.h"
#include "melee_character_select.h"
#include "melee_texture.h"
#include <melee/sc/types.h>
#include "fighter_selection.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#undef NDEBUG
#include <assert.h>
static u32 stage_ref(const MeleeArchive* a,u32 slot){u32 at;MeleeHostBool present;assert(melee_archive_pointer(a,slot,&at,&present));return present?at:UINT32_MAX;}
static unsigned check_stage_materials(const MeleeArchive* a,u32 disk,HSD_Joint* joint,u32 overrides,unsigned count){
    if(disk==UINT32_MAX){assert(!joint);return 0;}assert(joint);
    u32 flags;assert(melee_archive_u32(a,disk+4,&flags));assert(flags==joint->flags);
    unsigned checked=0;
    u32 dobj=(flags&(JOBJ_SPLINE|JOBJ_PTCL))?UINT32_MAX:stage_ref(a,disk+16);
    HSD_DObjDesc* d=(flags&(JOBJ_SPLINE|JOBJ_PTCL))?NULL:joint->u.dobjdesc;
    while(dobj!=UINT32_MAX){
        assert(d);u32 mat=stage_ref(a,dobj+8);
        if(mat!=UINT32_MAX){
            assert(d->mobjdesc);u32 expected;assert(melee_archive_u32(a,mat+4,&expected));
            for(unsigned i=0;i<count;i++)if(stage_ref(a,overrides+4*i)==mat)expected|=0x04000000;
            assert(d->mobjdesc->rendermode==expected);checked++;
        }else assert(!d->mobjdesc);
        dobj=stage_ref(a,dobj+4);d=d->next;
    }assert(!d);
    return checked+check_stage_materials(a,stage_ref(a,disk+8),joint->child,overrides,count)+
        check_stage_materials(a,stage_ref(a,disk+12),joint->next,overrides,count);
}
extern void melee_stock_probe(void);
extern void melee_training_probe(void);
extern void melee_hsd_video_gpu_init(void);
extern MeleeHostBool melee_gpu_begin(const char* cache);
extern MeleeHostBool melee_gpu_next_frame(void);
extern MeleeHostBool melee_gpu_finish(const char* output);
extern size_t aurora_fifo_copy(unsigned char* out,size_t capacity);
static void word(uint8_t* p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void archive_checks(void){
    uint8_t bytes[168]={0};uint8_t* d=bytes+32;
    word(bytes,sizeof(bytes));word(bytes+4,128);word(bytes+8,1);word(d+128,8);word(d+8,64);
    for(int j=0;j<2;j++)for(int k=0;k<3;k++)word(d+j*64+32+k*4,0x3F800000);
    word(d+44,0x40400000);word(d+64+44,0x40000000);
    MeleeArchive a;assert(melee_archive_open(&a,bytes,sizeof(bytes)));
    MeleeScene* s=melee_scene_decode(&a,0);assert(s&&melee_scene_joint_count(s)==2);
    word(d+44,0);HSD_JObjSetupMatrix(melee_scene_root(s)->child);
    assert(melee_scene_root(s)->child->mtx[0][3]==5);melee_scene_free(s);
    word(d+4,JOBJ_INSTANCE);assert(!melee_scene_decode(&a,0));word(d+4,0);
    word(d+16,20);assert(!melee_scene_decode(&a,0));word(d+16,0);
    word(d+8,0);assert(!melee_scene_decode(&a,0));word(d+8,64); /* Relocated cycle. */
    word(bytes+8,2);word(d+132,12);word(d+12,64);assert(melee_archive_open(&a,bytes,sizeof(bytes)));
    assert(!melee_scene_decode(&a,0)); /* Shared ordinary joint is not an instance. */
    size_t n=258,size=32+n*64+(n-1)*4;uint8_t* deep=calloc(1,size);assert(deep);
    word(deep,size);word(deep+4,n*64);word(deep+8,n-1);
    for(size_t j=0;j<n;j++){
        for(int k=0;k<3;k++)word(deep+32+j*64+32+k*4,0x3F800000);
        if(j+1<n){word(deep+32+j*64+8,(j+1)*64);word(deep+32+n*64+j*4,j*64+8);}
    }
    assert(melee_archive_open(&a,deep,size)&&!melee_scene_decode(&a,0));free(deep);
    assert(!melee_scene_decode(NULL,0));melee_scene_free(NULL);
    assert(!HSD_IDGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
}
static size_t check_expressions(HSD_JObj* joint){
    size_t count=0;
    for(;joint;joint=joint->next){
        for(HSD_DObj* d=joint->u.dobj;d;d=d->next)for(HSD_TObj* t=d->mobj?d->mobj->tobj:NULL;t;t=t->next){
            if(!t->aobj)continue;
            assert(t->aobj->end_frame<=4096);HSD_AObjSetRate(t->aobj,0);
            HSD_ImageDesc* first=NULL;unsigned changed=0;
            for(unsigned frame=0;frame<=(unsigned)t->aobj->end_frame;frame++){
                HSD_AObjReqAnim(t->aobj,frame);HSD_TObjAnim(t);
                if(frame==0)first=t->imagedesc;else changed+=t->imagedesc!=first;
            }
            assert(changed);HSD_AObjReqAnim(t->aobj,0);HSD_TObjAnim(t);HSD_AObjSetRate(t->aobj,1);count++;
        }
        count+=check_expressions(joint->child);
    }
    return count;
}
static size_t inspect(HSD_JObj* j,size_t* drawables,size_t* polygons,size_t* materials) {
    size_t joints=0;
    for(;j;j=j->next){
        joints++;HSD_JObjSetupMatrix(j);
        for(HSD_DObj* d=j->u.dobj;d;d=d->next){
            (*drawables)++;if(d->mobj){(*materials)++;assert(d->mobj->tevdesc);}
            for(HSD_PObj* p=d->pobj;p;p=p->next)(*polygons)++;
        }
        joints+=inspect(j->child,drawables,polygons,materials);
    }
    return joints;
}
typedef struct {
    Quaternion rotate;
    Vec3 scale,translate;
    float blending;
    HSD_ImageDesc* image;
} TextureFrame;
static void texture_frames(HSD_JObj* j,TextureFrame* frames,size_t* count){
    for(;j;j=j->next){
        for(HSD_DObj* d=j->u.dobj;d;d=d->next)
            for(HSD_TObj* t=d->mobj?d->mobj->tobj:NULL;t;t=t->next){
                assert(*count<128);TextureFrame* frame=&frames[(*count)++];
                memset(frame,0,sizeof(*frame));
                frame->rotate=t->rotate;frame->scale=t->scale;frame->translate=t->translate;
                frame->blending=t->blending;frame->image=t->imagedesc;
            }
        texture_frames(j->child,frames,count);
    }
}
static void matrices(HSD_JObj* j,Mtx* out,size_t* count){
    for(;j;j=j->next){HSD_JObjSetupMatrix(j);memcpy(out[(*count)++],j->mtx,sizeof(Mtx));matrices(j->child,out,count);}
}
static void check_finite_joints(HSD_JObj* joint){
    for(;joint;joint=joint->next){
        HSD_JObjSetupMatrix(joint);
        for(unsigned i=0;i<3;i++)for(unsigned j=0;j<4;j++)assert(isfinite(joint->mtx[i][j]));
        check_finite_joints(joint->child);
    }
}
/* Compare the CPU morph coordinate arrays against independent BE reads. */
static unsigned check_morph_data(const MeleeArchive* a,uint32_t root,HSD_Joint* joint){
    if(!joint)return 0;unsigned count=0;uint32_t child,next,d;
    assert(melee_archive_u32(a,root+8,&child)&&melee_archive_u32(a,root+12,&next)&&melee_archive_u32(a,root+16,&d));
    for(HSD_DObjDesc* dobj=joint->u.dobjdesc;dobj;dobj=dobj->next){
        uint32_t p;assert(melee_archive_u32(a,d+12,&p));
        for(HSD_PObjDesc* poly=dobj->pobjdesc;poly;poly=poly->next){
            if((poly->flags&0x3000)==POBJ_SHAPEANIM){
                uint32_t shape;assert(melee_archive_u32(a,p+20,&shape));HSD_ShapeSetDesc* set=poly->u.shape_set;count++;
                for(unsigned field=0;field<2;field++){
                    uint32_t desc,table,raw,layout,type;
                    assert(melee_archive_u32(a,shape+8+12*field,&desc)&&melee_archive_u32(a,shape+12+12*field,&table));
                    HSD_VtxDescList* native=field?set->normal_desc:set->vertex_desc;
                    unsigned n=field?set->nb_normal_index:set->nb_vertex_index;if(!n)continue;
                    assert(native&&native->comp_type==GX_F32&&native->stride==12);
                    assert(melee_archive_u32(a,desc+20,&raw)&&melee_archive_u32(a,desc+16,&layout)&&melee_archive_u32(a,desc+12,&type));
                    unsigned width=type==GX_F32?4:(type==GX_S16||type==GX_U16)?2:1;
                    for(unsigned sh=0;sh<set->nb_shape+!!(set->flags&SHAPESET_ADDITIVE);sh++){
                        uint32_t index;assert(melee_archive_u32(a,table+4*sh,&index));
                        for(unsigned k=0;k<n;k++){
                            const u8* q=a->bytes+32+index+(native->attr_type==GX_INDEX16?2*k:k);
                            unsigned ix=native->attr_type==GX_INDEX16?((unsigned)q[0]<<8|q[1]):q[0];
                            for(unsigned c=0;c<3;c++){
                                uint32_t at=raw+ix*(layout&65535)+c*width;float expected;
                                if(type==GX_F32)assert(melee_archive_f32(a,at,&expected));
                                else {const u8* v=a->bytes+32+at;int x=width==2?((unsigned)v[0]<<8|v[1]):v[0];
                                    if(type==GX_S16)x=(s16)x;else if(type==GX_S8)x=(s8)x;
                                    expected=ldexpf(x,-(int)(layout>>24));}
                                assert(((float*)native->vertex)[ix*3+c]==expected);
                            }
                        }
                    }
                }
            }
            assert(melee_archive_u32(a,p+4,&p));
        }
        assert(melee_archive_u32(a,d+4,&d));
    }
    return count+check_morph_data(a,child,joint->child)+check_morph_data(a,next,joint->next);
}
static unsigned check_joint_copies(HSD_JObj* j){
    unsigned count=0;
    for(;j;j=j->next){
        for(HSD_RObj* r=j->robj;r;r=r->next){
            assert(r->u.exp.is_bytecode&&r->u.exp.rvalue&&!r->u.exp.rvalue->next);
            HSD_Rvalue* v=r->u.exp.rvalue;assert(v->jobj);
            HSD_JObjSetRotationX(v->jobj,0.125f);HSD_JObjSetRotationY(v->jobj,0.25f);HSD_JObjSetRotationZ(v->jobj,0.375f);
            HSD_JObjSetMtxDirtySub(j);HSD_JObjSetupMatrix(j);
            float expected=v->flags==1?0.125f:v->flags==2?0.25f:0.375f;
            unsigned channel=(r->flags&0x0fffffff)-1;assert(channel<3);
            assert(fabsf((&j->rotate.x)[channel]-expected)<0.00001f);count++;
        }
        count+=check_joint_copies(j->child);
    }
    return count;
}
extern int HSD_TObjNativeIndexTest(void);
int main(int argc,char** argv) {
    if(argc==2&&!strcmp(argv[1],"--texture-index"))return HSD_TObjNativeIndexTest();
    if(argc==2&&!strcmp(argv[1],"--particle-tev")){
        HSD_Particle marker={0},particle={0};particle.next=&marker;
        const u32 modes[]={0x80,0x80000080,0x00100080,0x80100080};
        for(unsigned i=0;i<sizeof(modes)/sizeof(*modes);i++){
            particle.kind=modes[i];psSetupTevInvalidState();psSetupTev(&particle);
            assert(particle.next==&marker);assert(particle.kind==(modes[i]&~0x80u));
        }
        puts("Particle TEV reads native kind and preserves linked-list pointer");return 0;
    }

    const char* fighterData=NULL;const char* fighterSymbol=NULL;
    if(argc>=7&&!strcmp(argv[argc-3],"--fighter")){fighterData=argv[argc-2];fighterSymbol=argv[argc-1];argc-=3;}

    static _Alignas(32) uint8_t arena[8*1024*1024];
    assert(OSInitAlloc(arena,arena+sizeof(arena),1));
    HSD_SetHeap(OSCreateHeap(arena,arena+sizeof(arena)));
    HSD_AObjInitAllocData(); HSD_FObjInitAllocData(); HSD_VecInitAllocData();
    HSD_RObjInitAllocData(); HSD_MtxInitAllocData(); HSD_ListInitAllocData(); HSD_IDInitAllocData(); HSD_IDSetup();
    if((argc==4||argc==5)&&!strcmp(argv[1],"--costume")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));
        long size=ftell(file);assert(size>32);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==(size_t)size);fclose(file);
        MeleeArchive archive;u32 root;
        assert(melee_archive_open(&archive,bytes,size)&&melee_archive_find(&archive,argv[3],&root));
        MeleeScene* owner=melee_scene_decode(&archive,root);assert(owner);
        if(argc==5){assert(melee_archive_find(&archive,argv[4],&root));assert(melee_scene_bind_materials(owner,root));}
        melee_scene_release_objects(owner);
        memset(bytes,0xa5,size);free(bytes);
        /* Match the runtime's cached-descriptor load, including repeated loads
         * after the disc archive and the decoder's initial objects are gone. */
        for(unsigned pass=0;pass<2;pass++){
            HSD_JObj* joint=HSD_JObjLoadJoint(melee_scene_joint_descriptor(owner));assert(joint);
            HSD_JObjAddAnimAll(joint,NULL,melee_scene_material_descriptor(owner),NULL);
            for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
            size_t drawables=0,polygons=0,materials=0;
            assert(inspect(joint,&drawables,&polygons,&materials)==melee_scene_joint_count(owner));
            assert(drawables&&polygons);
            HSD_JObjRemoveAll(joint);
        }
        melee_scene_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        printf("Owned retail costume passed: %s\n",argv[2]);return 0;
    }
    if(argc==2&&!strcmp(argv[1],"--fighter-light")){
        for(unsigned i=0;i<20;i++){Fighter fp={0};ftCo_8009F578(&fp);assert(fp.x588);Vec3 p;assert(HSD_LObjGetPosition(fp.x588,&p));assert(p.x==0.57f&&p.y==0.57f&&p.z==0.57f);HSD_LObjRemoveAll(fp.x588);}
        assert(!HSD_IDGetAllocData()->used);puts("Original fighter light: 20 loads, native world position and cleanup passed");return 0;
    }
    if(((argc==6||argc==7)&&(!strcmp(argv[1],"--results-motions")||!strcmp(argv[1],"--intro-motions")))||(argc==9&&!strcmp(argv[1],"--cutscene-motions"))){
        bool intro=!strcmp(argv[1],"--intro-motions"),cutscene=!strcmp(argv[1],"--cutscene-motions");unsigned first=cutscene?atoi(argv[7]):intro?10:0,loaded=cutscene?atoi(argv[8]):intro?2:10;
        assert(loaded>0&&loaded<=10);
        unsigned count=argc>=7?atoi(argv[6]):14;assert(count>=14&&count<=32);
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* f=fopen(argv[2+i],"rb");assert(f&&!fseek(f,0,SEEK_END));sizes[i]=ftell(f);rewind(f);buffers[i]=malloc(sizes[i]);assert(buffers[i]&&fread(buffers[i],1,sizes[i],f)==sizes[i]);fclose(f);}
        MeleeArchive data,motion;u32 root,start;
        assert(melee_archive_open(&data,buffers[0],sizes[0])&&melee_archive_open(&motion,buffers[1],sizes[1]));
        assert(melee_archive_find(&data,argv[4],&root)&&melee_archive_find(&motion,argv[5],&start));
        u32 table;MeleeHostBool table_present,script_present[10];unsigned script_count=0;
        assert(melee_archive_pointer(&data,root+20,&table,&table_present)&&table_present);
        for(unsigned i=0;i<loaded;i++){u32 script;assert(melee_archive_pointer(&data,table+24*(first+i)+12,&script,&script_present[i]));script_count+=script_present[i];}
        const void* stream=motion.bytes+32+start;size_t length=motion.data_size-start;
        HSD_Archive* bridge=NULL;
        if(cutscene){bridge=melee_cutscene_decode(&motion);assert(bridge);stream=melee_intro_motion_bytes(bridge,argv[5],&length);assert(stream&&length);}
        assert(!melee_character_motions_decode_range(&data,root+20,count,count-4,5,stream,length));
        assert(!melee_character_motions_decode_range(&data,root+20,count,0,count,stream,length));
        assert(!melee_character_motions_decode_range(&data,root+20,count,first,loaded,stream,1));
        MeleeCharacterMotions* owner=melee_character_motions_decode_range(&data,root+20,count,first,loaded,stream,length);assert(owner);
        if(bridge)bridge->native_destroy(bridge);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        struct Fighter_WaitAnimData* records=melee_character_motions_records(owner);
        for(unsigned i=0;i<count;i++){
            assert(records[i].native_index==i);
            if(i>=first&&i<first+loaded){assert(records[i].native_owner==owner&&!!records[i].xC==!!script_present[i-first]);if(records[i].x8){assert(records[i].x0&&records[i].x14);assert(melee_character_motions_tree(owner,i));}else assert(!records[i].x14&&!melee_character_motions_tree(owner,i));}
            else assert(!records[i].native_owner&&!records[i].x0&&!records[i].xC&&!records[i].x14&&!melee_character_motions_tree(owner,i));
        }
        melee_character_motions_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("%s motions: %u owned records, %u scripts, preserved null slots/indices, invalid ranges and source disposal passed\n",cutscene?"Cutscene":intro?"Intro":"Results",loaded,script_count);return 0;
    }
    if(argc==5&&!strcmp(argv[1],"--fox-data")){
        bool falco=!strcmp(argv[4],"falco");assert(falco||!strcmp(argv[4],"fox"));u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));MeleeFoxData* owner=melee_fox_data_decode(&a,buffers[1],sizes[1],falco);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_fox_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<327;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_fox_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Fox/Falco normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--donkey-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));assert(!strcmp(name,"ftDataDonkey"));MeleeDonkeyData* owner=melee_donkey_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_donkey_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&!d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<337;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_donkey_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Donkey Kong normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--purin-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));assert(!strcmp(name,"ftDataPurin"));MeleePurinData* owner=melee_purin_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        u32 dynamics_at,table_at,params_at,expected_params[5][75];MeleeHostBool present;
        assert(melee_archive_pointer(&a,root+0x2c,&dynamics_at,&present)&&present);
        assert(melee_archive_pointer(&a,dynamics_at+4,&table_at,&present)&&present);
        const unsigned raw_counts[]={3,3,3,5,5};
        for(unsigned i=0;i<5;i++){
            assert(melee_archive_pointer(&a,table_at+24*i+4,&params_at,&present)&&present);
            for(unsigned k=0;k<raw_counts[i]*15;k++)assert(melee_archive_u32(&a,params_at+4*k,&expected_params[i][k]));
        }
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_purin_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&!d->x48_items[0]&&d->x48_items[1]&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<327;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(d->x2C->dynamicsNum==1);
        const unsigned hat_bones[]={7,3,7,3,9},hat_counts[]={3,3,3,5,5};
        for(unsigned i=0;i<5;i++){
            BoneDynamicsDesc* b=&d->x2C->ftDynamicBones->array[i];
            assert(b->bone_id==hat_bones[i]&&b->dyn_desc.count==hat_counts[i]&&b->dyn_desc.params);
            assert(!memcmp(b->dyn_desc.params,expected_params[i],hat_counts[i]*60));
            for(unsigned k=0;k<b->dyn_desc.count;k++)for(unsigned j=0;j<15;j++){float f;memcpy(&f,(u8*)&b->dyn_desc.params[k]+4*j,4);assert(isfinite(f));}
        }
        struct KirbyHatStruct* hat=d->x48_items[1];assert(!hat->hat_joint&&hat->desc.model_num==1&&hat->desc.vis_table);
        unsigned choices=0;for(unsigned c=0;c<5;c++)for(unsigned t=0;t<4;t++){
            FtPartsVisLookup* group=hat->desc.vis_table[c][t];if(!group)continue;
            for(unsigned g=0;g<hat->desc.model_num;g++)for(int k=0;k<group[g].x0;k++){
                TempS* choice=group[g].x4+k;assert(choice->x0>=0&&choice->x4);choices++;
                for(int j=0;j<choice->x0;j++)assert(choice->x4[j]<124);
            }
        }
        assert(choices>0&&animations>0&&scripts>0);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_purin_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Jigglypuff normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--mars-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));bool roy=!strcmp(name,"ftDataEmblem");assert(roy||!strcmp(name,"ftDataMars"));MeleeMarsData* owner=melee_mars_data_decode(&a,buffers[1],sizes[1],roy);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_mars_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&!d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<327;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_mars_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Marth/Roy normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--wireframe-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));bool girl=!strcmp(name,"ftDataGirl");assert(girl||!strcmp(name,"ftDataBoy"));MeleeWireframeData* owner=melee_wireframe_data_decode(&a,buffers[1],sizes[1],girl);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_wireframe_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&!d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<295;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(d->x1C[0]&&!d->x1C[1]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_wireframe_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Wireframe normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }    if(argc==4&&!strcmp(argv[1],"--sandbag-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));assert(!strcmp(name,"ftDataSandbag"));MeleeSandbagData* owner=melee_sandbag_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_sandbag_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&!d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&!d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<296;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(!d->x1C[0]);melee_sandbag_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Sandbag normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--captain-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));bool ganon=!strcmp(name,"ftDataGanon");assert(ganon||!strcmp(name,"ftDataCaptain"));MeleeCaptainData* owner=melee_captain_data_decode(&a,buffers[1],sizes[1],ganon);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_captain_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&!d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<318;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        if(ganon){
            for(unsigned group=3;group<5;group++){
                struct ftData_x1C* part=d->x1C[group];assert(part&&part->x0==54&&part->x2==3&&part->x4);
                for(unsigned j=0;j<6;j++){
                    HSD_AnimJoint* stack[256];unsigned n=1,nodes=0;stack[0]=part->x8[j];assert(stack[0]);
                    while(n){HSD_AnimJoint* a=stack[--n];nodes++;assert(n<254);if(a->child)stack[n++]=a->child;if(a->next)stack[n++]=a->next;}
                    assert(nodes>=3);
                }
            }
        }else assert(!d->x1C[3]&&!d->x1C[4]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_captain_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Captain Falcon/Ganondorf normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--sheik-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));assert(!strcmp(name,"ftDataSeak"));MeleeSheikData* owner=melee_sheik_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_sheik_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<317;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations==264&&scripts==317&&d->x48_items[0]&&d->x48_items[1]&&d->x48_items[2]&&d->x48_items[3]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_sheik_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Sheik normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--iceclimbers-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));int nana=!strcmp(name,"ftDataNana");assert(nana||!strcmp(name,"ftDataPopo"));MeleeIceClimbersData* owner=melee_iceclimbers_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_iceclimbers_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<321;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations==(nana?8u:260u)&&scripts==321);assert(nana?!d->x48_items:(d->x48_items&&d->x48_items[0]&&d->x48_items[1]&&d->x48_items[2]));
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_iceclimbers_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("IceClimbers normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==9&&(!strcmp(argv[1],"--kirby-falco-costumes")||!strcmp(argv[1],"--kirby-donkey-costumes")||!strcmp(argv[1],"--kirby-mewtwo-costumes")||!strcmp(argv[1],"--kirby-purin-costumes"))){
        bool donkey=!strcmp(argv[1],"--kirby-donkey-costumes");
        bool mewtwo=!strcmp(argv[1],"--kirby-mewtwo-costumes");
        bool purin=!strcmp(argv[1],"--kirby-purin-costumes");
        u32 expected[12];
        MeleeKirbyCompositeCopy*o=NULL;
        for(unsigned i=0;i<7;i++){
            FILE*f=fopen(argv[i+2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
            MeleeArchive a;assert(melee_archive_open(&a,b,n));
            if(!i){
                o=melee_kirby_composite_copy_decode(&a, purin?Ft_Kind_Purin:mewtwo?Ft_Kind_Mewtwo:donkey?Ft_Kind_Donkey:Ft_Kind_Falco);assert(o);
                assert(!melee_kirby_composite_copy_decode(&a, donkey?Ft_Kind_Falco:Ft_Kind_Donkey));
                MeleeKirbyCompositeCopyDesc*d=melee_kirby_composite_copy_descriptor(o);
                assert(d->replacement_mask==(purin?0xfu:mewtwo?0x7f0u:donkey?0u:0x1800u));
                assert(d->parts.model_num==1&&d->parts.vis_table&&d->textures.x8==2);
                assert(mewtwo?(d->laser&&!d->blaster):(donkey||purin)?(!d->laser&&!d->blaster):(d->laser&&d->blaster));
                if(mewtwo){u32 root,article,attrs;MeleeHostBool present;
                    assert(melee_archive_find(&a,"ftDataKirbyCopyMewtwo",&root)&&melee_archive_pointer(&a,root+24,&article,&present)&&present&&melee_archive_pointer(&a,article+4,&attrs,&present)&&present);
                    for(unsigned j=0;j<12;j++)assert(melee_archive_u32(&a,attrs+4*j,&expected[j]));
                }
            }
            else{
                assert(melee_kirby_composite_copy_bind_costume(o,&a,i-1));MeleeKirbyCompositeCopyDesc*d=melee_kirby_composite_copy_descriptor(o);HSD_Joint*saved=d->costume_joints[i-1];
                assert(!melee_kirby_composite_copy_bind_costume(o,&a,6));assert(!melee_kirby_composite_copy_bind_costume(o,&a,i%6));assert(d->costume_joints[i-1]==saved);
            }
            memset(b,0xa5,n);free(b);
        }
        MeleeKirbyCompositeCopyDesc*d=melee_kirby_composite_copy_descriptor(o);
        HSD_JObj*shared=HSD_JObjLoadJoint(d->shared_joint);assert(shared);HSD_JObjRemoveAll(shared);
        for(unsigned i=0;i<6;i++){
            assert(d->costume_joints[i]&&d->costume_materials[i]);HSD_JObj*g=HSD_JObjLoadJoint(d->costume_joints[i]);assert(g);
            HSD_JObjAddAnimAll(g,NULL,d->costume_materials[i],NULL);
            for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(g,frame);HSD_JObjAnimAll(g);}HSD_JObjRemoveAll(g);
        }
        if(purin)assert(d->dynamics&&d->dynamics->dynamicsNum==1&&d->dynamics->ftDynamicBones);
        if(mewtwo){
            assert(d->dynamics&&d->dynamics->dynamicsNum==1&&d->dynamics->ftDynamicBones);
            Article*item=d->laser;assert(item&&item->x4_specialAttributes&&item->xC_itemStates&&item->x10_modelDesc);
            for(unsigned j=0;j<12;j++){u32 actual;memcpy(&actual,(u8*)item->x4_specialAttributes+4*j,4);assert(actual==expected[j]);}
            for(unsigned j=0;j<10;j++){
                ItemStateDesc*state=&item->xC_itemStates->x0_itemStateDesc[j];HSD_JObj*g=HSD_JObjLoadJoint(item->x10_modelDesc->x0_joint);assert(g);
                HSD_JObjAddAnimAll(g,state->x0_anim_joint,state->x4_matanim_joint,state->x8_parameters);
                for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(g,frame);HSD_JObjAnimAll(g);}HSD_JObjRemoveAll(g);
            }
        }
        melee_kirby_composite_copy_free(o);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("%s copy: shared model, six costume models/material animations, invalid-slot preservation, source disposal and cleanup passed\n",purin?"Jigglypuff":mewtwo?"Mewtwo":donkey?"Donkey":"Falco");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kirby-falco-copy")){
        FILE*f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCompositeCopy*o=melee_kirby_composite_copy_decode(&a, Ft_Kind_Falco);assert(o);
        u32 root;assert(melee_archive_find(&a,"ftDataKirbyCopyFalco",&root));u32 expected[2][10];
        for(unsigned i=0;i<2;i++){u32 article,attrs;MeleeHostBool present;assert(melee_archive_pointer(&a,root+24+4*i,&article,&present)&&present);assert(melee_archive_pointer(&a,article+4,&attrs,&present)&&present);for(unsigned j=0;j<10;j++)assert(melee_archive_u32(&a,attrs+4*j,&expected[i][j]));}
        memset(b,0xa5,n);free(b);MeleeKirbyCompositeCopyDesc*d=melee_kirby_composite_copy_descriptor(o);
        assert(d&&d->replacement_mask==0x1800&&d->parts.model_num==1&&d->parts.vis_table&&d->textures.x8==2);
        assert(d->textures.xC[0]&&d->textures.xC[0][0]==3&&d->textures.xC[0][1]==1);for(unsigned c=1;c<6;c++)assert(!d->textures.xC[c]);
        HSD_JObj*g=HSD_JObjLoadJoint(d->shared_joint);assert(g);HSD_JObjRemoveAll(g);
        Article*articles[]={d->laser,d->blaster};const unsigned counts[]={2,9};unsigned models=0,scripts=0;
        for(unsigned i=0;i<2;i++)for(unsigned j=0;j<counts[i];j++){
            Article*item=articles[i];assert(item&&item->x4_specialAttributes&&item->xC_itemStates&&item->x10_modelDesc);
            for(unsigned word=0;word<10;word++){u32 actual;memcpy(&actual,(u8*)item->x4_specialAttributes+4*word,4);assert(actual==expected[i][word]);}
            ItemStateDesc*anim=&item->xC_itemStates->x0_itemStateDesc[j];scripts+=anim->xC_script!=NULL;
            if(!item->x10_modelDesc->x0_joint)continue;g=HSD_JObjLoadJoint(item->x10_modelDesc->x0_joint);assert(g);models++;
            HSD_JObjAddAnimAll(g,anim->x0_anim_joint,anim->x4_matanim_joint,anim->x8_parameters);
            for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(g,frame);HSD_JObjAnimAll(g);}HSD_JObjRemoveAll(g);
        }
        melee_kirby_composite_copy_free(o);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Falco composite copy: visibility, textures, mask, shared joint, %u article animations, %u scripts and source disposal passed\n",models,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--kirby-copy-links")){
        const FighterKind kinds[]={Ft_Kind_Link,Ft_Kind_CLink};
        const char*symbols[]={"ftDataKirbyCopyLink","ftDataKirbyCopyClink"};
        const unsigned words[]={9,2},counts[]={1,6};
        for(unsigned k=0;k<2;k++){
            FILE*f=fopen(argv[2+k],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
            MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,kinds[k]);assert(o);
            assert(!melee_kirby_copy_decode(&a,kinds[1-k]));u32 root,expected[2][9];assert(melee_archive_find(&a,symbols[k],&root));
            for(unsigned i=0;i<2;i++){u32 article,attrs;MeleeHostBool present;
                assert(melee_archive_pointer(&a,root+12+4*i,&article,&present)&&present&&melee_archive_pointer(&a,article+4,&attrs,&present)&&present);
                for(unsigned j=0;j<words[i];j++)assert(melee_archive_u32(&a,attrs+4*j,&expected[i][j]));
            }
            memset(b,0xa5,n);free(b);KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->desc.vis_table&&h->hat_dynamics[2]&&h->hat_dynamics[2]->dynamicsNum==1&&h->hat_dynamics[2]->ftDynamicBones);
            HSD_JObj*hat=HSD_JObjLoadJoint(h->hat_joint);assert(hat);HSD_JObjRemoveAll(hat);
            for(unsigned i=0;i<2;i++){
                Article*d=(Article*)h->hat_dynamics[i];assert(d&&d->x4_specialAttributes&&d->xC_itemStates&&d->x10_modelDesc);
                if(i==0){itLinkArrowAttributes*ar=d->x4_specialAttributes;assert(ar->x24&&ar->x28);
                    HSD_JObj*first=HSD_JObjLoadJoint(ar->x24),*second=HSD_JObjLoadJoint(ar->x28);assert(first&&second);HSD_JObjRemoveAll(first);HSD_JObjRemoveAll(second);}
                for(unsigned j=0;j<words[i];j++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*j,4);assert(actual==expected[i][j]);}
                for(unsigned j=0;j<counts[i];j++){
                    ItemStateDesc*state=&d->xC_itemStates->x0_itemStateDesc[j];
                    HSD_JObj*shot=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(shot);
                    HSD_JObjAddAnimAll(shot,state->x0_anim_joint,state->x4_matanim_joint,state->x8_parameters);
                    for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(shot,frame);HSD_JObjAnimAll(shot);}HSD_JObjRemoveAll(shot);
                }
            }
            melee_kirby_copy_free(o);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        }
        puts("Link/Young Link copy: hats, dynamics, bows/arrows and fourteen animations, source disposal and cleanup passed");return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--kirby-copy-electric")){
        const FighterKind kinds[]={Ft_Kind_Pikachu,Ft_Kind_Pichu};
        const char*symbols[]={"ftDataKirbyCopyPikachu","ftDataKirbyCopyPichu"};
        const unsigned words[]={4,1},counts[]={2,1};
        for(unsigned k=0;k<2;k++){
            FILE*f=fopen(argv[2+k],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
            MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,kinds[k]);assert(o);
            assert(!melee_kirby_copy_decode(&a,kinds[1-k]));u32 root,expected[2][4];assert(melee_archive_find(&a,symbols[k],&root));
            for(unsigned i=0;i<2;i++){u32 article,attrs;MeleeHostBool present;
                assert(melee_archive_pointer(&a,root+12+4*i,&article,&present)&&present&&melee_archive_pointer(&a,article+4,&attrs,&present)&&present);
                for(unsigned j=0;j<words[i];j++)assert(melee_archive_u32(&a,attrs+4*j,&expected[i][j]));
            }
            memset(b,0xa5,n);free(b);KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->desc.vis_table&&h->hat_dynamics[2]&&h->hat_dynamics[2]->dynamicsNum==3&&h->hat_dynamics[2]->ftDynamicBones);
            HSD_JObj*hat=HSD_JObjLoadJoint(h->hat_joint);assert(hat);HSD_JObjRemoveAll(hat);
            for(unsigned i=0;i<2;i++){
                Article*d=(Article*)h->hat_dynamics[i];assert(d&&d->x4_specialAttributes&&d->xC_itemStates&&d->x10_modelDesc);
                for(unsigned j=0;j<words[i];j++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*j,4);assert(actual==expected[i][j]);}
                for(unsigned j=0;j<counts[i];j++){
                    ItemStateDesc*state=&d->xC_itemStates->x0_itemStateDesc[j];
                    if(i==0){assert(!d->x10_modelDesc->x0_joint&&!state->x0_anim_joint&&!state->x4_matanim_joint&&!state->x8_parameters);continue;}
                    HSD_JObj*shot=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(shot);
                    HSD_JObjAddAnimAll(shot,state->x0_anim_joint,state->x4_matanim_joint,state->x8_parameters);
                    for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(shot,frame);HSD_JObjAnimAll(shot);}HSD_JObjRemoveAll(shot);
                }
            }
            melee_kirby_copy_free(o);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        }
        puts("Pikachu/Pichu copy: hats, both Thunder Jolt articles, two air animations and model-free ground states, source disposal and cleanup passed");return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--kirby-gamewatch-costumes")){
        MeleeKirbyCompositeCopy*o=NULL;
        for(unsigned i=0;i<2;i++){
            FILE*f=fopen(argv[2+i],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
            MeleeArchive a;assert(melee_archive_open(&a,b,n));
            if(!i){o=melee_kirby_composite_copy_decode(&a,Ft_Kind_GameWatch);assert(o);assert(!melee_kirby_composite_copy_decode(&a,Ft_Kind_Mewtwo));assert(!melee_kirby_composite_copy_bind_costume(o,&a,0));}
            else{for(unsigned c=0;c<6;c++)assert(melee_kirby_composite_copy_bind_costume(o,&a,c));assert(!melee_kirby_composite_copy_bind_costume(o,&a,6));}
            memset(b,0xa5,n);free(b);
        }
        MeleeKirbyCompositeCopyDesc*d=melee_kirby_composite_copy_descriptor(o);
        assert(d&&!d->shared_joint&&!d->replacement_mask&&!d->dynamics&&d->laser&&d->blaster);
        assert(d->parts.vis_table&&d->textures.x8==2&&d->outline[0].x0==1&&d->outline[0].x4[0].x0==7);
        for(unsigned i=0;i<7;i++)assert(d->outline[0].x4[0].x4[i]==3+i);
        for(unsigned i=1;i<11;i++)assert(!d->outline[i].x0&&!d->outline[i].x4);
        const u8 fill[]={0,0,0,255},outline[]={255,255,255,128};
        assert(d->model_depth==0.01f&&!memcmp(d->fill_rgba,fill,4)&&!memcmp(d->outline_rgba,outline,4));
        for(unsigned c=0;c<6;c++){
            HSD_JObj*g=HSD_JObjLoadJoint(d->costume_joints[c]);assert(g&&d->costume_materials[c]);
            HSD_JObjAddAnimAll(g,NULL,d->costume_materials[c],NULL);
            for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(g,frame);HSD_JObjAnimAll(g);}HSD_JObjRemoveAll(g);
        }
        melee_kirby_composite_copy_free(o);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Game & Watch copy: six costume slots, outline indices, offset-zero depth/colors, Chef articles and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kirby-chef-articles")){
        FILE*f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,b,n));u32 root;assert(melee_archive_find(&a,"ftDataKirbyCopyGamewatch",&root));
        MeleeItemArticle*owners[2];const unsigned kinds[]={It_Kind_Kirby_GameWatchChef,It_Kind_Kirby_GameWatchChefPan},counts[]={2,0};float expected[28];
        for(unsigned i=0;i<2;i++){u32 at,attrs;MeleeHostBool present;assert(melee_archive_pointer(&a,root+32+4*i,&at,&present)&&present);
            owners[i]=melee_item_article_decode(&a,kinds[i],at,counts[i]);assert(owners[i]);
            if(!i){assert(melee_archive_pointer(&a,at+4,&attrs,&present)&&present);for(unsigned j=0;j<28;j++)assert(melee_archive_f32(&a,attrs+4+4*j,&expected[j]));}
        }
        memset(b,0xa5,n);free(b);
        for(unsigned i=0;i<2;i++){
            Article*d=melee_item_article_descriptor(owners[i]);assert(d&&d->x4_specialAttributes&&d->x10_modelDesc);
            if(!i){itGamewatchchefAttributes*attrs=d->x4_specialAttributes;assert(attrs->x0);for(unsigned j=0;j<28;j++){float actual;memcpy(&actual,(u8*)&attrs->x4+4*j,4);assert(actual==expected[j]);}}
            for(unsigned j=0;j<(counts[i]?counts[i]:1);j++){
                HSD_JObj*g=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(g);
                if(counts[i]){ItemStateDesc*state=&d->xC_itemStates->x0_itemStateDesc[j];HSD_JObjAddAnimAll(g,state->x0_anim_joint,state->x4_matanim_joint,state->x8_parameters);for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(g,frame);HSD_JObjAnimAll(g);}}
                HSD_JObjRemoveAll(g);
            }
            melee_item_article_free(owners[i]);
        }
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Kirby Chef: food/pan models, outline descriptors, 28 parameters, two animation states and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kirby-copy-yoshi")){
        FILE*f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,Ft_Kind_Yoshi);assert(o);
        assert(!melee_kirby_copy_decode(&a,Ft_Kind_Koopa));memset(b,0xa5,n);free(b);
        KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->hat_joint&&h->desc.vis_table&&h->hat_dynamics[0]&&!h->hat_dynamics[6]);
        for(unsigned i=1;i<=4;i++){
            assert(h->hat_dynamics[i]);HSD_JObj*g=HSD_JObjLoadJoint(i%2?(HSD_Joint*)h->hat_dynamics[0]:h->hat_joint);assert(g);
            HSD_JObjAddAnimAll(g,(HSD_AnimJoint*)h->hat_dynamics[i],NULL,NULL);
            for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(g,frame);HSD_JObjAnimAll(g);}HSD_JObjRemoveAll(g);
        }
        Article*egg=(Article*)h->hat_dynamics[5];assert(egg&&!egg->x4_specialAttributes&&egg->x10_modelDesc);
        HSD_JObj*g=HSD_JObjLoadJoint(egg->x10_modelDesc->x0_joint);assert(g);HSD_JObjRemoveAll(g);
        melee_kirby_copy_free(o);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Yoshi copy: hat/capture models, four animation trees, egg article, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kirby-copy-koopa")){
        FILE*f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,Ft_Kind_Koopa);assert(o);
        assert(!melee_kirby_copy_decode(&a,Ft_Kind_Samus));u32 root,article,attrs,expected[6];MeleeHostBool present;
        assert(melee_archive_find(&a,"ftDataKirbyCopyKoopa",&root)&&melee_archive_pointer(&a,root+12,&article,&present)&&present&&melee_archive_pointer(&a,article+4,&attrs,&present)&&present);
        for(unsigned i=0;i<6;i++)assert(melee_archive_u32(&a,attrs+4*i,&expected[i]));
        memset(b,0xa5,n);free(b);KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->desc.vis_table&&!h->hat_dynamics[2]);
        assert(h->hat_dynamics[1]&&h->hat_dynamics[1]->dynamicsNum==1&&h->hat_dynamics[1]->ftDynamicBones);
        HSD_JObj*hat=HSD_JObjLoadJoint(h->hat_joint);assert(hat);HSD_JObjRemoveAll(hat);
        Article*d=(Article*)h->hat_dynamics[0];assert(d&&d->x4_specialAttributes&&d->xC_itemStates&&d->x10_modelDesc&&!d->x10_modelDesc->x0_joint);
        for(unsigned i=0;i<6;i++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*i,4);assert(actual==expected[i]);}
        ItemStateDesc*state=&d->xC_itemStates->x0_itemStateDesc[0];
        assert(!state->x0_anim_joint&&!state->x4_matanim_joint&&!state->x8_parameters&&state->xC_script);
        melee_kirby_copy_free(o);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Bowser copy: hat, dynamics, model-free flame parameters/script, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kirby-copy-samus")){
        FILE*f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,Ft_Kind_Samus);assert(o);
        assert(!melee_kirby_copy_decode(&a,Ft_Kind_Fox));u32 root,article,attrs,expected[8];MeleeHostBool present;
        assert(melee_archive_find(&a,"ftDataKirbyCopySamus",&root)&&melee_archive_pointer(&a,root+12,&article,&present)&&present&&melee_archive_pointer(&a,article+4,&attrs,&present)&&present);
        for(unsigned i=0;i<8;i++)assert(melee_archive_u32(&a,attrs+4*i,&expected[i]));
        memset(b,0xa5,n);free(b);KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->desc.vis_table&&!h->hat_dynamics[1]);
        HSD_JObj*hat=HSD_JObjLoadJoint(h->hat_joint);assert(hat);HSD_JObjRemoveAll(hat);
        Article*d=(Article*)h->hat_dynamics[0];assert(d&&d->x4_specialAttributes&&d->xC_itemStates&&d->x10_modelDesc);
        for(unsigned i=0;i<8;i++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*i,4);assert(actual==expected[i]);}
        unsigned scripts=0;for(unsigned i=0;i<9;i++){
            ItemStateDesc*state=&d->xC_itemStates->x0_itemStateDesc[i];scripts+=state->xC_script!=NULL;
            HSD_JObj*shot=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(shot);
            HSD_JObjAddAnimAll(shot,state->x0_anim_joint,state->x4_matanim_joint,state->x8_parameters);
            for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(shot,frame);HSD_JObjAnimAll(shot);}HSD_JObjRemoveAll(shot);
        }
        melee_kirby_copy_free(o);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Samus copy: helmet, Charge Shot parameters, nine animations, %u scripts, source disposal and cleanup passed\n",scripts);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kirby-copy-ice")){
        FILE*f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,Ft_Kind_Popo);assert(o);
        assert(!melee_kirby_copy_decode(&a,Ft_Kind_Peach));u32 root,article,attrs,expected[13];MeleeHostBool present;
        assert(melee_archive_find(&a,"ftDataKirbyCopyPopo",&root)&&melee_archive_pointer(&a,root+12,&article,&present)&&present&&melee_archive_pointer(&a,article+4,&attrs,&present)&&present);
        for(unsigned i=0;i<13;i++){
            if(i==8)memcpy(&expected[i],a.bytes+32+attrs+4*i,4); /* pad_20 bytes stay byte-ordered. */
            else assert(melee_archive_u32(&a,attrs+4*i,&expected[i]));
        }
        memset(b,0xa5,n);free(b);KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->desc.vis_table&&!h->hat_dynamics[2]);
        HSD_JObj*hat=HSD_JObjLoadJoint(h->hat_joint),*hammer=HSD_JObjLoadJoint((HSD_Joint*)h->hat_dynamics[1]);assert(hat&&hammer);
        Article*d=(Article*)h->hat_dynamics[0];assert(d&&d->x4_specialAttributes&&d->xC_itemStates&&d->x10_modelDesc);
        for(unsigned i=0;i<13;i++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*i,4);assert(actual==expected[i]);}
        ItemStateDesc*state=&d->xC_itemStates->x0_itemStateDesc[0];HSD_JObj*ice=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(ice);
        HSD_JObjAddAnimAll(ice,state->x0_anim_joint,state->x4_matanim_joint,state->x8_parameters);
        for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(ice,frame);HSD_JObjAnimAll(ice);}
        HSD_JObjRemoveAll(ice);HSD_JObjRemoveAll(hammer);HSD_JObjRemoveAll(hat);melee_kirby_copy_free(o);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Ice Climbers copy: parka, hammer, ice-shot parameters/animation, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kirby-copy-peach")){
        FILE*f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,Ft_Kind_Peach);assert(o);
        assert(!melee_kirby_copy_decode(&a,Ft_Kind_Ness));u32 root;assert(melee_archive_find(&a,"ftDataKirbyCopyPeach",&root));
        u32 expected[2][4];const unsigned words[]={1,4},counts[]={2,1};
        for(unsigned i=0;i<2;i++){u32 article,attrs;MeleeHostBool present;assert(melee_archive_pointer(&a,root+12+4*i,&article,&present)&&present);assert(melee_archive_pointer(&a,article+4,&attrs,&present)&&present);for(unsigned j=0;j<words[i];j++)assert(melee_archive_u32(&a,attrs+4*j,&expected[i][j]));}
        memset(b,0xa5,n);free(b);KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->desc.vis_table&&!h->hat_dynamics[2]);
        HSD_JObj*hat=HSD_JObjLoadJoint(h->hat_joint);assert(hat);HSD_JObjRemoveAll(hat);
        unsigned models=0;for(unsigned i=0;i<2;i++){
            Article*d=(Article*)h->hat_dynamics[i];assert(d&&d->x4_specialAttributes&&d->xC_itemStates&&d->x10_modelDesc);
            for(unsigned j=0;j<words[i];j++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*j,4);assert(actual==expected[i][j]);}
            for(unsigned j=0;j<counts[i];j++){
                ItemStateDesc*a=&d->xC_itemStates->x0_itemStateDesc[j];
                if(i==1){assert(!d->x10_modelDesc->x0_joint&&!a->x0_anim_joint&&!a->x4_matanim_joint&&!a->x8_parameters&&a->xC_script);continue;}
                HSD_JObj*g=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(g);models++;
                HSD_JObjAddAnimAll(g,a->x0_anim_joint,a->x4_matanim_joint,a->x8_parameters);
                for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(g,frame);HSD_JObjAnimAll(g);}HSD_JObjRemoveAll(g);
            }
        }
        melee_kirby_copy_free(o);assert(models==2&&!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Peach copy: crown, Toad/spore parameters, two Toad animations and model-free spore script, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kirby-copy-sheik")){
        FILE*f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,Ft_Kind_Seak);assert(o);
        memset(b,0xa5,n);free(b);KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->desc.vis_table&&h->hat_dynamics[2]->dynamicsNum==2);
        HSD_JObj*hat=HSD_JObjLoadJoint(h->hat_joint);assert(hat);HSD_JObjRemoveAll(hat);
        const unsigned counts[]={5,1};for(unsigned i=0;i<2;i++){
            Article*d=(Article*)h->hat_dynamics[i];assert(d&&d->x4_specialAttributes&&d->xC_itemStates&&d->x10_modelDesc);
            for(unsigned j=0;j<counts[i];j++){
                ItemStateDesc*a=&d->xC_itemStates->x0_itemStateDesc[j];HSD_JObj*g=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(g);
                HSD_JObjAddAnimAll(g,a->x0_anim_joint,a->x4_matanim_joint,a->x8_parameters);
                for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(g,frame);HSD_JObjAnimAll(g);}HSD_JObjRemoveAll(g);
            }
        }
        melee_kirby_copy_free(o);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Sheik copy: headgear, two dynamic chains, six needle animations, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kirby-copy-zelda")){
        FILE*f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,Ft_Kind_Zelda);assert(o);
        assert(!melee_kirby_copy_decode(&a,Ft_Kind_Mars));memset(b,0xa5,n);free(b);
        KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->desc.vis_table&&!h->hat_dynamics[1]);
        HSD_JObj*hat=HSD_JObjLoadJoint(h->hat_joint);assert(hat);
        ftDynamics*d=h->hat_dynamics[0];assert(d&&d->dynamicsNum==3&&!d->x4&&!d->x10);
        const unsigned bones[]={9,15,3};for(unsigned i=0;i<3;i++){
            BoneDynamicsDesc*c=&d->ftDynamicBones->array[i];assert(c->bone_id==bones[i]&&c->dyn_desc.count==4&&c->dyn_desc.params);
        }
        HSD_JObjRemoveAll(hat);melee_kirby_copy_free(o);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Zelda copy: headgear, visibility, three four-link dynamic chains, source disposal and cleanup passed");return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--kirby-copy-swords")){
        for(unsigned k=0;k<2;k++){
            FILE*f=fopen(argv[k+2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
            MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,k?Ft_Kind_Emblem:Ft_Kind_Mars);assert(o);
            assert(!melee_kirby_copy_decode(&a,k?Ft_Kind_Mars:Ft_Kind_Emblem));memset(b,0xa5,n);free(b);
            KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->desc.vis_table);
            HSD_JObj*hat=HSD_JObjLoadJoint(h->hat_joint),*sword=HSD_JObjLoadJoint((HSD_Joint*)h->hat_dynamics[0]);assert(hat&&sword);
            ftDynamics*d=h->hat_dynamics[1];assert(d&&d->dynamicsNum==(k?4:3)&&!d->x4&&!d->x10);
            for(int i=0;i<d->dynamicsNum;i++){
                BoneDynamicsDesc*chain=&d->ftDynamicBones->array[i];assert(chain->dyn_desc.count==2&&chain->dyn_desc.params);
                assert(chain->bone_id==(k?3+i*3:i==0?10:i==1?4:7));
            }
            HSD_JObjRemoveAll(sword);HSD_JObjRemoveAll(hat);melee_kirby_copy_free(o);
            assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        }
        puts("Marth/Roy copy: hats, swords, three/four dynamic chains, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--kirby-copy-captain")||!strcmp(argv[1],"--kirby-copy-ganon"))){
        bool ganon=!strcmp(argv[1],"--kirby-copy-ganon");
        FILE*f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,ganon?Ft_Kind_Ganon:Ft_Kind_Captain);assert(o);
        assert(!melee_kirby_copy_decode(&a,Ft_Kind_Fox));memset(b,0xa5,n);free(b);
        KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->hat_joint&&h->desc.model_num==1&&h->desc.vis_table);
        assert(!h->hat_dynamics[0]&&!h->hat_dynamics[1]);HSD_JObj*g=HSD_JObjLoadJoint(h->hat_joint);assert(g);HSD_JObjRemoveAll(g);
        melee_kirby_copy_free(o);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("%s copy: helmet, visibility, absent articles, wrong-kind rejection and source disposal passed\n",ganon?"Ganon":"Captain");return 0;
    }
    if(argc==5&&!strcmp(argv[1],"--kirby-copy-fireballs")){
        const FighterKind kinds[]={Ft_Kind_Mario,Ft_Kind_DrMario,Ft_Kind_Luigi};const unsigned counts[]={1,6,1},words[]={5,5,4};
        unsigned models=0,scripts=0;
        for(unsigned k=0;k<3;k++){
            FILE*f=fopen(argv[k+2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
            MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,kinds[k]);assert(o);
            const char*name;u32 root,article,attrs;MeleeHostBool present;assert(melee_archive_public(&a,0,&name,&root));
            assert(melee_archive_pointer(&a,root+12,&article,&present)&&present);assert(melee_archive_pointer(&a,article+4,&attrs,&present)&&present);u32 expected[5];
            for(unsigned j=0;j<words[k];j++)assert(melee_archive_u32(&a,attrs+4*j,&expected[j]));
            assert(!melee_kirby_copy_decode(&a,Ft_Kind_Kirby));assert(!melee_kirby_copy_decode(&a,kinds[(k+1)%3]));
            memset(b,0xa5,n);free(b);KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->hat_joint&&h->desc.model_num==1&&h->desc.vis_table);
            HSD_JObj*g=HSD_JObjLoadJoint(h->hat_joint);assert(g);HSD_JObjRemoveAll(g);
            Article*d=(Article*)h->hat_dynamics[0];assert(d&&d->x4_specialAttributes&&d->xC_itemStates&&d->x10_modelDesc);
            for(unsigned j=0;j<words[k];j++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*j,4);assert(actual==expected[j]);}
            for(unsigned j=0;j<counts[k];j++){
                ItemStateDesc*anim=&d->xC_itemStates->x0_itemStateDesc[j];scripts+=anim->xC_script!=NULL;
                if(!d->x10_modelDesc->x0_joint)continue;g=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(g);models++;
                HSD_JObjAddAnimAll(g,anim->x0_anim_joint,anim->x4_matanim_joint,anim->x8_parameters);
                for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(g,frame);HSD_JObjAnimAll(g);}HSD_JObjRemoveAll(g);
            }
            melee_kirby_copy_free(o);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        }
        printf("Kirby Mario/Doctor/Luigi copies: three hats, %u article animations, %u scripts, invalid kinds and source disposal passed\n",models,scripts);return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--kirby-copy-ness")||!strcmp(argv[1],"--kirby-copy-fox"))){
        int fox=!strcmp(argv[1],"--kirby-copy-fox");
        const char*symbol=fox?"ftDataKirbyCopyFox":"ftDataKirbyCopyNess";
        FILE*f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,b,n));MeleeKirbyCopy*o=melee_kirby_copy_decode(&a,fox?Ft_Kind_Fox:Ft_Kind_Ness);assert(o);
        u32 root;assert(melee_archive_find(&a,symbol,&root));u32 expected[2][11]={{0}};const unsigned words[]={fox?10:11,fox?10:5},counts[]={fox?2:3,fox?9:1};
        for(unsigned i=0;i<2;i++){u32 article,attrs;MeleeHostBool present;assert(melee_archive_pointer(&a,root+12+4*i,&article,&present)&&present);assert(melee_archive_pointer(&a,article+4,&attrs,&present)&&present);for(unsigned j=0;j<words[i];j++)assert(melee_archive_u32(&a,attrs+4*j,&expected[i][j]));}
        memset(b,0xa5,n);free(b);KirbyHatStruct*h=melee_kirby_copy_descriptor(o);assert(h&&h->hat_joint&&h->desc.model_num==1&&h->desc.vis_table);
        HSD_JObj*g=HSD_JObjLoadJoint(h->hat_joint);assert(g);HSD_JObjRemoveAll(g);unsigned models=0,scripts=0;
        for(unsigned i=0;i<2;i++){
            Article*d=(Article*)h->hat_dynamics[i];assert(d&&d->x4_specialAttributes&&d->xC_itemStates&&d->x10_modelDesc);
            for(unsigned j=0;j<words[i];j++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*j,4);assert(actual==expected[i][j]);}
            for(unsigned j=0;j<counts[i];j++){
                ItemStateDesc*anim=&d->xC_itemStates->x0_itemStateDesc[j];scripts+=anim->xC_script!=NULL;
                if(!d->x10_modelDesc->x0_joint)continue;g=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(g);models++;
                HSD_JObjAddAnimAll(g,anim->x0_anim_joint,anim->x4_matanim_joint,anim->x8_parameters);
                for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(g,frame);HSD_JObjAnimAll(g);}HSD_JObjRemoveAll(g);
            }
        }
        melee_kirby_copy_free(o);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("%s: hat, visibility, %u model animations, %u scripts, source disposal and cleanup passed\n",symbol,models,scripts);return 0;
    }
    if(argc==6&&!strcmp(argv[1],"--hand-data")){
        for(unsigned hand=0;hand<2;hand++){
            u8* buffers[2];size_t sizes[2];
            for(unsigned i=0;i<2;i++){
                FILE* f=fopen(argv[2+2*hand+i],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
                buffers[i]=malloc(size);sizes[i]=size;assert(buffers[i]&&fread(buffers[i],1,size,f)==size);fclose(f);
            }
            MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));
            MeleeHandData* owner=melee_hand_data_decode(&a,buffers[1],sizes[1],hand);assert(owner);
            for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
            ftData* d=melee_hand_data_header(owner);
            assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&!d->x1C[0]&&d->x20&&!d->x58&&!d->x5C);
            assert(d->x44&&d->x50&&d->x54&&d->x48_items[0]&&d->x48_items[1]&&d->x48_items[2]);
            unsigned count=0;
            for(unsigned i=0;i<(hand?344u:345u);i++){
                struct Fighter_WaitAnimData* motion=&d->xC[i];
                if(motion->x8){assert(melee_character_motions_tree(motion->native_owner,motion->native_index));count++;}
            }
            assert(count==(hand?49u:50u));printf("%s Hand native data: %u owned animation trees after source disposal\n",hand?"Crazy":"Master",count);
            melee_hand_data_free(owner);
            assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        }
        return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--hand-items")){
        for(unsigned hand=0;hand<2;hand++){
            FILE* f=fopen(argv[2+hand],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
            u8* b=malloc(size);assert(b&&fread(b,1,size,f)==size);fclose(f);
            MeleeArchive a;u32 root,table;MeleeHostBool present;
            assert(melee_archive_open(&a,b,size)&&melee_archive_find(&a,hand?"ftDataCrazyhand":"ftDataMasterhand",&root));
            assert(melee_archive_pointer(&a,root+0x48,&table,&present)&&present);
            const unsigned words[]={2,1,3};u32 expected[3][3];
            for(unsigned i=0;i<3;i++){
                u32 article,params;assert(melee_archive_pointer(&a,table+4*i,&article,&present)&&present);
                assert(melee_archive_pointer(&a,article+4,&params,&present)&&present);
                for(unsigned j=0;j<words[i];j++)assert(melee_archive_u32(&a,params+4*j,&expected[i][j]));
            }
            MeleeHandItems* owner=melee_hand_items_decode(&a,root,hand);assert(owner);
            memset(b,0xa5,size);free(b);
            for(unsigned i=0;i<3;i++){
                Article* article=melee_hand_items_entries(owner)[i];assert(article&&article->x4_specialAttributes);
                assert(!memcmp(article->x4_specialAttributes,expected[i],4*words[i]));
                assert(article->x10_modelDesc&&article->xC_itemStates);
                HSD_JObj* joint=HSD_JObjLoadJoint(article->x10_modelDesc->x0_joint);assert(joint);
                HSD_JObjRemoveAll(joint);
            }
            melee_hand_items_free(owner);
            assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        }
        puts("Boss projectile articles: both hands, all three slots, scalar parameters and source disposal passed");return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--hand-attributes")){
        for(unsigned hand=0;hand<2;hand++){
            FILE* f=fopen(argv[2+hand],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
            u8* b=malloc(size);assert(b&&fread(b,1,size,f)==size);fclose(f);
            MeleeArchive a;u32 root,at;MeleeHostBool present;
            assert(melee_archive_open(&a,b,size)&&melee_archive_find(&a,hand?"ftDataCrazyhand":"ftDataMasterhand",&root));
            assert(melee_archive_pointer(&a,root+4,&at,&present)&&present);
            struct ftMasterHand_SpecialAttrs master;ftCrazyHand_DatAttrs crazy;
            assert(hand?melee_crazyhand_attributes_decode(&a,root,&crazy):melee_masterhand_attributes_decode(&a,root,&master));
            void* decoded=hand?(void*)&crazy:(void*)&master;unsigned bytes=hand?sizeof(crazy):sizeof(master);
            for(unsigned i=0;i<bytes;i+=4){u32 expected,actual;assert(melee_archive_u32(&a,at+i,&expected));memcpy(&actual,(u8*)decoded+i,4);assert(actual==expected);}
            /* Reject nonfinite motion parameters without partially writing output. */
            u8 saved[4];memcpy(saved,b+32+at+0x30,4);memcpy(b+32+at+0x30,"\x7f\x80\x00\x00",4);
            u8 before[sizeof(master)];memcpy(before,decoded,bytes);
            assert(!(hand?melee_crazyhand_attributes_decode(&a,root,&crazy):melee_masterhand_attributes_decode(&a,root,&master)));
            assert(!memcmp(before,decoded,bytes));memcpy(b+32+at+0x30,saved,4);
            memset(b,0xa5,size);free(b);assert(!memcmp(before,decoded,bytes));
        }
        puts("Master/Crazy Hand attributes: all words, finite motion validation, atomic rejection and source disposal passed");return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--kirby-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));assert(!strcmp(name,"ftDataKirby"));MeleeKirbyData* owner=melee_kirby_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        u32 table;MeleeHostBool present;assert(melee_archive_pointer(&a,root+12,&table,&present)&&present);
        ftData* original=melee_kirby_data_header(owner);
        for(unsigned i=0;i<479;i++){u32 script;assert(melee_archive_pointer(&a,table+24*i+12,&script,&present));assert((original->xC[i].xC!=NULL)==present);}
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_kirby_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<479;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations==433&&scripts==468&&d->x48_items[0]&&d->x48_items[1]);
        /* The retail archive loader clears both external middle-hand poses.
         * Other hand poses and their tracks must survive source disposal. */
        for(unsigned group=0;group<2;group++){
            assert(d->x1C[group]->x8[0]==NULL);
            for(unsigned pose=1;pose<3;pose++){
                HSD_AnimJoint* anim=d->x1C[group]->x8[pose];
                assert(anim&&anim->aobjdesc&&anim->aobjdesc->fobjdesc);
            }
        }
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_kirby_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Kirby normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--gamewatch-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));assert(!strcmp(name,"ftDataGamewatch"));MeleeGameWatchData* owner=melee_gamewatch_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_gamewatch_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<323;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations==269&&scripts==323&&d->x48_items[0]&&d->x48_items[1]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_gamewatch_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("GameWatch normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--mewtwo-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));assert(!strcmp(name,"ftDataMewtwo"));MeleeMewtwoData* owner=melee_mewtwo_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_mewtwo_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<314;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations==263&&scripts==314&&d->x48_items[0]&&d->x48_items[1]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_mewtwo_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Mewtwo normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--peach-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));assert(!strcmp(name,"ftDataPeach"));MeleePeachData* owner=melee_peach_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_peach_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<318;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations==265&&scripts==318&&d->x48_items[0]&&d->x48_items[1]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_peach_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Peach normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--samus-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));assert(!strcmp(name,"ftDataSamus"));MeleeSamusData* owner=melee_samus_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_samus_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<313;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations==265&&scripts==313&&d->x48_items[0]&&d->x48_items[1]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_samus_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Samus normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--link-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));int young=!strcmp(name,"ftDataClink");assert(young||!strcmp(name,"ftDataLink"));MeleeLinkData* owner=melee_link_data_decode(&a,buffers[1],sizes[1],young);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_link_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<314;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations==(young?264u:261u)&&scripts==314&&d->x48_items[0]&&d->x48_items[1]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_link_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Link normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--zelda-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));assert(!strcmp(name,"ftDataZelda"));MeleeZeldaData* owner=melee_zelda_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_zelda_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<311;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations==261&&scripts==311&&d->x48_items[0]&&d->x48_items[1]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_zelda_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Zelda normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--yoshi-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));assert(!strcmp(name,"ftDataYoshi"));MeleeYoshiData* owner=melee_yoshi_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_yoshi_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        assert(!d->x20->x0&&d->x20->x8==0.0f);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<314;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations>0&&scripts>0&&d->x48_items[0]&&d->x48_items[1]&&d->x48_items[2]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_yoshi_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Yoshi normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--pikachu-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));bool pichu=!strcmp(name,"ftDataPichu");assert(pichu||!strcmp(name,"ftDataPikachu"));MeleePikachuData* owner=melee_pikachu_data_decode(&a,buffers[1],sizes[1],pichu);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_pikachu_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<320;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations>0&&scripts>0&&d->x48_items[0]&&d->x48_items[1]&&d->x48_items[2]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_pikachu_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Pikachu/Pichu normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--luigi-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));assert(!strcmp(name,"ftDataLuigi"));MeleeLuigiData* owner=melee_luigi_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_luigi_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<312;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations==260&&scripts==312&&d->x48_items[0]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_luigi_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Luigi normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--koopa-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));bool giga=!strcmp(name,"ftDataGkoopa");assert(giga||!strcmp(name,"ftDataKoopa"));MeleeKoopaData* owner=melee_koopa_data_decode(&a,buffers[1],sizes[1],giga);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_koopa_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<316;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        assert(animations==(giga?267u:269u)&&scripts==316&&d->x48_items[0]);
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_koopa_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Bowser/Giga Bowser normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--mario-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));bool doctor=!strcmp(name,"ftDataDrmario");assert(doctor||!strcmp(name,"ftDataMario"));MeleeMarioData* owner=melee_mario_data_decode(&a,buffers[1],sizes[1],doctor);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_mario_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<303;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_mario_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Mario/Dr. Mario normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--ness-data")){
        u8* buffers[2];size_t sizes[2];
        for(unsigned i=0;i<2;i++){FILE* file=fopen(argv[2+i],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);sizes[i]=size;buffers[i]=malloc(size);assert(buffers[i]&&fread(buffers[i],1,size,file)==size);fclose(file);}
        MeleeArchive a;assert(melee_archive_open(&a,buffers[0],sizes[0]));MeleeNessData* owner=melee_ness_data_decode(&a,buffers[1],sizes[1]);assert(owner);
        for(unsigned i=0;i<2;i++){memset(buffers[i],0xa5,sizes[i]);free(buffers[i]);}
        ftData* d=melee_ness_data_header(owner);assert(d&&d->x0&&d->ext_attr&&d->x8&&d->xC&&d->x10&&d->x1C&&d->x20&&d->x2C&&d->x30&&d->x34&&d->x38&&d->x3C&&d->x40&&d->x44&&d->x48_items&&d->x4C_sfx&&d->x50&&d->x54&&d->x58&&d->x5C&&!d->x14);
        unsigned animations=0,scripts=0;for(unsigned i=0;i<326;i++){struct Fighter_WaitAnimData* r=d->xC+i;if(r->x8){assert(melee_character_motions_tree(r->native_owner,r->native_index));animations++;}scripts+=r->xC!=NULL;}
        HSD_JObj* joint=HSD_JObjLoadJoint(d->x5C);assert(joint);HSD_JObjRemoveAll(joint);melee_ness_data_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);printf("Ness normal-match data: assembled owner, %u animations, %u scripts, source disposal and cleanup passed; demo motions absent\n",animations,scripts);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--yoshi-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root;assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root));
        assert(!strcmp(name,"ftDataYoshi"));
        MeleeYoshiItems* owner=melee_yoshi_items_decode(&a,root);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_yoshi_items_entries(owner);
        const unsigned counts[]={2,1,0};
        for(unsigned i=0;i<3;i++){
            Article* d=entries[i];assert(d&&d->x0_common_attr&&d->x10_modelDesc);
            for(unsigned j=0;j<counts[i];j++)assert(d->xC_itemStates->x0_itemStateDesc[j].xC_script);
        }
        float* egg=((Article*)entries[0])->x4_specialAttributes;
        float* star=((Article*)entries[1])->x4_specialAttributes;
        assert(egg&&egg[0]==54.0f&&egg[1]==20.0f);
        assert(star&&star[0]==0.8f&&star[1]==-0.02f);
        Article* lay=entries[2];assert(!lay->x4_specialAttributes&&!lay->xC_itemStates&&lay->x8_hurtbones);
        HSD_JObj* capture=HSD_JObjLoadJoint(entries[3]);assert(capture&&HSD_JObjGetChild(capture));
        HSD_JObjRemoveAll(capture);
        melee_yoshi_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Yoshi: thrown egg, star and Egg Lay, capture model, three animation scripts, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--sheik-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root;assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root));
        assert(!strcmp(name,"ftDataSeak"));
        MeleeSheikItems* owner=melee_sheik_items_decode(&a,root);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_sheik_items_entries(owner);
        const unsigned counts[]={5,1,1,0};unsigned scripts=0,models=0;
        for(unsigned i=0;i<4;i++){
            Article* d=entries[i];assert(d&&d->x0_common_attr&&d->x10_modelDesc);
            assert((d->x4_specialAttributes!=NULL)==(i!=2));
            assert((d->xC_itemStates!=NULL)==(counts[i]!=0));
            for(unsigned j=0;j<counts[i];j++)scripts+=d->xC_itemStates->x0_itemStateDesc[j].xC_script!=NULL;
            if(d->x10_modelDesc->x0_joint){HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);HSD_JObjRemoveAll(joint);models++;}
        }
        assert(scripts==7&&models==3);
        float* needle=((Article*)entries[0])->x4_specialAttributes;
        assert(needle[0]==30.0f&&needle[1]==120.0f&&needle[2]==4.0f);
        itSeakChain_Attrs* chain=((Article*)entries[3])->x4_specialAttributes;
        assert(chain->x0==20&&chain->x4==1.5f&&chain->x60==1.0f);
        HSD_JObj* segment=HSD_JObjLoadJoint(chain->x64_joint);assert(segment);HSD_JObjRemoveAll(segment);
        segment=HSD_JObjLoadJoint(chain->x68_joint);assert(segment);HSD_JObjRemoveAll(segment);
        for(unsigned i=4;i<6;i++){
            HSD_Joint* pose=entries[i];assert(pose&&pose->child);
            HSD_JObj* skeleton=HSD_JObjLoadJoint(pose);assert(skeleton&&HSD_JObjGetChild(skeleton));HSD_JObjRemoveAll(skeleton);
        }
        melee_sheik_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Sheik: four articles, %u scripts, %u item models, two chain models and two fighter pose skeletons, source disposal and cleanup passed\n",scripts,models);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--link-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root;
        assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root));
        int young=!strcmp(name,"ftDataClink");assert(young||!strcmp(name,"ftDataLink"));
        MeleeLinkItems* owner=melee_link_items_decode(&a,root,young);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_link_items_entries(owner);
        for(unsigned i=0;i<5;i++)assert(entries[i]);assert((entries[5]!=NULL)==young);
        itLinkBoomerangAttributes* boomerang=((Article*)entries[1])->x4_specialAttributes;
        HSD_Joint* models[]={boomerang->x44,boomerang->x48};
        AnimBundle* bundles[]={&boomerang->x4C_anim,&boomerang->x58_anim};
        for(unsigned i=0;i<2;i++){
            HSD_JObj* joint=HSD_JObjLoadJoint(models[i]);assert(joint);
            assert(bundles[i]->anim||bundles[i]->matanim||bundles[i]->shapeanim);
            HSD_JObjAddAnimAll(joint,bundles[i]->anim,bundles[i]->matanim,bundles[i]->shapeanim);
            HSD_JObjReqAnimAll(joint,0);HSD_JObjAnimAll(joint);
            HSD_JObjReqAnimAll(joint,10);HSD_JObjAnimAll(joint);HSD_JObjRemoveAll(joint);
        }
        HSD_JObj* accessory=HSD_JObjLoadJoint(entries[6]);assert(accessory);HSD_JObjRemoveAll(accessory);
        melee_link_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Link complete items: young=%d seven entries, two boomerang animation bundles, accessory, source disposal and cleanup passed\n",young);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--link-tether-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root,table;MeleeHostBool present;
        assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root));
        int young=!strcmp(name,"ftDataClink");assert(young||!strcmp(name,"ftDataLink"));
        assert(melee_archive_pointer(&a,root+0x48,&table,&present)&&present);
        MeleeItemArticle* owners[2];
        for(unsigned i=0;i<2;i++){
            u32 at;assert(melee_archive_pointer(&a,table+4*(i?3:2),&at,&present)&&present);
            unsigned kind=i?(young?It_Kind_CLink_Arrow:It_Kind_Link_Arrow):(young?It_Kind_CLink_HShot:It_Kind_Link_HShot);
            owners[i]=melee_item_article_decode(&a,kind,at,i?1:0);assert(owners[i]);
        }
        memset(bytes,0xa5,size);free(bytes);
        Article* hook=melee_item_article_descriptor(owners[0]);Article* arrow=melee_item_article_descriptor(owners[1]);
        itLinkHookshotAttributes* h=hook->x4_specialAttributes;itLinkArrowAttributes* ar=arrow->x4_specialAttributes;
        assert(h->xC==(young?10:15)&&h->x2C==(young?10:15)&&h->x50==1.5f);
        assert(ar->x0==(young?55.0f:60.0f)&&!hook->xC_itemStates&&arrow->xC_itemStates);
        HSD_Joint* models[]={h->x54,h->x58,h->x5C,ar->x24,ar->x28,arrow->x10_modelDesc->x0_joint};
        for(unsigned i=0;i<6;i++){HSD_JObj* joint=HSD_JObjLoadJoint(models[i]);assert(joint);HSD_JObjRemoveAll(joint);}
        melee_item_article_free(owners[0]);melee_item_article_free(owners[1]);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Link tether/arrow: young=%d six models, source disposal and cleanup passed\n",young);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--samus-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root;
        assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root)&&!strcmp(name,"ftDataSamus"));
        u32 table,beam_at,model,first;MeleeHostBool present;
        assert(melee_archive_pointer(&a,root+0x48,&table,&present)&&present);
        assert(melee_archive_pointer(&a,table+16,&beam_at,&present)&&present);
        assert(melee_archive_pointer(&a,beam_at,&model,&present)&&present);
        assert(melee_archive_pointer(&a,model+8,&first,&present)&&present);
        /* Turning a borrowed reference into a second ordinary owner must fail. */
        u8 flag=bytes[32+first+6];bytes[32+first+6]&=~0x10;
        assert(!melee_scene_decode(&a,model));bytes[32+first+6]=flag;
        MeleeSamusItems* owner=melee_samus_items_decode(&a,root);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_samus_items_entries(owner);
        for(unsigned i=0;i<4;i++){Article* article=entries[i];assert(article&&article->x4_specialAttributes&&article->x10_modelDesc);}
        struct UNK_SAMUS_S1* beam=entries[4];assert(beam&&beam->x4_anim_joints);
        for(unsigned i=0;i<4;i++){
            HSD_JObj* joint=HSD_JObjLoadJoint(beam->x0_joint);assert(joint&&beam->x4_anim_joints[i]);
            unsigned instances=0;
            for(HSD_JObj* child=joint->child;child;child=child->next)if(child->flags&JOBJ_INSTANCE){
                unsigned owners=0;for(HSD_JObj* target=joint->child;target;target=target->next)owners+=target==child->child&&!(target->flags&JOBJ_INSTANCE);
                assert(owners==1);instances++;
            }
            assert(instances==25);
            HSD_JObjAddAnimAll(joint,beam->x8_anim_joint,beam->xC_matanim_joint,NULL);
            HSD_JObjAddAnimAll(joint,beam->x4_anim_joints[i],NULL,NULL);
            for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
            HSD_JObjRemoveAll(joint);
        }
        melee_samus_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Samus item table: four articles and four throw attachment animations, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--samus-grapple")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root,table,at,special;MeleeHostBool present;
        assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root)&&!strcmp(name,"ftDataSamus"));
        assert(melee_archive_pointer(&a,root+0x48,&table,&present)&&present);
        assert(melee_archive_pointer(&a,table+12,&at,&present)&&present);
        assert(melee_archive_pointer(&a,at+4,&special,&present)&&present);
        u32 expected[25];for(unsigned i=0;i<25;i++)assert(melee_archive_u32(&a,special+4*i,&expected[i]));
        MeleeItemArticle* owner=melee_item_article_decode(&a,It_Kind_Samus_GBeam,at,0);assert(owner);
        memset(bytes,0xa5,size);free(bytes);
        Article* article=melee_item_article_descriptor(owner);itSamusGrappleAttributes* attrs=article->x4_specialAttributes;
        for(unsigned i=0;i<25;i++){u32 actual;memcpy(&actual,(u8*)attrs+4*i,4);assert(actual==expected[i]);}
        assert(attrs->xC==30&&attrs->x34==30);
        HSD_Joint* models[]={attrs->x64,attrs->x68,attrs->x6C,attrs->x70,article->x10_modelDesc->x0_joint};
        HSD_AnimJoint** anims[]={attrs->x74,attrs->x80,attrs->x8C,attrs->x98,attrs->xA4};
        HSD_MatAnimJoint** mats[]={attrs->x78,attrs->x84,attrs->x90,attrs->x9C,attrs->xA8};
        HSD_ShapeAnimJoint** shapes[]={attrs->x7C,attrs->x88,attrs->x94,attrs->xA0,attrs->xAC};
        for(unsigned i=0;i<5;i++){
            HSD_JObj* joint=HSD_JObjLoadJoint(models[i]);assert(joint&&joint->child);
            HSD_JObjAddAnimAll(joint->child,anims[i]?*anims[i]:NULL,mats[i]?*mats[i]:NULL,shapes[i]?*shapes[i]:NULL);
            for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(joint->child,frame);HSD_JObjAnimAll(joint->child);}
            HSD_JObjRemoveAll(joint);
        }
        melee_item_article_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Samus grapple: 25 scalar words, five child animation bundles, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--samus-basic-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root,table;MeleeHostBool present;
        assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root));
        assert(!strcmp(name,"ftDataSamus"));
        assert(melee_archive_pointer(&a,root+0x48,&table,&present)&&present);
        const unsigned slots[]={0,1,2},counts[]={2,9,4};
        const ItemKind kinds[]={It_Kind_Samus_Bomb,It_Kind_Samus_Charge,It_Kind_Samus_Missile};
        const unsigned lengths[]={16,32,56};
        u32 expected[3][14]={{0}};
        MeleeItemArticle* owners[3]={0};
        for(unsigned i=0;i<3u;i++){
            u32 at;assert(melee_archive_pointer(&a,table+4*slots[i],&at,&present)&&present);
            u32 special;assert(melee_archive_pointer(&a,at+4,&special,&present)&&present);
            for(unsigned j=0;j<lengths[i]/4;j++)assert(melee_archive_u32(&a,special+4*j,&expected[i][j]));
            owners[i]=melee_item_article_decode(&a,kinds[i],at,counts[i]);assert(owners[i]);
        }
        memset(bytes,0xa5,size);free(bytes);unsigned scripts=0;
        for(unsigned i=0;i<3u;i++){
            Article* d=melee_item_article_descriptor(owners[i]);assert(d&&d->x4_specialAttributes&&d->xC_itemStates);
            for(unsigned j=0;j<lengths[i]/4;j++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*j,4);assert(actual==expected[i][j]);}
            for(unsigned j=0;j<counts[i];j++){
                ItemStateDesc* animation=&d->xC_itemStates->x0_itemStateDesc[j];scripts+=animation->xC_script!=NULL;
                HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);
                HSD_JObjAddAnimAll(joint,animation->x0_anim_joint,animation->x4_matanim_joint,animation->x8_parameters);
                for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);
            }
        }
        itSamusBombAttributes* bomb=melee_item_article_descriptor(owners[0])->x4_specialAttributes;
        assert(bomb->x0==72&&bomb->x4==40&&bomb->x8==2);
        for(unsigned i=0;i<3;i++)melee_item_article_free(owners[i]);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Samus basic items: scripts=%u scalar words and 15 state animations, source disposal and cleanup passed\n",scripts);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--link-basic-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root,table;MeleeHostBool present;
        assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root));
        int young=!strcmp(name,"ftDataClink");assert(young||!strcmp(name,"ftDataLink"));
        assert(melee_archive_pointer(&a,root+0x48,&table,&present)&&present);
        const unsigned slots[]={0,4,5},counts[]={4,6,2};
        const ItemKind kinds[]={young?It_Kind_CLink_Bomb:It_Kind_Link_Bomb,young?It_Kind_CLink_Bow:It_Kind_Link_Bow,It_Kind_CLink_Milk};
        MeleeItemArticle* owners[3]={0};
        for(unsigned i=0;i<(young?3u:2u);i++){
            u32 at;assert(melee_archive_pointer(&a,table+4*slots[i],&at,&present)&&present);
            owners[i]=melee_item_article_decode(&a,kinds[i],at,counts[i]);assert(owners[i]);
        }
        memset(bytes,0xa5,size);free(bytes);unsigned scripts=0;
        for(unsigned i=0;i<(young?3u:2u);i++){
            Article* d=melee_item_article_descriptor(owners[i]);assert(d&&d->x4_specialAttributes&&d->xC_itemStates);
            HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);HSD_JObjRemoveAll(joint);
            for(unsigned j=0;j<counts[i];j++)scripts+=d->xC_itemStates->x0_itemStateDesc[j].xC_script!=NULL;
        }
        itLinkBombAttributes* bomb=melee_item_article_descriptor(owners[0])->x4_specialAttributes;
        assert(bomb->lifetime==300&&bomb->x4==6&&bomb->x8==10);
        for(unsigned i=0;i<3;i++)melee_item_article_free(owners[i]);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Link basic items: young=%d scripts=%u models, source disposal and cleanup passed\n",young,scripts);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--iceclimbers-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root,table;MeleeHostBool present;
        assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root));
        assert(!strcmp(name,"ftDataPopo")||!strcmp(name,"ftDataNana"));
        assert(melee_archive_pointer(&a,root+0x48,&table,&present)&&present);
        const unsigned counts[]={1,1,0},words[]={13,6,9};u32 expected[3][13]={{0}};
        for(unsigned i=0;i<3;i++){
            u32 article,special;assert(melee_archive_pointer(&a,table+4*i,&article,&present)&&present);
            assert(melee_archive_pointer(&a,article+4,&special,&present)&&present);
            for(unsigned j=0;j<words[i];j++)assert(melee_archive_u32(&a,special+4*j,&expected[i][j]));
        }
        for(unsigned i=0;i<3;i+=2){u32 article,special;assert(melee_archive_pointer(&a,table+4*i,&article,&present)&&present);assert(melee_archive_pointer(&a,article+4,&special,&present)&&present);unsigned j=i==0?8:4;memcpy(&expected[i][j],bytes+32+special+4*j,4);}
        MeleeIceClimbersItems* owner=melee_iceclimbers_items_decode(&a,root);
        if(!strcmp(name,"ftDataNana")){
            assert(!owner);free(bytes);
            assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
            puts("Nana standalone items: unresolved shared Popo models rejected with clean ownership");return 0;
        }
        assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_iceclimbers_items_entries(owner);unsigned models=0,scripts=0;
        for(unsigned i=0;i<3;i++){
            Article* d=entries[i];assert(d&&d->x0_common_attr&&d->x10_modelDesc);
            assert(d->x4_specialAttributes);
            for(unsigned j=0;j<words[i];j++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*j,4);if((i==0&&j==8)||(i==2&&j==4))assert(!memcmp((u8*)d->x4_specialAttributes+4*j,(u8*)&expected[i][j],4));else assert(actual==expected[i][j]);}
            for(unsigned j=0;j<counts[i];j++){
                ItemStateDesc* animation=&d->xC_itemStates->x0_itemStateDesc[j];scripts+=animation->xC_script!=NULL;
                if(!d->x10_modelDesc->x0_joint)continue;
                HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);models++;
                HSD_JObjAddAnimAll(joint,animation->x0_anim_joint,animation->x4_matanim_joint,animation->x8_parameters);
                for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);
            }
        }
        itClimbersStringAttributes* rope=((Article*)entries[2])->x4_specialAttributes;
        HSD_Joint* variants[]={rope->x24_joint,rope->x28_joint};
        assert(rope->x0_count==40&&rope->x4==25);
        for(unsigned i=0;i<2;i++){HSD_JObj* joint=HSD_JObjLoadJoint(variants[i]);assert(joint);HSD_JObjRemoveAll(joint);}
        melee_iceclimbers_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("IceClimbers: three items and two rope models, %u model animations, %u scripts, source disposal and cleanup passed\n",models,scripts);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kirby-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root,table;MeleeHostBool present;
        assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root));
        assert(!strcmp(name,"ftDataKirby"));
        assert(melee_archive_pointer(&a,root+0x48,&table,&present)&&present);
        const unsigned counts[]={1,1,1,1},words[]={4,0,1,0};u32 expected[4][4]={{0}};
        for(unsigned i=0;i<4;i++){
            u32 article,special;assert(melee_archive_pointer(&a,table+4*i,&article,&present)&&present);
            assert(melee_archive_pointer(&a,article+4,&special,&present)&&present==(words[i]!=0));
            for(unsigned j=0;j<words[i];j++)assert(melee_archive_u32(&a,special+4*j,&expected[i][j]));
        }
        MeleeKirbyItems* owner=melee_kirby_items_decode(&a,root);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_kirby_items_entries(owner);unsigned models=0,scripts=0;
        for(unsigned i=0;i<4;i++){
            Article* d=entries[i];assert(d&&d->x0_common_attr&&d->x10_modelDesc&&d->xC_itemStates);
            assert((d->x4_specialAttributes!=NULL)==(words[i]!=0));
            for(unsigned j=0;j<words[i];j++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*j,4);assert(actual==expected[i][j]);}
            for(unsigned j=0;j<counts[i];j++){
                ItemStateDesc* animation=&d->xC_itemStates->x0_itemStateDesc[j];scripts+=animation->xC_script!=NULL;
                if(!d->x10_modelDesc->x0_joint)continue;
                HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);models++;
                HSD_JObjAddAnimAll(joint,animation->x0_anim_joint,animation->x4_matanim_joint,animation->x8_parameters);
                for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);
            }
        }
        HSD_JObj* swallowed=HSD_JObjLoadJoint(entries[4]);assert(swallowed);HSD_JObjRemoveAll(swallowed);
        melee_kirby_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Kirby: four base items and swallowed-opponent model, %u model animations, %u scripts, source disposal and cleanup passed\n",models,scripts);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--mewtwo-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root,table;MeleeHostBool present;
        assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root));
        assert(!strcmp(name,"ftDataMewtwo"));
        assert(melee_archive_pointer(&a,root+0x48,&table,&present)&&present);
        const unsigned counts[]={1,10},words[]={2,12};u32 expected[2][12]={{0}};
        for(unsigned i=0;i<2;i++){
            u32 article,special;assert(melee_archive_pointer(&a,table+4*i,&article,&present)&&present);
            assert(melee_archive_pointer(&a,article+4,&special,&present)&&present);
            for(unsigned j=0;j<words[i];j++)assert(melee_archive_u32(&a,special+4*j,&expected[i][j]));
        }
        MeleeMewtwoItems* owner=melee_mewtwo_items_decode(&a,root);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_mewtwo_items_entries(owner);unsigned models=0,scripts=0;
        for(unsigned i=0;i<2;i++){
            Article* d=entries[i];assert(d&&d->x0_common_attr&&d->x10_modelDesc&&d->xC_itemStates);
            assert(d->x4_specialAttributes);
            for(unsigned j=0;j<words[i];j++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*j,4);assert(actual==expected[i][j]);}
            for(unsigned j=0;j<counts[i];j++){
                ItemStateDesc* animation=&d->xC_itemStates->x0_itemStateDesc[j];scripts+=animation->xC_script!=NULL;
                if(!d->x10_modelDesc->x0_joint)continue;
                HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);models++;
                HSD_JObjAddAnimAll(joint,animation->x0_anim_joint,animation->x4_matanim_joint,animation->x8_parameters);
                for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);
            }
        }
        melee_mewtwo_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Mewtwo: two items, %u model animations, %u scripts, source disposal and cleanup passed\n",models,scripts);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--gamewatch-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root,table;MeleeHostBool present;
        assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root)&&!strcmp(name,"ftDataGamewatch"));
        assert(melee_archive_pointer(&a,root+0x48,&table,&present)&&present);
        u8 expected[10][2][256]={0};unsigned lengths[10][2];float chef[28];
        for(unsigned i=0;i<10;i++){
            u32 article,special,parts;assert(melee_archive_pointer(&a,table+4*i,&article,&present)&&present);
            assert(melee_archive_pointer(&a,article+4,&special,&present)&&present);
            assert(melee_archive_pointer(&a,special,&parts,&present)&&present);
            for(unsigned j=0;j<2;j++){u32 packed,indices;assert(melee_archive_u32(&a,parts+8*j,&packed));lengths[i][j]=packed>>16;assert(lengths[i][j]<=256);assert(melee_archive_pointer(&a,parts+8*j+4,&indices,&present)&&present);memcpy(expected[i][j],bytes+32+indices,lengths[i][j]);}
            if(i==8)for(unsigned j=0;j<28;j++)assert(melee_archive_f32(&a,special+4+4*j,&chef[j]));
        }
        MeleeGameWatchItems* owner=melee_gamewatch_items_decode(&a,root);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_gamewatch_items_entries(owner);
        const unsigned counts[]={4,1,1,2,2,2,1,2,2,2};unsigned models=0,scripts=0;
        for(unsigned i=0;i<10;i++){
            Article* d=entries[i];assert(d&&d->x4_specialAttributes&&d->xC_itemStates);
            it_266F_ItemVars* parts=*(void**)d->x4_specialAttributes;assert(parts&&parts->x0==lengths[i][0]&&parts->x8==lengths[i][1]);
            assert(!memcmp(parts->x4,expected[i][0],parts->x0)&&!memcmp(parts->xC,expected[i][1],parts->x8));
            if(i==8){itGamewatchchefAttributes* attrs=d->x4_specialAttributes;assert(!memcmp(&attrs->x4,chef,sizeof(chef)));}
            for(unsigned j=0;j<counts[i];j++){
                ItemStateDesc* animation=&d->xC_itemStates->x0_itemStateDesc[j];scripts+=animation->xC_script!=NULL;
                HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);models++;
                HSD_JObjAddAnimAll(joint,animation->x0_anim_joint,animation->x4_matanim_joint,animation->x8_parameters);
                for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);
            }
        }
        FtPartsVisLookup* outline=entries[10];assert(outline);unsigned outline_choices=0;
        const unsigned expected_counts[]={1,1,4,1,1,8,1,1,1,1,6};
        for(unsigned i=0;i<11;i++){assert(outline[i].x0==expected_counts[i]);for(int j=0;j<outline[i].x0;j++){TempS* choice=&outline[i].x4[j];assert(choice->x0>=0&&choice->x0<=124&&choice->x4);for(int k=0;k<choice->x0;k++)assert(choice->x4[k]<124);outline_choices++;}}
        assert(outline_choices==26);
        Item alias={0};alias.xDD4_itemVar.gamewatchchef.x4=4;
        alias.xDD4_itemVar.gamewatch.attr=outline;
        assert(alias.xDD4_itemVar.gamewatchchef.x4==4&&alias.xDD4_itemVar.gamewatchchef.x0==outline);
        melee_gamewatch_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Game & Watch: ten items, twenty outline index lists, 28 Chef floats, %u model animations, %u scripts, source disposal and cleanup passed\n",models,scripts);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--peach-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root,table;MeleeHostBool present;
        assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root));
        assert(!strcmp(name,"ftDataPeach"));
        assert(melee_archive_pointer(&a,root+0x48,&table,&present)&&present);
        const unsigned counts[]={2,3,2,2,1},words[]={0,18,1,1,4};u32 expected[5][18]={{0}};
        for(unsigned i=1;i<5;i++){
            u32 article,special;assert(melee_archive_pointer(&a,table+4*i,&article,&present)&&present);
            assert(melee_archive_pointer(&a,article+4,&special,&present)&&present);
            for(unsigned j=0;j<words[i];j++)assert(melee_archive_u32(&a,special+4*j,&expected[i][j]));
        }
        MeleePeachItems* owner=melee_peach_items_decode(&a,root);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_peach_items_entries(owner);unsigned models=0,scripts=0;
        for(unsigned i=0;i<5;i++){
            Article* d=entries[i];assert(d&&d->x0_common_attr&&d->x10_modelDesc&&d->xC_itemStates);
            assert((d->x4_specialAttributes!=NULL)==(i!=0));
            for(unsigned j=0;j<words[i];j++){u32 actual;memcpy(&actual,(u8*)d->x4_specialAttributes+4*j,4);assert(actual==expected[i][j]);}
            for(unsigned j=0;j<counts[i];j++){
                ItemStateDesc* animation=&d->xC_itemStates->x0_itemStateDesc[j];scripts+=animation->xC_script!=NULL;
                if(!d->x10_modelDesc->x0_joint)continue;
                HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);models++;
                HSD_JObjAddAnimAll(joint,animation->x0_anim_joint,animation->x4_matanim_joint,animation->x8_parameters);
                for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);
            }
        }
        Item turnip={0};HSD_GObj object={0};object.user_data=&turnip;turnip.xC4_article_data=entries[1];
        unsigned selected=0;u32 saved_seed=*seed_ptr;
        for(unsigned trial=0;trial<65536;trial++){
            const unsigned thresholds[]={35,41,46,49,52,56,57,58};
            *seed_ptr=trial;unsigned roll=HSD_Randi(58),expected_variant=0;
            while(roll>=thresholds[expected_variant])expected_variant++;
            *seed_ptr=trial;int result=it_802BD32C(&object);assert(result==(int)expected_variant);selected|=1u<<result;
        }
        *seed_ptr=saved_seed;assert(selected==255);
        melee_peach_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Peach: five items, %u model animations, %u scripts, source disposal and cleanup passed\n",models,scripts);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--zelda-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root;assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root));
        assert(!strcmp(name,"ftDataZelda"));
        MeleeZeldaItems* owner=melee_zelda_items_decode(&a,root);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_zelda_items_entries(owner);
        const unsigned counts[]={2,1};
        for(unsigned i=0;i<2;i++){
            Article* d=entries[i];assert(d&&d->x0_common_attr&&d->x4_specialAttributes&&d->x10_modelDesc&&d->xC_itemStates);
            for(unsigned j=0;j<counts[i];j++)assert(d->xC_itemStates->x0_itemStateDesc[j].xC_script);
        }
        float* fire=((Article*)entries[0])->x4_specialAttributes;
        float* explosion=((Article*)entries[1])->x4_specialAttributes;
        assert(fire[0]==65.0f&&fire[1]==60.0f&&fire[11]==22.0f);
        assert(explosion[0]==60.0f&&explosion[3]==3.0f&&explosion[4]==0.17f);
        for(unsigned i=0;i<2;i++)assert(!((Article*)entries[i])->x10_modelDesc->x0_joint);
        melee_zelda_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Zelda: projectile and explosion, three animation scripts, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--pikachu-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;const char* name;u32 root;assert(melee_archive_open(&a,bytes,size)&&melee_archive_public(&a,0,&name,&root));
        bool pichu=!strcmp(name,"ftDataPichu");assert(pichu||!strcmp(name,"ftDataPikachu"));
        MeleePikachuItems* owner=melee_pikachu_items_decode(&a,root,pichu);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_pikachu_items_entries(owner);
        const unsigned counts[]={1,2,1};
        for(unsigned i=0;i<3;i++){
            Article* d=entries[i];assert(d&&d->x0_common_attr&&d->x4_specialAttributes&&d->x10_modelDesc&&d->xC_itemStates);
            for(unsigned j=0;j<counts[i];j++)assert(d->xC_itemStates->x0_itemStateDesc[j].xC_script);
        }
        float* thunder=((Article*)entries[0])->x4_specialAttributes;
        assert(thunder[0]==(pichu?40.0f:60.0f)&&thunder[1]==40.0f&&thunder[2]==38.0f);
        melee_pikachu_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Pikachu/Pichu: three owned projectiles, four animation scripts, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--luigi-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);assert(size>32);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==(size_t)size);fclose(file);
        MeleeArchive archive;u32 root;const char* name;
        assert(melee_archive_open(&archive,bytes,size)&&melee_archive_public(&archive,0,&name,&root));
        assert(!strcmp(name,"ftDataLuigi"));
        MeleeLuigiItems* owner=melee_luigi_items_decode(&archive,root);assert(owner);
        memset(bytes,0xa5,size);free(bytes);
        Article* article=melee_luigi_items_entries(owner)[0];
        assert(article&&article->x0_common_attr&&article->x4_specialAttributes&&article->x10_modelDesc&&article->xC_itemStates);
        float* special=article->x4_specialAttributes;
        assert(special[0]==1.2f&&special[1]==50.0f&&special[2]==0.85f&&special[3]==0.9f);
        ItemStateDesc* animation=article->xC_itemStates->x0_itemStateDesc;
        assert(animation->xC_script);
        HSD_JObj* joint=HSD_JObjLoadJoint(article->x10_modelDesc->x0_joint);assert(joint);
        HSD_JObjAddAnimAll(joint,animation->x0_anim_joint,animation->x4_matanim_joint,animation->x8_parameters);
        for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
        HSD_JObjRemoveAll(joint);
        melee_luigi_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        puts("Luigi fireball: owned attributes, command script and animated model, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--koopa-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);assert(size>32);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==(size_t)size);fclose(file);
        MeleeArchive archive;u32 root;const char* name;
        assert(melee_archive_open(&archive,bytes,size)&&melee_archive_public(&archive,0,&name,&root));
        bool giga=!strcmp(name,"ftDataGkoopa");assert(giga||!strcmp(name,"ftDataKoopa"));
        MeleeKoopaItems* owner=melee_koopa_items_decode(&archive,root);assert(owner);
        memset(bytes,0xa5,size);free(bytes);
        Article* article=melee_koopa_items_entries(owner)[0];
        assert(article&&article->x0_common_attr&&article->x4_specialAttributes&&article->x10_modelDesc&&article->xC_itemStates);
        float* special=article->x4_specialAttributes;
        assert(special[0]==(giga?32.0f:28.0f)&&special[1]==20.0f&&special[2]==1.9f&&special[3]==2.2f);
        assert(special[4]>1.7f&&special[4]<2.2f&&special[5]>2.4f&&special[5]<2.6f);
        /* The flame is rendered by effects, not an article model. */
        assert(!article->x10_modelDesc->x0_joint);
        ItemStateDesc* animation=article->xC_itemStates->x0_itemStateDesc;
        assert(!animation->x0_anim_joint&&!animation->x4_matanim_joint&&!animation->x8_parameters);
        assert(animation->xC_script);
        melee_koopa_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        puts("Koopa flame: owned attributes and command script, original null model, source disposal and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--mario-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);assert(size>32);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==(size_t)size);fclose(file);
        MeleeArchive archive;u32 root;const char* name;
        assert(melee_archive_open(&archive,bytes,size)&&melee_archive_public(&archive,0,&name,&root));
        bool doctor=!strcmp(name,"ftDataDrmario");assert(doctor||!strcmp(name,"ftDataMario"));
        MeleeMarioItems* owner=melee_mario_items_decode(&archive,root,doctor);assert(owner);
        memset(bytes,0xa5,size);free(bytes);
        void** entries=melee_mario_items_entries(owner);unsigned animation_count=0;
        for(unsigned i=0;i<4;i++){
            if((i&1)!=(unsigned)doctor){assert(!entries[i]);continue;}
            Article* article=entries[i];assert(article&&article->x0_common_attr&&article->x4_specialAttributes&&article->x10_modelDesc&&article->xC_itemStates);
            if(i<2){float* special=article->x4_specialAttributes;assert(special[0]==(doctor?1.4f:1.5f)&&special[2]==75.0f);}
            unsigned count=i<2?(doctor?6:1):2;
            for(unsigned state=0;state<count;state++){
                ItemStateDesc* animation=article->xC_itemStates->x0_itemStateDesc+state;
                HSD_JObj* joint=HSD_JObjLoadJoint(article->x10_modelDesc->x0_joint);assert(joint);
                HSD_JObjAddAnimAll(joint,animation->x0_anim_joint,animation->x4_matanim_joint,animation->x8_parameters);
                for(unsigned frame=0;frame<=120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);animation_count++;
            }
        }
        melee_mario_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        printf("Mario items: two owned articles, %u animation records, source disposal and cleanup passed\n",animation_count);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--ness-items")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));
        MeleeNessItems* owner=melee_ness_items_decode(&a,root);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_ness_items_entries(owner);
        for(unsigned i=0;i<11;i++){Article* d=entries[i];assert(d&&d->x0_common_attr&&d->x4_specialAttributes&&d->x10_modelDesc);
            HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);HSD_JObjRemoveAll(joint);}
        itYoyoAttributes* yoyo=((Article*)entries[10])->x4_specialAttributes;
        HSD_JObj* string=HSD_JObjLoadJoint(yoyo->x50_string_joint);assert(string);HSD_JObjRemoveAll(string);
        HSD_JObj* joint=HSD_JObjLoadJoint(yoyo->x54_yoyo_joint);assert(joint);
        HSD_JObjAddAnimAll(joint,NULL,yoyo->x58_yoyo_matanim,NULL);HSD_JObjReqAnimAll(joint,0);HSD_JObjAnimAll(joint);
                HSD_JObjRemoveAll(joint);
        melee_ness_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Ness items: 11 articles, 12 states, source-free model reloads, yoyo material animation and cleanup passed");return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--fox-items")){
        bool falco=!strcmp(argv[3],"falco");assert(falco||!strcmp(argv[3],"fox"));
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));u32 table=stage_ref(&a,root+0x48);
        unsigned slots[]={0,1,falco?3:2};MeleeFoxItems* owner=melee_fox_items_decode(&a,root,falco);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_fox_items_entries(owner);assert(!entries[falco?2:3]);
        for(unsigned i=0;i<3;i++){Article* d=entries[slots[i]];assert(d&&d->x0_common_attr&&d->x4_specialAttributes&&d->x10_modelDesc);
            HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);HSD_JObjRemoveAll(joint);}
        melee_fox_items_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Fox/Falco items: laser, blaster and illusion articles, 14 states, source-free model reloads and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--character-models")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));const char* name;u32 root;assert(melee_archive_public(&a,0,&name,&root));
        MeleeCharacterModels* owner=melee_character_models_decode(&a,root);assert(owner);ftData data={0};melee_character_models_bind(owner,&data);
        memset(bytes,0xa5,size);free(bytes);HSD_Joint* descriptors[]={data.x20->x0,data.x5C};unsigned total=0;
        assert(data.x20->x0->child&&isfinite(data.x20->x8));
        for(unsigned i=0;i<2;i++){HSD_JObj* j=HSD_JObjLoadJoint(descriptors[i]);assert(j);Mtx matrix[4096];size_t count=0;matrices(j,matrix,&count);total+=count;
            for(size_t n=0;n<count;n++)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)assert(isfinite(matrix[n][r][c]));HSD_JObjRemoveAll(j);}
        melee_character_models_free(owner);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Character guard/metal models: %u joints, source-free reloads, finite matrices and cleanup passed\n",total);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--fighter-data")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));MeleeFighterData* owner=melee_fighter_data_decode(&a);assert(owner);
        memset(bytes,0xa5,size);free(bytes);void** entries=melee_fighter_data_entries(owner);
        for(unsigned i=0;i<23;i++)assert(entries[i]);
        unsigned indexes[]={16,20};for(unsigned i=0;i<2;i++){HSD_JObj* j=HSD_JObjLoadJoint(entries[indexes[i]]);assert(j);HSD_JObjRemoveAll(j);}
        HSD_JObj* j=HSD_JObjLoadJoint(((void**)entries[8])[0]);assert(j);HSD_JObjAddAnimAll(j,((void**)entries[8])[1],NULL,NULL);HSD_JObjReqAnimAll(j,15);HSD_JObjAnimAll(j);HSD_JObjRemoveAll(j);
        melee_fighter_data_free(owner);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Fighter data: 23 owned entries, model reloads, source disposal and HSD cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--fighter-models")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        MeleeFighterModels* owner=melee_fighter_models_decode(&a);assert(owner);
        memset(bytes,0xa5,size);free(bytes);unsigned total=0;
        for(unsigned i=0;i<3;i++){
            HSD_JObj* joint=HSD_JObjLoadJoint(melee_fighter_models_joint(owner,i));assert(joint);
            if(!i)HSD_JObjAddAnimAll(joint,melee_fighter_models_respawn_animation(owner),NULL,NULL);
            void* first=NULL;size_t first_size=0;unsigned moved=0;
            for(unsigned frame=0;frame<=120;frame+=5){
                HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);Mtx matrix[4096];size_t count=0;matrices(joint,matrix,&count);
                for(size_t n=0;n<count;n++)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)assert(isfinite(matrix[n][r][c]));
                if(!frame){total+=count;first_size=count*sizeof(Mtx);first=malloc(first_size);assert(first);memcpy(first,matrix,first_size);}
                else {assert(count*sizeof(Mtx)==first_size);moved+=memcmp(first,matrix,first_size)!=0;}
            }
            free(first);if(!i)assert(moved);HSD_JObjRemoveAll(joint);
        }
        melee_fighter_models_free(owner);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Fighter models: three owned models, %u joints, respawn animation and finite matrices passed\n",total);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--coin-article")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;assert(melee_archive_find(&a,"itPublicData",&root));
        u32 table=stage_ref(&a,root+8),article=stage_ref(&a,table+4*(It_Kind_Coin-It_Kind_Kuriboh));
        u32 special=stage_ref(&a,article+4),expected[19];
        for(unsigned i=0;i<19;i++)assert(melee_archive_u32(&a,special+4*i,expected+i));
        MeleeItemArticle* owner=melee_item_article_decode(&a,It_Kind_Coin,article,0);assert(owner);
        memset(bytes,0xa5,size);free(bytes);Article* coin=melee_item_article_descriptor(owner);
        assert(coin->x4_specialAttributes&&!memcmp(coin->x4_specialAttributes,expected,sizeof(expected)));
        assert(!coin->xC_itemStates&&coin->x10_modelDesc&&!coin->x10_modelDesc->x0_joint&&!coin->x10_modelDesc->x4_bone_count);
        assert(coin->x8_hurtbones&&coin->x8_hurtbones->count==1);
        melee_item_article_free(owner);puts("Coin article: owned float parameters, root hurtbone and dynamic model schema passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--items-data")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));MeleeItemsData* data=melee_items_data_decode(&a);assert(data);
        it_804D6D20_t* header=melee_items_data_header(data);assert(header->x0&&header->x4&&header->x8&&header->xC&&header->x10&&header->x14);
        u32 root;assert(melee_archive_find(&a,"itPublicData",&root));u32 enemy=stage_ref(&a,root+16);
        for(unsigned i=0;i<7;i++){u32 raw,native;assert(melee_archive_u32(&a,enemy+4*i,&raw));memcpy(&native,(u8*)header->x10+4*i,4);assert(raw==native);}
        u32 chars=stage_ref(&a,root+8);
        u32 goomba_at=stage_ref(&a,chars),goomba_sp=stage_ref(&a,goomba_at+4),goomba_cp=stage_ref(&a,goomba_sp),goomba_words[5];
        for(unsigned i=0;i<2;i++)assert(melee_archive_u32(&a,goomba_cp+4*i,&goomba_words[i]));
        for(unsigned i=0;i<3;i++)assert(melee_archive_u32(&a,goomba_sp+4+4*i,&goomba_words[2+i]));
        u32 maze_common[2][5],maze_special[2][8];
        const unsigned maze_kinds[]={It_Kind_Leadead,It_Kind_Octarock,It_Kind_Octarock_Stone,It_Kind_Ottosea},maze_counts[]={10,5,1,7};
        for(unsigned i=0;i<2;i++){
            u32 at=stage_ref(&a,chars+4*(maze_kinds[i]-It_Kind_Kuriboh)),sp=stage_ref(&a,at+4),cp=stage_ref(&a,sp);
            for(unsigned j=0;j<5;j++)assert(melee_archive_u32(&a,cp+4*j,&maze_common[i][j]));
            for(unsigned j=0;j<8;j++)assert(melee_archive_u32(&a,sp+4*j,&maze_special[i][j]));
        }
        u32 coin_at=stage_ref(&a,chars+4*(It_Kind_Coin-It_Kind_Kuriboh));
        u32 coin_special=stage_ref(&a,coin_at+4),coin_words[19];
        for(unsigned i=0;i<19;i++)assert(melee_archive_u32(&a,coin_special+4*i,coin_words+i));
        u32 match_coin_at=stage_ref(&a,chars+4*(It_Kind_Unk4-It_Kind_Kuriboh));
        u32 match_coin_special=stage_ref(&a,match_coin_at+4),match_coin_words[48];
        for(unsigned i=0;i<48;i++)assert(melee_archive_u32(&a,match_coin_special+4*i,match_coin_words+i));
        u32 otto_at=stage_ref(&a,chars+4*(It_Kind_Ottosea-It_Kind_Kuriboh)),otto_sp=stage_ref(&a,otto_at+4),otto_cp=stage_ref(&a,otto_sp),otto_common[2],otto_words[11];
        for(unsigned i=0;i<2;i++)assert(melee_archive_u32(&a,otto_cp+4*i,otto_common+i));
        for(unsigned i=0;i<11;i++)assert(melee_archive_u32(&a,otto_sp+4*i,otto_words+i));
        memset(bytes,0xa5,size);free(bytes);
        Article* goomba=melee_items_data_article(data,It_Kind_Kuriboh);assert(goomba&&goomba==header->x8[0]);
        u8* goomba_special=goomba->x4_specialAttributes;assert(goomba_special);
        assert(!memcmp(*(void**)goomba_special,goomba_words,8)&&!memcmp(goomba_special+sizeof(void*),goomba_words+2,12));
        HSD_JObj* goomba_joint=HSD_JObjLoadJoint(goomba->x10_modelDesc->x0_joint);assert(goomba_joint);HSD_JObjRemoveAll(goomba_joint);
        for(unsigned i=0;i<4;i++){
            Article* d=melee_items_data_article(data,maze_kinds[i]);assert(d);
            if(i<2){
                assert(d->x4_specialAttributes&&!memcmp(*(void**)d->x4_specialAttributes,maze_common[i],20));
                if(i==0){
                    itLeadeadAttributes* v=d->x4_specialAttributes;assert(!memcmp(&v->x4,maze_special[i]+1,20));
                    assert((u16)v->x18==(maze_special[i][6]>>16)&&(u16)v->x1A==(maze_special[i][6]&65535)&&(u16)v->x1C==(maze_special[i][7]>>16)&&(u8)v->x1E==((maze_special[i][7]>>8)&255));
                }else{itOctarockAttributes* v=d->x4_specialAttributes;assert(!memcmp(&v->x4,maze_special[i]+1,24)&&(u16)v->x1C==(maze_special[i][7]>>16));}
            }else if(i==2)assert(!d->x4_specialAttributes);
            else{
                itOldottoseaAttributes* v=d->x4_specialAttributes;assert(v&&!memcmp(v->x0,otto_common,8));
                assert(!memcmp(&v->x4,otto_words+1,12)&&!memcmp(&v->x14,otto_words+5,20));
                assert((u8)v->x10==(otto_words[4]>>24)&&(u8)v->x28==(otto_words[10]>>24));
            }
            for(unsigned j=0;j<maze_counts[i];j++){
                HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);
                ItemStateDesc* anim=d->xC_itemStates->x0_itemStateDesc+j;
                HSD_JObjAddAnimAll(joint,anim->x0_anim_joint,anim->x4_matanim_joint,anim->x8_parameters);
                for(unsigned frame=0;frame<120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);
            }
        }
        Article* coin=melee_items_data_article(data,It_Kind_Coin);
        Article* match_coin=melee_items_data_article(data,It_Kind_Unk4);assert(match_coin);
        it_2E5A_Attrs* match_attrs=match_coin->x4_specialAttributes;assert(match_attrs&&!memcmp(match_attrs,match_coin_words,60));
        for(unsigned i=0;i<3;i++){
            it_2E5A_TierEntry* tier=&match_attrs->tiers[i];
            assert(!memcmp(&tier->xD84_value,match_coin_words+15+11*i+4,28));
            assert(tier->native_state&&tier->native_state->x0_anim_joint==tier->anim_joint&&!tier->native_state->xC_script);
            HSD_JObj* joint=HSD_JObjLoadJoint(tier->joint);assert(joint);
            HSD_JObjAddAnimAll(joint,tier->anim_joint,tier->matanim_joint,tier->shape_anim_joint);
            for(unsigned frame=0;frame<120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
            HSD_JObjRemoveAll(joint);
        }
        assert(coin&&coin==header->x8[It_Kind_Coin-It_Kind_Kuriboh]);
        assert(coin->x4_specialAttributes&&!memcmp(coin->x4_specialAttributes,coin_words,sizeof(coin_words)));
        assert(coin->x10_modelDesc&&!coin->x10_modelDesc->x0_joint&&!coin->x10_modelDesc->x4_bone_count);
        for(unsigned i=0;i<43;i++)assert(melee_items_data_article(data,i)==header->x4[i]&&header->x4[i]);
        /* Every ordinary weighted Poké Ball outcome must have a native article. */
        itPokemonSpawn_DatAttrs* spawn_attrs=header->x4[It_Kind_M_Ball]->x4_specialAttributes;
        for(unsigned p=0;p<It_PKind_Terminate-It_PKind_Start;p++){
            if(spawn_attrs->pokemon_spawn_weights[p]>0)
                assert(melee_items_data_article(data,It_PKind_Start+p));
        }
        const unsigned pokemon_kinds[]={It_PKind_Tosakinto,It_PKind_Chicorita,It_PKind_Kabigon,It_Kind_Chicorita_Leaf,It_PKind_Hassam,It_PKind_Kamex,It_Kind_Kamex_HydroPump,It_PKind_Matadogas,It_Kind_Matadogas_Gas1,It_Kind_Matadogas_Gas2,It_PKind_Lizardon,It_Kind_Lizardon_Flame1,It_Kind_Lizardon_Flame2,It_Kind_Lizardon_Flame3,It_Kind_Lizardon_Flame4,It_PKind_Fire,It_PKind_Thunder,It_PKind_Freezer,It_PKind_Sonans,It_PKind_Kireihana,It_PKind_Entei,It_PKind_Raikou,It_PKind_Suikun,It_PKind_Marumine,It_PKind_Unknown,It_Kind_Unknown_Swarm,It_PKind_Lugia,It_Kind_Lugia_Aeroblast,It_Kind_Lugia_Aeroblast2,It_Kind_Lugia_Aeroblast3,It_PKind_Houou,It_Kind_Houou_SacredFire,It_PKind_Mew,It_PKind_Cerebi,It_PKind_Hinoarashi,It_Kind_Hinoarashi_Flame,It_PKind_Maril,It_PKind_Fushigibana,It_PKind_Hitodeman,It_Kind_Hitodeman_Star,It_PKind_Lucky,It_Kind_Lucky_Egg,It_PKind_Porygon2,It_PKind_Pippi,It_PKind_Togepy};
        const unsigned pokemon_counts[]={3,2,2,1,3,3,1,2,1,1,4,1,1,1,1,3,3,3,2,5,1,1,1,7,2,1,6,1,1,1,6,1,3,3,2,1,1,2,2,1,4,0,2,6,7};
        for(unsigned p=0;p<sizeof(pokemon_kinds)/sizeof(*pokemon_kinds);p++){
        Article* goldeen=melee_items_data_article(data,pokemon_kinds[p]);
        assert(goldeen&&goldeen==header->xC[pokemon_kinds[p]-It_PKind_Start]);
        assert((goldeen->x4_specialAttributes==NULL)==(pokemon_kinds[p]==It_PKind_Porygon2));
        if(pokemon_kinds[p]==It_Kind_Hinoarashi_Flame||pokemon_kinds[p]==It_Kind_Matadogas_Gas1||pokemon_kinds[p]==It_Kind_Matadogas_Gas2||(pokemon_kinds[p]>=It_Kind_Lizardon_Flame1&&pokemon_kinds[p]<=It_Kind_Lizardon_Flame4)||(pokemon_kinds[p]>=It_Kind_Lugia_Aeroblast&&pokemon_kinds[p]<=It_Kind_Lugia_Aeroblast3)){
            /* These projectiles are rendered by spawned effects. */
            assert(goldeen->x10_modelDesc&&!goldeen->x10_modelDesc->x0_joint);
            assert(!goldeen->x10_modelDesc->x4_bone_count);
            ItemStateDesc* state=goldeen->xC_itemStates->x0_itemStateDesc;
            assert(!state->x0_anim_joint&&!state->x4_matanim_joint&&!state->x8_parameters);
            continue;
        }
        if(pokemon_kinds[p]==It_PKind_Unknown||pokemon_kinds[p]==It_Kind_Unknown_Swarm){
            assert(goldeen->x10_modelDesc&&!goldeen->x10_modelDesc->x0_joint);
            assert(!goldeen->x10_modelDesc->x4_bone_count);
            itUnknownAttributes* attrs=goldeen->x4_specialAttributes;
            for(unsigned v=0;v<26;v++){
                HSD_JObj* variant=HSD_JObjLoadJoint(attrs->x24[v]);assert(variant);
                for(unsigned state=0;state<pokemon_counts[p];state++){
                    ItemStateDesc* anim=goldeen->xC_itemStates->x0_itemStateDesc+state;
                    HSD_JObjAddAnimAll(variant,anim->x0_anim_joint,anim->x4_matanim_joint,anim->x8_parameters);
                    for(unsigned frame=0;frame<120;frame++){HSD_JObjReqAnimAll(variant,frame);HSD_JObjAnimAll(variant);}
                }
                HSD_JObjRemoveAll(variant);
            }
            continue;
        }
        if(pokemon_kinds[p]==It_Kind_Lucky_Egg){
            HSD_JObj* joint=HSD_JObjLoadJoint(goldeen->x10_modelDesc->x0_joint);assert(joint);
            HSD_JObjRemoveAll(joint);
        }
        for(unsigned state=0;state<pokemon_counts[p];state++){
            HSD_JObj* joint=HSD_JObjLoadJoint(goldeen->x10_modelDesc->x0_joint);assert(joint);
            ItemStateDesc* animation=goldeen->xC_itemStates->x0_itemStateDesc+state;
            HSD_JObjAddAnimAll(joint,animation->x0_anim_joint,animation->x4_matanim_joint,animation->x8_parameters);
            for(unsigned frame=0;frame<120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
            HSD_JObjRemoveAll(joint);
        }
        }
        assert(melee_items_data_article(data,It_Kind_Ottosea));Article registered={0};header->x8[5]=&registered;assert(melee_items_data_article(data,48)==&registered);
        melee_items_data_free(data);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Items data: common articles, Adventure enemies including Ottosea, auxiliary parameters, color bank, deferred registration slots and owned lifetime passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--whitebear-article")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;
        assert(melee_archive_find(&a,"itemdata",&root));u32 entry=stage_ref(&a,root),kind;
        assert(melee_archive_u32(&a,entry,&kind)&&kind==It_Kind_Whitebea);
        MeleeItemArticle* owner=melee_item_article_decode(&a,kind,stage_ref(&a,entry+4),8);assert(owner);
        memset(bytes,0xa5,size);free(bytes);Article* article=melee_item_article_descriptor(owner);
        itWhiteBeaAttributes* attrs=article->x4_specialAttributes;
        assert(attrs&&attrs->x0&&attrs->x0->x0==25&&fabsf(attrs->x0->x4-0.3f)<0.00001f);
        assert(attrs->x4==0.5f&&attrs->x8==120&&attrs->xA==320&&attrs->xC==240&&attrs->xE==480&&attrs->x14==20);
        for(unsigned i=0;i<8;i++){
            ItemStateDesc* d=&article->xC_itemStates->x0_itemStateDesc[i];
            HSD_JObj* j=HSD_JObjLoadJoint(article->x10_modelDesc->x0_joint);assert(j);
            HSD_JObjAddAnimAll(j,d->x0_anim_joint,d->x4_matanim_joint,d->x8_parameters);
            HSD_JObjReqAnimAll(j,15);HSD_JObjAnimAll(j);Mtx matrix[4096];size_t n=0;matrices(j,matrix,&n);
            for(size_t k=0;k<n;k++)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)assert(isfinite(matrix[k][r][c]));
            HSD_JObjRemoveAll(j);
        }
        melee_item_article_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Polar Bear article: owned mixed-width attributes, eight animations and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--tools-article")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;
        assert(melee_archive_find(&a,"itemdata",&root));u32 entry=stage_ref(&a,root),kind;
        assert(melee_archive_u32(&a,entry,&kind)&&kind==It_Kind_Tools);
        MeleeItemArticle* owner=melee_item_article_decode(&a,kind,stage_ref(&a,entry+4),10);assert(owner);
        u32 special=stage_ref(&a,stage_ref(&a,entry+4)+4),expected[39];
        for(unsigned i=0;i<39;i++)assert(melee_archive_u32(&a,special+4*i,&expected[i]));
        MeleeArchive truncated=a;truncated.data_size=special+155;
        assert(!melee_item_special_decode(&truncated,kind,special));
        u8 saved[4];memcpy(saved,bytes+32+special,4);
        bytes[32+special]=0x7f;bytes[33+special]=0x80;bytes[34+special]=bytes[35+special]=0;
        assert(!melee_item_special_decode(&a,kind,special));memcpy(bytes+32+special,saved,4);
        memset(bytes,0xa5,size);free(bytes);Article* article=melee_item_article_descriptor(owner);
        itToolsAttributes* attrs=article->x4_specialAttributes;assert(attrs);
        assert(attrs->x0==180&&attrs->x4==30&&attrs->x8==2&&attrs->xC==50);
        for(unsigned i=0;i<39;i++){u32 actual;memcpy(&actual,(u8*)attrs+4*i,4);assert(actual==expected[i]);}
        for(unsigned i=0;i<10;i++){
            ItemStateDesc* d=&article->xC_itemStates->x0_itemStateDesc[i];
            HSD_JObj* j=HSD_JObjLoadJoint(article->x10_modelDesc->x0_joint);assert(j);
            HSD_JObjAddAnimAll(j,d->x0_anim_joint,d->x4_matanim_joint,d->x8_parameters);
            HSD_JObjReqAnimAll(j,15);HSD_JObjAnimAll(j);Mtx matrix[4096];size_t n=0;matrices(j,matrix,&n);
            for(size_t k=0;k<n;k++)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)assert(isfinite(matrix[k][r][c]));
            HSD_JObjRemoveAll(j);
        }
        melee_item_article_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Flat Zone tools: all five motion records, ten animations, malformed attributes and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--item-articles")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;assert(melee_archive_find(&a,"itPublicData",&root));u32 table=stage_ref(&a,root+4);unsigned total=0;
        for(unsigned kind=0;kind<43;kind++){
            u32 at=stage_ref(&a,table+4*kind),states=stage_ref(&a,at+12);unsigned count=states==UINT32_MAX?0:(at-states)/16;
            assert(!count||(at>states&&(at-states)%16==0&&count<=16));
            MeleeItemArticle* owner=melee_item_article_decode(&a,kind,at,count);if(!owner){fprintf(stderr,"Article %u failed\n",kind);return 2;}
            Article* article=melee_item_article_descriptor(owner);assert(article->x0_common_attr&&article->x4_specialAttributes&&article->x10_modelDesc);
            u8* saved=malloc(size);assert(saved);memcpy(saved,bytes,size);memset(bytes,0xa5,size);
            for(unsigned i=0;i<count;i++){
                ItemStateDesc* d=&article->xC_itemStates->x0_itemStateDesc[i];
                if(article->x10_modelDesc->x0_joint){
                    HSD_JObj* j=HSD_JObjLoadJoint(article->x10_modelDesc->x0_joint);assert(j);HSD_JObjAddAnimAll(j,d->x0_anim_joint,d->x4_matanim_joint,d->x8_parameters);
                    HSD_JObjReqAnimAll(j,15);HSD_JObjAnimAll(j);Mtx matrix[4096];size_t n=0;matrices(j,matrix,&n);
                    for(size_t k=0;k<n;k++)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)assert(isfinite(matrix[k][r][c]));HSD_JObjRemoveAll(j);
                }
                if(d->xC_script)assert(((union CmdUnion*)d->xC_script)->unk0.opcode<=25);total++;
            }
            melee_item_article_free(owner);memcpy(bytes,saved,size);free(saved);
        }
        free(bytes);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_RObjGetAllocData()->used&&!HSD_RvalueObjGetAllocData()->used);
        printf("Common articles: 43 assembled, %u states reloaded, source disposal and HSD cleanup passed\n",total);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--item-mushrooms")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;assert(melee_archive_find(&a,"itPublicData",&root));u32 table=stage_ref(&a,root+4);
        MeleeItemMushroom* owners[2]={0};
        for(unsigned i=0;i<2;i++){
            u32 article=stage_ref(&a,table+(26+i)*4),at=stage_ref(&a,article+4),model=stage_ref(&a,article+16),joint=stage_ref(&a,model);
            owners[i]=melee_item_mushroom_decode(&a,at,joint);assert(owners[i]);KinokoAttrs* attrs=melee_item_mushroom_attributes(owners[i]);
            float x,y;assert(melee_archive_f32(&a,at,&x)&&melee_archive_f32(&a,at+4,&y)&&attrs->x0==x&&attrs->x4==y);
            u32 saved;assert(melee_archive_u32(&a,at,&saved));word(bytes+32+at,0x7fc00000);assert(!melee_item_mushroom_decode(&a,at,joint));word(bytes+32+at,saved);
            MeleeArchive truncated=a;truncated.data_size=at+15;assert(!melee_item_mushroom_decode(&truncated,at,joint));
        }
        memset(bytes,0xa5,size);free(bytes);
        /* Original fighter consumer creates one ordinary joint for scale animation. */
        HSD_Joint desc={0};desc.flags=8;desc.scale.x=desc.scale.y=desc.scale.z=1;
        for(unsigned i=0;i<2;i++)for(unsigned k=0;k<2;k++){
            HSD_JObj* instance=HSD_JObjLoadJoint(&desc);assert(instance);KinokoAttrs* attrs=melee_item_mushroom_attributes(owners[i]);
            HSD_JObjAddAnim(instance,attrs->animations[k],NULL,NULL);Mtx first;unsigned changed=0;
            for(unsigned frame=0;frame<=120;frame+=5){
                HSD_JObjReqAnimAll(instance,frame);HSD_JObjAnimAll(instance);HSD_JObjSetupMatrix(instance);
                for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)assert(isfinite(instance->mtx[r][c]));
                if(!frame)memcpy(first,instance->mtx,sizeof(Mtx));else changed+=memcmp(first,instance->mtx,sizeof(Mtx))!=0;
            }
            assert(changed);HSD_JObjRemoveAll(instance);
        }
        for(unsigned i=0;i<2;i++)melee_item_mushroom_free(owners[i]);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Mushrooms: both attribute records, four owned animation bindings, scalar values and motion passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--item-wstar")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;assert(melee_archive_find(&a,"itPublicData",&root));
        u32 table=stage_ref(&a,root+4),article=stage_ref(&a,table+29*4),at=stage_ref(&a,article+4),model=stage_ref(&a,article+16),joint=stage_ref(&a,model);
        MeleeItemModel* model_owner=melee_item_model_decode(&a,model);assert(model_owner);
        MeleeItemWstar* w=melee_item_wstar_decode(&a,at,joint);assert(w);itWstarAttributes* attrs=melee_item_wstar_attributes(w);assert(attrs->x24_count==7);
        for(unsigned i=0;i<9;i++){float v,actual;assert(melee_archive_f32(&a,at+4*i,&v));memcpy(&actual,(u8*)attrs+4*i,4);assert(v==actual);}
        for(unsigned i=0;i<7;i++){u32 sfx,actual;assert(melee_archive_u32(&a,at+44+8*i,&sfx));memcpy(&actual,&attrs->x28_entries[i].x4_sfx,4);assert(actual==sfx);}
        word(bytes+32+at+36,1);assert(!melee_item_wstar_decode(&a,at,joint));word(bytes+32+at+36,8);assert(!melee_item_wstar_decode(&a,at,joint));word(bytes+32+at+36,7);
        u32 saved;assert(melee_archive_u32(&a,at+4,&saved));word(bytes+32+at+4,0);assert(!melee_item_wstar_decode(&a,at,joint));word(bytes+32+at+4,saved);
        memset(bytes,0xa5,size);free(bytes);
        for(unsigned i=0;i<7;i++){
            HSD_JObj* instance=HSD_JObjLoadJoint(melee_item_model_descriptor(model_owner)->x0_joint);assert(instance);
            HSD_JObjAddAnim(instance,attrs->x28_entries[i].x0_anim_joint,NULL,NULL);
            Mtx first[4096],current[4096];size_t initial=0;unsigned changed=0;
            for(unsigned frame=0;frame<=120;frame+=10){
                HSD_JObjReqAnimAll(instance,frame);HSD_JObjAnimAll(instance);size_t n=0;matrices(instance,current,&n);
                for(size_t k=0;k<n;k++)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)assert(isfinite(current[k][r][c]));
                if(!frame){initial=n;memcpy(first,current,n*sizeof(Mtx));}else{assert(n==initial);changed+=memcmp(first,current,n*sizeof(Mtx))!=0;}
            }
            assert(changed);HSD_JObjRemoveAll(instance);
        }
        melee_item_wstar_free(w);melee_item_model_free(model_owner);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_MtxGetAllocData()->used&&!HSD_VecGetAllocData()->used);puts("Warp Star: seven owned animation/sound variants, sampled motion, scalar values and cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--item-foods")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;assert(melee_archive_find(&a,"itPublicData",&root));
        u32 table=stage_ref(&a,root+4),article=stage_ref(&a,table+18*4),at=stage_ref(&a,article+4);
        MeleeItemFoods* foods=melee_item_foods_decode(&a,at);assert(foods);itFoodsNativeAttributes* attrs=melee_item_foods_attributes(foods);
        u32 count;assert(melee_archive_u32(&a,at,&count)&&count==28&&attrs->count==count);
        for(unsigned i=0;i<count;i++){
            u32 heal;float x,y;assert(melee_archive_u32(&a,at+8+16*i,&heal));assert(melee_archive_f32(&a,at+12+16*i,&x)&&melee_archive_f32(&a,at+16+16*i,&y));
            assert(attrs->entries[i].heal_amount==heal&&attrs->entries[i].offset_x==x&&attrs->entries[i].offset_y==y);
        }
        u32 saved;assert(melee_archive_u32(&a,at+12,&saved));word(bytes+32+at+12,0x7fc00000);assert(!melee_item_foods_decode(&a,at));word(bytes+32+at+12,saved);
        word(bytes+32+at,0);assert(!melee_item_foods_decode(&a,at));word(bytes+32+at,257);assert(!melee_item_foods_decode(&a,at));word(bytes+32+at,count);
        MeleeArchive truncated=a;truncated.data_size=at+4+count*16-1;assert(!melee_item_foods_decode(&truncated,at));
        memset(bytes,0xa5,size);free(bytes);
        for(unsigned i=0;i<count;i++){
            HSD_JObj* instance=HSD_JObjLoadJoint(attrs->entries[i].joint);assert(instance);Mtx result[4096];size_t n=0;matrices(instance,result,&n);
            for(size_t k=0;k<n;k++)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)assert(isfinite(result[k][r][c]));
            HSD_JObjRemoveAll(instance);
        }
        melee_item_foods_free(foods);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_MtxGetAllocData()->used&&!HSD_VecGetAllocData()->used);puts("Food attributes: 28 models, healing, offsets, owned lifetime and HSD cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--item-animations")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive original,a;assert(melee_archive_open(&original,bytes,size));
        size_t resolved_size;u8* resolved=melee_archive_copy_null_externals(&original,&resolved_size);assert(resolved);
        assert(melee_archive_open(&a,resolved,resolved_size)&&!a.extern_count);free(bytes);
        u32 root;assert(melee_archive_find(&a,"itPublicData",&root));u32 table=stage_ref(&a,root+4);
        unsigned loaded=0,failed=0,animated=0;
        for(unsigned i=0;i<43;i++){
            u32 article=stage_ref(&a,table+4*i),states=stage_ref(&a,article+12),model=stage_ref(&a,article+16);
            u32 joint=stage_ref(&a,model);if(states==UINT32_MAX)continue;
            /* Retail fixture layout: state table immediately precedes Article.
             * This is not a production rule for determining state counts. */
            assert(article>states&&(article-states)%16==0&&(article-states)/16<=16);
            for(unsigned k=0;k<(article-states)/16;k++){
                u32 at=states+16*k;unsigned has=0;for(unsigned n=0;n<3;n++)has+=stage_ref(&a,at+4*n)!=UINT32_MAX;
                if(joint==UINT32_MAX){assert(!has);loaded++;continue;}
                MeleeScene* scene=melee_item_animation_decode(&a,joint,at);
                if(!scene){fprintf(stderr,"Item animation %u/%u unsupported\n",i,k);failed++;continue;}
                /* Prove animation tracks and graph descriptors own their bytes. */
                u8* saved=malloc(resolved_size);assert(saved);memcpy(saved,resolved,resolved_size);memset(resolved,0xa5,resolved_size);
                HSD_Joint* descriptor=melee_scene_joint_descriptor(scene);
                HSD_AnimJoint* ja=melee_scene_animation_descriptor(scene);HSD_MatAnimJoint* ma=melee_scene_material_descriptor(scene);HSD_ShapeAnimJoint* sa=melee_scene_shape_descriptor(scene);
                melee_scene_release_objects(scene);HSD_JObj* instance=HSD_JObjLoadJoint(descriptor);assert(instance);HSD_JObjAddAnimAll(instance,ja,ma,sa);
                for(unsigned frame=0;frame<=60;frame+=10){
                    HSD_JObjReqAnimAll(instance,frame);HSD_JObjAnimAll(instance);
                    Mtx result[4096];size_t count=0;matrices(instance,result,&count);
                    for(size_t n=0;n<count;n++)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)assert(isfinite(result[n][r][c]));
                }
                HSD_JObjRemoveAll(instance);memcpy(resolved,saved,resolved_size);free(saved);
                animated+=!!has;loaded++;melee_scene_free(scene);
            }
        }
        free(resolved);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_RObjGetAllocData()->used&&!HSD_RvalueObjGetAllocData()->used);
        printf("Item states: %u decoded (%u animated), %u unsupported\n",loaded,animated,failed);return failed?2:0;
    }
    if(argc==3&&!strcmp(argv[1],"--item-models")){
        u8 negate[]={6,0x3f,0xc0,0,0,9,1};assert(HSD_ByteCodeEval(negate,NULL,0)==-1.5f);
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;assert(melee_archive_find(&a,"itPublicData",&root));
        u32 table=stage_ref(&a,root+4);assert(table!=UINT32_MAX);
        MeleeItemModel* models[43]={0};unsigned loaded=0;
        for(unsigned i=0;i<43;i++){
            u32 article=stage_ref(&a,table+4*i);assert(article!=UINT32_MAX);u32 at=stage_ref(&a,article+16);assert(at!=UINT32_MAX);
            models[i]=melee_item_model_decode(&a,at);if(!models[i]){fprintf(stderr,"Item model %u unsupported\n",i);continue;}
            ItemModelDesc* m=melee_item_model_descriptor(models[i]);u32 expected,actual;
            assert(melee_archive_u32(&a,at+4,&expected)&&m->x4_bone_count==expected);
            assert(melee_archive_u32(&a,at+8,&expected));memcpy(&actual,&m->x8_bone_attach_id,4);assert(actual==expected);
            assert(m->xC_bit_field==bytes[32+at+12]);loaded++;
        }
        memset(bytes,0xa5,size);free(bytes);
        for(unsigned i=0;i<43;i++)if(models[i]){
            ItemModelDesc* m=melee_item_model_descriptor(models[i]);
            if(m->x0_joint){HSD_JObj* joint=HSD_JObjLoadJoint(m->x0_joint);assert(joint);unsigned copies=check_joint_copies(joint);assert(copies==(i==25?16:0));Mtx result[4096];size_t count=0;matrices(joint,result,&count);
                for(size_t k=0;k<count;k++)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)assert(isfinite(result[k][r][c]));
                HSD_JObjRemoveAll(joint);}
            melee_item_model_free(models[i]);
        }
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_MtxGetAllocData()->used&&!HSD_VecGetAllocData()->used);
        assert(!HSD_RObjGetAllocData()->used&&!HSD_RvalueObjGetAllocData()->used);
        printf("Common item models: %u/43 decoded; metadata, owned descriptor lifetime and HSD cleanup passed\n",loaded);return loaded==43?0:2;
    }
    if(argc==3&&!strcmp(argv[1],"--final-archive")){
        StageCallbacks cb={0};cb.flags=0x80000000;assert(cb.flags_b0&&!cb.flags_b1);
        cb.flags=0x40000000;assert(!cb.flags_b0&&cb.flags_b1);
        cb.flags=0;cb.flags_b7=1;assert(cb.flags==0x01000000);

        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));HSD_Archive* bridge=melee_final_stage_decode(&a);assert(bridge);
        memset(bytes,0xa5,size);free(bytes);
        const char* symbols[]={"map_head","coll_data","grGroundParam","yakumono_param","itemdata","ALDYakuAll","quake_model_set","map_plit","map_ptcl","map_texg"};
        for(unsigned i=0;i<10;i++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[i]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"missing"));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");LightList** lights=HSD_ArchiveGetPublicAddress(bridge,"map_plit");
        for(int m=0;m<head->unkC;m++)for(LightList** l=head->unk8[m].x18;l&&*l;l++)for(HSD_LightAnim** a=(*l)->anims;a&&*a;a++){
            int found=0;for(int k=0;k<head->unk24;k++)if(head->unk20[k].unk0==*a)found=1;
            assert(found);
        }
        for(unsigned i=0;lights&&lights[i];i++)for(unsigned j=0;lights[i]->anims&&lights[i]->anims[j];j++){
            int found=0;for(int k=0;k<head->unk24;k++)if(head->unk20[k].unk0==lights[i]->anims[j])found=1;
            assert(found);
        }
        LightOverrideEntry* overrides=head->unk18;
        for(unsigned i=0;lights[i];i++){int found=0;for(int j=0;j<head->unk1C;j++)if(lights[i]->desc==overrides[j].desc)found=1;assert(found);}
        union ColorOverlay_x8_t** colors=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");
        for(unsigned i=0;i<4;i++){
            assert(colors[i]&&colors[i][0].unk.unk==18&&colors[i][2].unk.unk==19);
            assert(colors[i][2].unk.timer==(i<2?120:60)&&colors[i][4].unk.unk==11);
            assert(colors[i][1].light_color.a==(i%2?255:0)&&colors[i][3].light_color.a==(i%2?0:255));
            assert(colors[i][i<2?5:6].unk.unk==10);
        }
        assert(head->unk14==2&&head->unk24==3);
        bridge->native_destroy(bridge);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_MtxGetAllocData()->used&&!HSD_VecGetAllocData()->used);
        puts("Final Destination archive: ten native symbols, particle-light identity, source disposal and HSD cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--greens-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[31];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<31;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        HSD_Archive* bridge=melee_greens_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        assert(!memcmp(HSD_ArchiveGetPublicAddress(bridge,"yakumono_param"),expected,sizeof(expected)));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==7);
        assert(head->unk8[0].x1C&&head->unk8[0].x1C->start==2000&&head->unk8[0].x1C->end==2000);
        struct GroundItemData** items=HSD_ArchiveGetPublicAddress(bridge,"itemdata");assert(items&&items[0]&&items[1]&&!items[2]);
        for(unsigned i=0;i<2;i++){
            assert(items[i]->unk0==(i?It_Kind_WhispyHealApple:It_Kind_WhispyApple));
            Article* article=items[i]->unk4;assert(article&&article->x10_modelDesc&&article->xC_itemStates);
            itWhispyAppleAttributes* attrs=article->x4_specialAttributes;
            assert(attrs&&attrs->common==attrs->owned_common&&attrs->common[0]==10&&attrs->common[4]==100);
            assert(attrs->x4==8&&attrs->x8==540&&attrs->x14==3&&attrs->x18==0.5f);
        }
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit","map_ptcl","map_texg"};
        for(unsigned j=0;j<7;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Green Greens archive: owned apple articles, hazard parameters and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--corneria-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[33];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<33;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        HSD_Archive* bridge=melee_corneria_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        struct grCorneria_YakumonoParam* params=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");
        assert(params&&!memcmp(params,expected,132)&&params->x84);
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==11);
        struct GroundItemData** items=HSD_ArchiveGetPublicAddress(bridge,"itemdata");assert(items&&items[0]&&items[1]&&!items[2]&&items[0]->unk0==It_Kind_Arwing_Laser);
        Article* article=items[0]->unk4;assert(article&&article->xC_itemStates);
        ArwingLaserAttr* attrs=article->x4_specialAttributes;
        assert(attrs&&attrs->x0&&attrs->x0->x4_throw_speed_mul==4&&attrs->x4==5&&attrs->x8==6);
        void** text=HSD_ArchiveGetPublicAddress(bridge,"SIS_GrCorneriaData");assert(text&&text[0]&&text[52]);
        assert(items[1]->unk0==It_Kind_GreatFox_Laser);
        Article* greatfox=items[1]->unk4;assert(greatfox&&greatfox->xC_itemStates);
        itGreatFoxLaser_Attrs* laser=greatfox->x4_specialAttributes;
        assert(laser&&laser->x0==50&&laser->x4==20&&laser->x8==10&&laser->xC==20);
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Corneria archive: owned laser articles, color script, text and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--venom-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[14];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<14;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        HSD_Archive* bridge=melee_venom_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        struct grVenom_YakumonoParam* params=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");
        assert(params&&!memcmp(params,expected,56)&&params->x38);
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==8);
        struct GroundItemData** items=HSD_ArchiveGetPublicAddress(bridge,"itemdata");assert(items&&items[0]&&!items[1]&&items[0]->unk0==It_Kind_Arwing_Laser);
        Article* article=items[0]->unk4;assert(article&&article->xC_itemStates);
        ArwingLaserAttr* attrs=article->x4_specialAttributes;
        assert(attrs&&attrs->x0&&attrs->x0->x4_throw_speed_mul==4&&attrs->x4==5&&attrs->x8==6);
        void** text=HSD_ArchiveGetPublicAddress(bridge,"SIS_GrCorneriaData");assert(text&&text[0]&&text[52]);
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&!HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Venom archive: owned laser article, color script, text and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--mutecity-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[20];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<20;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        HSD_Archive* bridge=melee_mutecity_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        struct grMc_YakumonoParam* params=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");
        assert(params&&params->x0&&params->x4&&params->x8&&params->xC);
        assert(!memcmp(params->pad10,expected+4,64));
        assert(((u32*)params->x8)[0]==1&&((u32*)params->x8)[1]==8);
        assert(((u32*)params->xC)[0]==1&&((u32*)params->xC)[1]==18);
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==39);
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&!HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Mute City archive: owned color scripts, hit records, absent particle bank and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--icemt-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;
        assert(melee_archive_find(&a,"yakumono_param",&root));u8 expected[0xac];
        for(unsigned i=0;i<0xac/4;i++){u32 v;assert(melee_archive_u32(&a,root+4*i,&v));memcpy(expected+4*i,&v,4);}
        const unsigned ranges[][2]={{0,4},{0x34,4},{0x98,2},{0xa4,4}};
        for(unsigned i=0;i<4;i++)for(unsigned j=0;j<ranges[i][1];j++){
            unsigned off=ranges[i][0]+2*j;const u8* q=bytes+32+root+off;u16 v=((u16)q[0]<<8)|q[1];memcpy(expected+off,&v,2);
        }
        s16 tables[40];unsigned used=0;const unsigned counts[]={16,12,12};
        for(unsigned i=0;i<3;i++){
            u32 at=stage_ref(&a,root+0xac+4*i);
            for(unsigned j=0;j<counts[i];j++){const u8* q=bytes+32+at+2*j;u16 v=((u16)q[0]<<8)|q[1];memcpy(tables+used++,&v,2);}
        }
        grZakoGenerator_SpawnDesc spawn[32];
        for(unsigned i=0;i<32;i++){const u8* q=bytes+32+root+0xbc+4*i;spawn[i].kind=((u16)q[0]<<8)|q[1];spawn[i].x2=q[2];spawn[i].respawn=q[3];}
        HSD_Archive* bridge=melee_icemt_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        struct grIceMt_YakumonoParam* p=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");
        assert(p&&!memcmp(p,expected,sizeof(expected))&&p->xB8==1);
        assert(!memcmp(p->field_ixs,tables,32)&&!memcmp(p->xB0,tables+16,24)&&!memcmp(p->xB4,tables+28,24));
        assert(!memcmp(p->spawn,spawn,sizeof(spawn))&&p->spawn[15].kind==It_Kind_Whitebea);
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==9);
        struct GroundItemData** items=HSD_ArchiveGetPublicAddress(bridge,"itemdata");assert(items&&items[0]&&items[0]->unk0==It_Kind_Whitebea&&!items[1]);
        assert(HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        bridge->native_destroy(bridge);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Icicle Mountain archive: mixed-width parameters, three owned tables, full spawn block and Polar Bear passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--fourside-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[19];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<19;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        u16 halves[4];for(unsigned i=0;i<4;i++){const u8* p=bytes+32+root+0x44+2*i;halves[i]=((u16)p[0]<<8)|p[1];}
        HSD_Archive* bridge=melee_fourside_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        assert(!memcmp(HSD_ArchiveGetPublicAddress(bridge,"yakumono_param"),expected,68));
        assert(!memcmp((u8*)HSD_ArchiveGetPublicAddress(bridge,"yakumono_param")+0x44,halves,8));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==7);
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&!HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Fourside archive: native stage parameters, absent particle bank and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--flatzone-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[16];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<16;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        HSD_Archive* bridge=melee_flatzone_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        assert(!memcmp(HSD_ArchiveGetPublicAddress(bridge,"yakumono_param"),expected,64));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==8);
        assert(*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&!HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Flat Zone archive: native stage parameters, absent particle bank and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kraid-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[13];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<13;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        HSD_Archive* bridge=melee_kraid_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        assert(!memcmp(HSD_ArchiveGetPublicAddress(bridge,"yakumono_param"),expected,52));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==5);
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Brinstar Depths archive: native stage parameters, particle bank and source disposal passed");return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--training-archive")||!strcmp(argv[1],"--homerun-hud"))){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        bool hr=!strcmp(argv[1],"--homerun-hud");HSD_Archive* owner=hr?melee_homerun_hud_decode(&a):melee_training_decode(&a);assert(owner);memset(bytes,0xa5,size);free(bytes);
        DynamicModelDesc** list=HSD_ArchiveGetPublicAddress(owner,hr?"ScInfCnt_scene_models":"ScGamTraining_scene_models");assert(list&&list[0]&&!list[1]);
        DynamicModelDesc* d=list[0];unsigned counts[3]={0};
        while(d->anims&&d->anims[counts[0]]){assert(counts[0]<64);counts[0]++;}
        while(d->matanims&&d->matanims[counts[1]]){assert(counts[1]<64);counts[1]++;}
        while(d->shapeanims&&d->shapeanims[counts[2]]){assert(counts[2]<64);counts[2]++;}
        unsigned variants=1;for(unsigned k=0;k<3;k++)if(counts[k]>variants)variants=counts[k];
        for(unsigned i=0;i<variants;i++){
            HSD_JObj* joint=HSD_JObjLoadJoint(d->joint);assert(joint);
            HSD_JObjAddAnimAll(joint,i<counts[0]?d->anims[i]:NULL,i<counts[1]?d->matanims[i]:NULL,i<counts[2]?d->shapeanims[i]:NULL);
            for(unsigned f=0;f<300;f++){HSD_JObjReqAnimAll(joint,f);HSD_JObjAnimAll(joint);check_finite_joints(joint);}
            HSD_JObjRemoveAll(joint);
        }
        owner->native_destroy(owner);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("%s: %u variants, 300 animation samples, finite matrices and source disposal passed\n",hr?"Home Run HUD":"Training menu",variants);return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--target-fox-archive")||!strcmp(argv[1],"--target-archive"))){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[8][9];unsigned hit_count=0;
        assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<8;i++){
            u32 at;MeleeHostBool present;
            if(!melee_archive_pointer(&a,root+4*i,&at,&present)||!present)break;hit_count++;
            for(unsigned j=0;j<9;j++)assert(melee_archive_u32(&a,at+4*j,&expected[i][j]));
        }
        u32 item,entry,article,special,common,common_words[5];MeleeHostBool present;
        assert(melee_archive_find(&a,"itemdata",&item));
        assert(melee_archive_pointer(&a,item,&entry,&present)&&present);
        assert(melee_archive_pointer(&a,entry+4,&article,&present)&&present);
        assert(melee_archive_pointer(&a,article+4,&special,&present)&&present);
        assert(melee_archive_pointer(&a,special,&common,&present)&&present);
        for(unsigned i=0;i<5;i++)assert(melee_archive_u32(&a,common+4*i,common_words+i));
        HSD_Archive* bridge=melee_target_stage_decode(&a,argv[2]);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        void** hits=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");assert(hits);
        for(unsigned i=0;i<hit_count;i++)assert(hits[i]&&!memcmp(hits[i],expected[i],36));
        if(!hit_count)assert(!hits[0]);
        struct GroundItemData** items=HSD_ArchiveGetPublicAddress(bridge,"itemdata");
        assert(items&&items[0]&&!items[1]&&items[0]->unk0==It_Kind_Mato);
        Article* target=items[0]->unk4;assert(target&&target->x4_specialAttributes);
        void* common_native=*(void**)target->x4_specialAttributes;
        assert(common_native&&!memcmp(common_native,common_words,sizeof(common_words)));
        const char* symbols[]={"map_head","itemdata","coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned i=0;i<7;i++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[i]));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC);
        unsigned models=head->unkC;
        for(unsigned i=0;i<models;i++){
            struct UnkStageDat_x8_t* m=&head->unk8[i];HSD_JObj* joint=HSD_JObjLoadJoint(m->unk0);assert(joint);
            HSD_JObjAddAnimAll(joint,m->unk4?m->unk4[0]:NULL,m->unk8?m->unk8[0]:NULL,m->unkC?m->unkC[0]:NULL);
            for(unsigned f=0;f<(strstr(argv[2],"GrTMr.dat")?1800u:120u);f++){HSD_JObjReqAnimAll(joint,f);HSD_JObjAnimAll(joint);check_finite_joints(joint);}HSD_JObjRemoveAll(joint);
        }
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Target stage %s: %u models, %u hit descriptors and owned target article passed after source disposal\n",argv[2],models,hit_count);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--inishie2-archive")){
        FILE* file=fopen(argv[2],"rb");assert(file&&!fseek(file,0,SEEK_END));long size=ftell(file);rewind(file);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,items_at,expected[19],common[2][5],special[2][18];
        assert(melee_archive_find(&a,"yakumono_param",&root)&&melee_archive_find(&a,"itemdata",&items_at));
        for(unsigned i=0;i<19;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        for(unsigned i=0;i<12;i++){unsigned off=i<10?2*i:0x48+2*(i-10);const u8* q=bytes+32+root+off;u16 v=((u16)q[0]<<8)|q[1];memcpy((u8*)expected+off,&v,2);}
        const unsigned kinds[]={It_Kind_Kyasarin,It_Kind_Kyasarin_Egg},words[]={18,4},counts[]={4,3};
        for(unsigned i=0;i<2;i++){
            u32 entry=stage_ref(&a,items_at+4*i),article=stage_ref(&a,entry+4),sp=stage_ref(&a,article+4),cp=stage_ref(&a,sp);
            for(unsigned j=0;j<5;j++)assert(melee_archive_u32(&a,cp+4*j,&common[i][j]));
            for(unsigned j=0;j<words[i];j++)assert(melee_archive_u32(&a,sp+4+4*j,&special[i][j]));
        }
        HSD_Archive* owner=melee_inishie2_stage_decode(&a);assert(owner);memset(bytes,0xa5,size);free(bytes);
        assert(!memcmp(HSD_ArchiveGetPublicAddress(owner,"yakumono_param"),expected,sizeof(expected)));
        struct GroundItemData** items=HSD_ArchiveGetPublicAddress(owner,"itemdata");assert(items&&items[0]&&items[1]&&!items[2]);
        for(unsigned i=0;i<2;i++){
            assert(items[i]->unk0==kinds[i]);Article* d=items[i]->unk4;assert(d&&d->x4_specialAttributes);
            void* cp;u8* tail;
            if(i==0){itKyasarinAttributes* v=d->x4_specialAttributes;cp=v->x0;tail=(u8*)&v->x4;}
            else{itKyasarinEggAttributes* v=d->x4_specialAttributes;cp=v->x0;tail=(u8*)&v->x4;}
            assert(!memcmp(cp,common[i],20)&&!memcmp(tail,special[i],4*words[i]));
            for(unsigned j=0;j<counts[i];j++){
                HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);
                ItemStateDesc* anim=d->xC_itemStates->x0_itemStateDesc+j;
                HSD_JObjAddAnimAll(joint,anim->x0_anim_joint,anim->x4_matanim_joint,anim->x8_parameters);
                for(unsigned f=0;f<120;f++){HSD_JObjReqAnimAll(joint,f);HSD_JObjAnimAll(joint);}HSD_JObjRemoveAll(joint);
            }
        }
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(owner,"map_head");assert(head&&head->unkC);
        unsigned models=head->unkC;
        for(unsigned i=0;i<models;i++){
            struct UnkStageDat_x8_t* m=&head->unk8[i];HSD_JObj* joint=HSD_JObjLoadJoint(m->unk0);assert(joint);
            HSD_JObjAddAnimAll(joint,m->unk4?m->unk4[0]:NULL,m->unk8?m->unk8[0]:NULL,m->unkC?m->unkC[0]:NULL);
            for(unsigned f=0;f<120;f++){HSD_JObjReqAnimAll(joint,f);HSD_JObjAnimAll(joint);}HSD_JObjRemoveAll(joint);
        }
        owner->native_destroy(owner);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Mushroom Kingdom II: %u models, Birdo/egg states, mixed parameters and owned lifetime passed\n",models);return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--homerun-archive")||!strcmp(argv[1],"--inishie1-archive")||!strcmp(argv[1],"--heal-archive")||!strcmp(argv[1],"--zebes-route-archive")||!strcmp(argv[1],"--bigblue-route-archive"))){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        int homerun=!strcmp(argv[1],"--homerun-archive"),inishie1=!strcmp(argv[1],"--inishie1-archive"),heal=!strcmp(argv[1],"--heal-archive"),bigblue=!strcmp(argv[1],"--bigblue-route-archive");u32 root,expected[21];unsigned words=homerun?1:inishie1?21:bigblue?20:2;
        assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<words;i++)assert(melee_archive_u32(&a,root+4*i,&expected[i]));
        if(inishie1)for(unsigned i=0;i<6;i++){unsigned off=0x14+2*i;const u8* q=bytes+32+root+off;u16 v=((u16)q[0]<<8)|q[1];memcpy((u8*)expected+off,&v,2);}
        HSD_Archive* bridge=homerun?melee_homerun_stage_decode(&a):inishie1?melee_inishie1_stage_decode(&a):heal?melee_heal_stage_decode(&a):bigblue?melee_bigblue_route_stage_decode(&a):melee_zebes_route_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        int* params=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");assert(params&&!memcmp(params,expected,words*4));
        const char* symbols[]={"map_head","coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned i=0;i<6;i++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[i]));
        struct GroundItemData** items=HSD_ArchiveGetPublicAddress(bridge,"itemdata");assert(items&&!items[0]);
        if(homerun||bigblue||heal||inishie1){
            UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==(homerun?11:inishie1?4:heal?5:38));if(bigblue)assert(!head->unk8[33].unk4);
            unsigned models=0;
            for(unsigned i=0;i<head->unkC;i++){
                struct UnkStageDat_x8_t* m=&head->unk8[i];if(!m->unk0)continue;
                HSD_JObj* j=HSD_JObjLoadJoint(m->unk0);assert(j);
                HSD_JObjAddAnimAll(j,m->unk4?m->unk4[0]:NULL,m->unk8?m->unk8[0]:NULL,m->unkC?m->unkC[0]:NULL);
                for(unsigned frame=0;frame<120;frame++){HSD_JObjReqAnimAll(j,frame);HSD_JObjAnimAll(j);}
                HSD_JObjRemoveAll(j);models++;
            }
            assert(homerun?models==11:inishie1?models==4:heal?models==5:models>30);if(bigblue)printf("F-Zero native route: %u model hierarchies animated; dormant slot 33 animation omitted\n",models);
        }
        bridge->native_destroy(bridge);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Adventure route stage: native parameters, model/collision/light owners and source disposal passed");return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--brinstar-cutscene-archive")||!strcmp(argv[1],"--explosion-cutscene-archive")||!strcmp(argv[1],"--cutscene-archive"))){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        char symbols[4][128];const char* names[4];unsigned count=0,models=0;
        for(unsigned i=0;i<a.public_count;i++){
            const char* name;u32 root;assert(melee_archive_public(&a,i,&name,&root));size_t n=strlen(name);
            if(!strncmp(name,"visual",6)&&n>=5&&!strcmp(name+n-5,"Scene")){
                assert(count<4&&n<128);strcpy(symbols[count],name);names[count]=symbols[count];count++;
            }
        }
        assert(count);HSD_Archive* bridge=melee_cutscene_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        for(unsigned i=0;i<count;i++){
            SceneDesc* scene=HSD_ArchiveGetPublicAddress(bridge,names[i]);assert(scene&&scene->models);
            for(unsigned j=0;scene->cameras&&scene->cameras[j].desc;j++){
                struct SceneCameraDesc* desc=&scene->cameras[j];
                HSD_CObj* camera=HSD_CObjLoadDesc(desc->desc);assert(camera);
                for(unsigned k=0;desc->anims&&desc->anims[k];k++){
                    HSD_CObjAddAnim(camera,desc->anims[k]);
                    for(unsigned frame=0;frame<=600;frame+=10){HSD_CObjReqAnim(camera,frame);HSD_CObjAnim(camera);}
                }
                HSD_OBJECT_METHOD(camera)->release((HSD_Class*)camera);HSD_OBJECT_METHOD(camera)->destroy((HSD_Class*)camera);
            }
            for(unsigned j=0;scene->lights&&scene->lights[j];j++){
                HSD_LObj* light=HSD_LObjLoadDesc(scene->lights[j]->desc);assert(light);
                for(unsigned k=0;scene->lights[j]->anims&&scene->lights[j]->anims[k];k++){
                    HSD_LObjAddAnimAll(light,scene->lights[j]->anims[k]);
                    for(unsigned frame=0;frame<=600;frame+=10){HSD_LObjReqAnimAll(light,frame);HSD_LObjAnimAll(light);}
                }
                HSD_LObjRemoveAll(light);
            }
            if(scene->fogs){
                HSD_Fog* fog=HSD_FogLoadDesc(scene->fogs->desc);assert(fog);
                for(unsigned k=0;scene->fogs->anims&&scene->fogs->anims[k];k++){
                    HSD_Fog_8037DE7C(fog,scene->fogs->anims[k]->aobjdesc);
                    for(unsigned frame=0;frame<=600;frame+=10){HSD_FogReqAnim(fog,frame);HSD_FogInterpretAnim(fog);assert(isfinite(fog->start)&&isfinite(fog->end));}
                }
                hsdDelete(fog);
            }
            for(unsigned j=0;scene->models[j];j++){
                DynamicModelDesc* m=scene->models[j];HSD_JObj* joint=HSD_JObjLoadJoint(m->joint);assert(joint);
                HSD_JObjAddAnimAll(joint,m->anims?m->anims[0]:NULL,m->matanims?m->matanims[0]:NULL,m->shapeanims?m->shapeanims[0]:NULL);
                for(unsigned frame=0;frame<120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);models++;
            }
        }
        assert(models>0);bridge->native_destroy(bridge);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Cutscene: %u owned scenes, %u animated models and source disposal passed\n",count,models);return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--demo-wait-archive")||!strcmp(argv[1],"--demo-result-archive"))){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));const char* name;u32 root;
        assert(melee_archive_public(&a,0,&name,&root));char symbol[128];assert(strlen(name)<128);strcpy(symbol,name);
        HSD_Archive* bridge=!strcmp(argv[1],"--demo-result-archive")?melee_demo_result_decode(&a):melee_demo_wait_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        size_t n;const u8* motion=melee_intro_motion_bytes(bridge,symbol,&n);MeleeArchive inner;assert(motion&&n>=32);u32 inner_size=((u32)motion[0]<<24)|((u32)motion[1]<<16)|((u32)motion[2]<<8)|motion[3];assert(inner_size<=n&&melee_archive_open(&inner,motion,inner_size));
        bridge->native_destroy(bridge);puts("Demo archive: nested motion survives source disposal");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--adventure-intro-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        HSD_Archive* bridge=melee_adventure_intro_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        assert(HSD_ArchiveGetPublicAddress(bridge,"ScItrNormal_scene_data"));
        for(unsigned i=1;i<=12;i++){
            char symbol[5];snprintf(symbol,sizeof(symbol),"mc%02u",i);
            struct SceneCameraDesc* desc=HSD_ArchiveGetPublicAddress(bridge,symbol);assert(desc&&desc->desc&&desc->anims&&desc->anims[0]);
            HSD_CObj* camera=HSD_CObjLoadDesc(desc->desc);assert(camera);
            for(unsigned j=0;desc->anims[j];j++){
                HSD_CObjAddAnim(camera,desc->anims[j]);
                for(unsigned frame=0;frame<=600;frame+=10){HSD_CObjReqAnim(camera,frame);HSD_CObjAnim(camera);}
            }
            HSD_OBJECT_METHOD(camera)->release((HSD_Class*)camera);HSD_OBJECT_METHOD(camera)->destroy((HSD_Class*)camera);
        }
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Adventure intro: twelve owned animated route cameras and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--maze-stage-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,words[74],hit[9];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<74;i++)assert(melee_archive_u32(&a,root+4*i,&words[i]));
        u32 hp=stage_ref(&a,root+16);for(unsigned i=0;i<9;i++)assert(melee_archive_u32(&a,hp+4*i,&hit[i]));
        HSD_Archive* bridge=melee_maze_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        struct grShrineRoute_YakumonoParam* p=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");
        assert(p&&p->x0&&p->x4&&p->x8&&p->xC&&p->x10&&!memcmp(p->x10,hit,36)&&!memcmp(&p->x14,words+5,20));
        u8* spawn=(u8*)&p->spawn_desc;
        for(unsigned i=0;i<64;i++){u16 kind;memcpy(&kind,spawn+4*i,2);assert(kind==(words[10+i]>>16)&&spawn[4*i+2]==((words[10+i]>>8)&255)&&spawn[4*i+3]==(words[10+i]&255));}
        for(unsigned i=64*4;i<80*4;i++)assert(spawn[i]==0);
        struct GroundItemData** items=HSD_ArchiveGetPublicAddress(bridge,"itemdata");assert(items&&items[0]&&!items[1]&&items[0]->unk0==It_Kind_Likelike);
        Article* item=items[0]->unk4;assert(item&&item->x4_specialAttributes);
        HSD_JObj* joint=HSD_JObjLoadJoint(item->x10_modelDesc->x0_joint);assert(joint);HSD_JObjRemoveAll(joint);
        const char* symbols[]={"map_head","coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit","map_ptcl","map_texg"};
        for(unsigned i=0;i<8;i++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[i]));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Maze archive: color scripts, damage descriptor, 64 spawn records, Like Like article and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--likelike-article")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 table;assert(melee_archive_find(&a,"itemdata",&table));
        u32 entry=stage_ref(&a,table),kind;assert(melee_archive_u32(&a,entry,&kind)&&kind==It_Kind_Likelike);
        u32 article=stage_ref(&a,entry+4),sp=stage_ref(&a,article+4),cp=stage_ref(&a,sp),common[5],words[34];u8 flags[4];
        for(unsigned i=0;i<5;i++)assert(melee_archive_u32(&a,cp+4*i,&common[i]));
        for(unsigned i=0;i<34;i++)assert(melee_archive_u32(&a,sp+4*i,&words[i]));memcpy(flags,bytes+32+sp+0x3C,4);
        MeleeItemArticle* owner=melee_item_article_decode(&a,kind,article,10);assert(owner);memset(bytes,0xa5,size);free(bytes);
        Article* d=melee_item_article_descriptor(owner);itLikelikeAttributes* attrs=d->x4_specialAttributes;assert(attrs);
        assert(!memcmp(attrs->x0.x0_f32,common,20)&&!memcmp(&attrs->x4,words+1,56)&&!memcmp(&attrs->x3C,flags,4)&&!memcmp(attrs->x40,words+16,72));
        for(unsigned i=0;i<10;i++){
            HSD_JObj* joint=HSD_JObjLoadJoint(d->x10_modelDesc->x0_joint);assert(joint);
            ItemStateDesc* anim=d->xC_itemStates->x0_itemStateDesc+i;
            HSD_JObjAddAnimAll(joint,anim->x0_anim_joint,anim->x4_matanim_joint,anim->x8_parameters);
            for(unsigned frame=0;frame<120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
            HSD_JObjRemoveAll(joint);
        }
        melee_item_article_free(owner);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Like Like article: nested attributes, two hit descriptors, packed bytes, ten animations and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kinoko-route-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,words[81],special[4][18],common[2][2],itemroot;
        assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<81;i++)assert(melee_archive_u32(&a,root+4*i,words+i));
        assert(melee_archive_find(&a,"itemdata",&itemroot));
        const unsigned kinds[]={It_Kind_Nokonoko,It_Kind_Patapata,It_Kind_ZGShell,It_Kind_ZRShell},counts[]={2,15,18,18};
        for(unsigned i=0;i<4;i++){
            u32 entry=stage_ref(&a,itemroot+4*i),article=stage_ref(&a,entry+4),sp=stage_ref(&a,article+4);
            if(i<2){u32 cp=stage_ref(&a,sp);for(unsigned j=0;j<2;j++)assert(melee_archive_u32(&a,cp+4*j,&common[i][j]));sp+=4;}
            for(unsigned j=0;j<counts[i];j++)assert(melee_archive_u32(&a,sp+4*j,&special[i][j]));
        }
        HSD_Archive* bridge=melee_kinoko_route_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        u8* params=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");assert(params&&!memcmp(params,words,4));
        for(unsigned i=0;i<80;i++){
            u16 kind;memcpy(&kind,params+4+4*i,2);assert(kind==(words[i+1]>>16));
            assert(params[6+4*i]==((words[i+1]>>8)&255)&&params[7+4*i]==(words[i+1]&255));
        }
        struct GroundItemData** items=HSD_ArchiveGetPublicAddress(bridge,"itemdata");assert(items&&!items[4]);
        for(unsigned i=0;i<4;i++){
            assert(items[i]&&items[i]->unk0==kinds[i]);Article* item=items[i]->unk4;assert(item&&item->x4_specialAttributes);
            u8* sp=item->x4_specialAttributes;
            if(i<2){void* cp=*(void**)sp;assert(cp&&!memcmp(cp,common[i],8));sp+=sizeof(void*);}
            assert(!memcmp(sp,special[i],4*counts[i]));
            assert(item->x10_modelDesc&&item->x10_modelDesc->x0_joint);
            HSD_JObj* joint=HSD_JObjLoadJoint(item->x10_modelDesc->x0_joint);assert(joint);HSD_JObjRemoveAll(joint);
        }
        const char* symbols[]={"map_head","coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit","map_ptcl","map_texg"};
        for(unsigned i=0;i<8;i++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[i]));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Kinoko Route archive: 80 mixed-width spawns, four enemy articles, nested attributes and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--pushon-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,hits[6][9],words[0x214/4];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<0x214/4;i++)assert(melee_archive_u32(&a,root+4*i,words+i));
        for(unsigned i=0;i<6;i++){
            u32 at;MeleeHostBool found;assert(melee_archive_pointer(&a,root+4*i,&at,&found)&&found);
            for(unsigned j=0;j<9;j++)assert(melee_archive_u32(&a,at+4*j,&hits[i][j]));
        }
        HSD_Archive* bridge=melee_pushon_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        struct grPushon_YakumonoParam* p=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");assert(p);
        void* records[]={p->x0,p->x4,p->x8,p->xC,p->x10,p->x14};
        for(unsigned i=0;i<6;i++)assert(records[i]&&!memcmp(records[i],hits[i],36));
        assert(p->x18==words[6]);
        for(unsigned i=0;i<30;i++){
            assert((u32)p->x1c[i].x0==words[7+2*i]);
            assert((u16)p->x1c[i].x4==(words[8+2*i]>>16));
            assert((u16)p->x1c[i].x6==(words[8+2*i]&65535));
        }
        assert(!memcmp(p->x10c,words+0x10c/4,sizeof(p->x10c)));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC>0);
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&!HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Race to the Finish archive: six hit descriptors, 30 mixed-width entries, 33 lookups and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--figureget-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[6];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<6;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        HSD_Archive* bridge=melee_figureget_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        assert(!memcmp(HSD_ArchiveGetPublicAddress(bridge,"yakumono_param"),expected,24));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==2);
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&!HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Trophy bonus archive: native stage parameters, absent particle bank and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--pura-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[1];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<1;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        HSD_Archive* bridge=melee_pura_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        assert(!memcmp(HSD_ArchiveGetPublicAddress(bridge,"yakumono_param"),expected,4));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==28);
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&!HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Poke Floats archive: native stage parameters, absent particle bank and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--rcruise-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[18];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<18;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        u32 flagroot,flagwords[90],poswords[3];assert(melee_archive_find(&a,"dynamicsdata_shipflag",&flagroot));
        u32 flagparams=stage_ref(&a,flagroot);
        for(unsigned i=0;i<90;i++)assert(melee_archive_u32(&a,flagparams+4*i,flagwords+i));
        for(unsigned i=0;i<3;i++)assert(melee_archive_u32(&a,flagroot+8+4*i,poswords+i));
        HSD_Archive* bridge=melee_rcruise_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        assert(!memcmp(HSD_ArchiveGetPublicAddress(bridge,"yakumono_param"),expected,72));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==7);
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&!HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        DynamicsDesc* flag=HSD_ArchiveGetPublicAddress(bridge,"dynamicsdata_shipflag");
        assert(flag&&flag->count==6&&flag->params&&!memcmp(flag->params,flagwords,sizeof(flagwords))&&!memcmp(&flag->pos,poswords,sizeof(poswords)));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Rainbow Cruise archive: native stage parameters, owned ship-flag dynamics and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--bigblue-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[81];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<81;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        HSD_Archive* bridge=melee_bigblue_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        assert(!memcmp(HSD_ArchiveGetPublicAddress(bridge,"yakumono_param"),expected,324));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==41);
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&!HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        HSD_CObj* camera=HSD_CObjLoadDesc((HSD_CObjDesc*)head->unk8[31].x10);assert(camera);
        HSD_CameraAnim* anim=head->unk8[31].x14;
        assert(anim&&anim->eye_anim&&anim->interest_anim);
        HSD_CObjAddAnim(camera,anim);HSD_CObjReqAnim(camera,0);HSD_CObjAnim(camera);
        Vec3 eye0,target0,eye,target;HSD_CObjGetEyePosition(camera,&eye0);HSD_CObjGetInterest(camera,&target0);
        int eye_changed=0,target_changed=0;
        for(unsigned frame=1;frame<=7200;frame+=60){
            HSD_CObjReqAnim(camera,frame);HSD_CObjAnim(camera);
            HSD_CObjGetEyePosition(camera,&eye);HSD_CObjGetInterest(camera,&target);
            assert(isfinite(eye.x)&&isfinite(eye.y)&&isfinite(eye.z));
            assert(isfinite(target.x)&&isfinite(target.y)&&isfinite(target.z));
            if(fabsf(eye.x-eye0.x)+fabsf(eye.y-eye0.y)+fabsf(eye.z-eye0.z)>1)eye_changed=1;
            if(fabsf(target.x-target0.x)+fabsf(target.y-target0.y)+fabsf(target.z-target0.z)>1)target_changed=1;
        }
        assert(eye_changed&&target_changed);hsdDelete(camera);
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Big Blue archive: native stage parameters, absent particle bank and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--shrine-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[1];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<1;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        HSD_Archive* bridge=melee_shrine_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        assert(!memcmp(HSD_ArchiveGetPublicAddress(bridge,"yakumono_param"),expected,4));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==3);
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&!HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Temple archive: native stage parameters, absent particle bank and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--yorster-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,expected[8];assert(melee_archive_find(&a,"yakumono_param",&root));
        for(unsigned i=0;i<8;i++)assert(melee_archive_u32(&a,root+4*i,expected+i));
        HSD_Archive* bridge=melee_yorster_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        assert(!memcmp(HSD_ArchiveGetPublicAddress(bridge,"yakumono_param"),expected,32));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==2);
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit"};
        for(unsigned j=0;j<5;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&!HSD_ArchiveGetPublicAddress(bridge,"map_texg"));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Yoshi Island archive: native block parameters, absent particle bank and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--castle-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;assert(melee_archive_find(&a,"yakumono_param",&root));
        u8 expected[324];for(unsigned at=0;at<324;){
            int half=at<16||(at>=0x40&&at<0x48)||(at>=0x54&&at<0x5C)||(at>=0x12C&&at<0x134)||(at>=0x5C&&at<0x110&&(at-0x5C)%20<4);
            if(half){u16 v=((u16)bytes[32+root+at]<<8)|bytes[33+root+at];memcpy(expected+at,&v,2);at+=2;}
            else{u32 v;assert(melee_archive_u32(&a,root+at,&v));memcpy(expected+at,&v,4);at+=4;}
        }
        const char* names[]={"dynamicsdata_flag3","dynamicsdata_flag4","dynamicsdata_flag6"};
        const unsigned counts[]={3,4,6};float values[3][90],positions[3][3];
        for(unsigned j=0;j<3;j++){u32 at;assert(melee_archive_find(&a,names[j],&at));u32 params=stage_ref(&a,at);
            for(unsigned k=0;k<counts[j]*15;k++)assert(melee_archive_f32(&a,params+4*k,&values[j][k]));
            for(unsigned k=0;k<3;k++)assert(melee_archive_f32(&a,at+8+4*k,&positions[j][k]));
        }
        HSD_Archive* bridge=melee_castle_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        struct grCastle_YakumonoParam* params=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");
        assert(params&&params->x114&&!memcmp(params,expected,0x114)&&!memcmp(&params->x118,expected+0x118,0x2C));
        for(unsigned j=0;j<3;j++){DynamicsDesc* d=HSD_ArchiveGetPublicAddress(bridge,names[j]);assert(d&&d->params&&d->count==counts[j]);assert(!memcmp(d->params,values[j],60*counts[j])&&!memcmp(&d->pos,positions[j],12));}
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==21);
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit","map_ptcl","map_texg"};
        for(unsigned j=0;j<7;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Castle archive: owned flag physics, mixed-width hazard parameters and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--brinstar-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;assert(melee_archive_find(&a,"yakumono_param",&root));
        u8 expected[400];for(unsigned at=0;at<400;){
            if(at>=160){u16 v=((u16)bytes[32+root+at]<<8)|bytes[33+root+at];memcpy(expected+at,&v,2);at+=2;}
            else{u32 v;assert(melee_archive_u32(&a,root+at,&v));memcpy(expected+at,&v,4);at+=4;}
        }
        u32 hit=stage_ref(&a,root+0x2C),words[9];for(unsigned j=0;j<9;j++)assert(melee_archive_u32(&a,hit+4*j,words+j));
        HSD_Archive* bridge=melee_brinstar_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        grZe_YakumonoParam* params=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");
        assert(params&&params->acid_hit&&!memcmp(params,expected,0x2C)&&!memcmp(&params->x30,expected+0x30,0x160));
        assert(!memcmp(params->acid_hit,words,36));assert(((lbColl_80008D30_arg1*)params->acid_hit)->damage==14);
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==10);
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit","map_ptcl","map_texg"};
        for(unsigned j=0;j<7;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        bridge->native_destroy(bridge);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Brinstar archive: owned acid hit description and mixed-width timing table passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--kongo-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,item;
        assert(melee_archive_find(&a,"yakumono_param",&root));u8 expected[188];
        for(unsigned at=0;at<188;){
            if(at>=0x44&&at<0x54){u16 v=((u16)bytes[32+root+at]<<8)|bytes[33+root+at];memcpy(expected+at,&v,2);at+=2;}
            else{u32 v;assert(melee_archive_u32(&a,root+at,&v));memcpy(expected+at,&v,4);at+=4;}
        }
        assert(melee_archive_find(&a,"itemdata",&item));u32 entry=stage_ref(&a,item),article=stage_ref(&a,entry+4),special=stage_ref(&a,article+4),common=stage_ref(&a,special);
        u32 words[5];for(unsigned j=0;j<5;j++)assert(melee_archive_u32(&a,common+4*j,words+j));
        HSD_Archive* bridge=melee_kongo_stage_decode(&a);assert(bridge);memset(bytes,0xa5,size);free(bytes);
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==11);
        struct grKongo_YakumonoParam* params=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");assert(params&&params->unk84);
        assert(!memcmp(params,expected,0x84)&&!memcmp(&params->unk88,expected+0x88,0x34));
        struct GroundItemData** items=HSD_ArchiveGetPublicAddress(bridge,"itemdata");assert(items[0]&&!items[1]&&items[0]->unk0==It_Kind_Klap);
        itKlapAttributes* attrs=items[0]->unk4->x4_specialAttributes;assert(attrs&&attrs->common==attrs->owned_common&&!memcmp(attrs->common,words,20));
        assert(items[0]->unk4->xC_itemStates);
        const char* symbols[]={"coll_data","grGroundParam","ALDYakuAll","quake_model_set","map_plit","map_ptcl","map_texg"};
        for(unsigned j=0;j<7;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        bridge->native_destroy(bridge);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Kongo archive: mixed-width hazard block, owned Klaptrap pointer and source disposal passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--greatbay-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,item;
        assert(melee_archive_find(&a,"yakumono_param",&root));
        u8 expected[164];
        for(unsigned at=0;at<164;){
            bool half=at<4||(at>=0x44&&at<0x4c)||(at>=0x70&&at<0x78)||at>=0x7c;
            if(half){u16 v=((u16)bytes[32+root+at]<<8)|bytes[33+root+at];memcpy(expected+at,&v,2);at+=2;}
            else{u32 v;assert(melee_archive_u32(&a,root+at,&v));memcpy(expected+at,&v,4);at+=4;}
        }
        assert(melee_archive_find(&a,"itemdata",&item));u32 entry=stage_ref(&a,item),article=stage_ref(&a,entry+4),special=stage_ref(&a,article+4);
        u32 scalar[21];for(unsigned j=0;j<21;j++)assert(melee_archive_u32(&a,special+4*j,scalar+j));
        u8 packed[4];memcpy(packed,bytes+32+special+84,4);
        HSD_Archive* bridge=melee_greatbay_stage_decode(&a);assert(bridge);
        memset(bytes,0xa5,size);free(bytes);
        const char* symbols[]={"map_head","coll_data","grGroundParam","yakumono_param","itemdata","ALDYakuAll","quake_model_set","map_plit","map_ptcl","map_texg"};
        for(unsigned j=0;j<10;j++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[j]));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head->unkC==10);
        assert(!memcmp(HSD_ArchiveGetPublicAddress(bridge,"yakumono_param"),expected,164));
        struct GroundItemData** items=HSD_ArchiveGetPublicAddress(bridge,"itemdata");assert(items[0]&&!items[1]&&items[0]->unk0==It_Kind_Tincle);
        u8* attrs=items[0]->unk4->x4_specialAttributes;assert(attrs);
        for(unsigned j=0;j<21;j++){u32 v;memcpy(&v,attrs+4*j,4);assert(v==scalar[j]);}
        assert(!memcmp(attrs+84,packed,4));assert(items[0]->unk4->xC_itemStates);
        bridge->native_destroy(bridge);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Great Bay archive: mixed-width parameters, owned Tingle article and source disposal passed");return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--stadium-archive")||!strcmp(argv[1],"--stadium-transform-archive"))){
        bool transform=!strcmp(argv[1],"--stadium-transform-archive");
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root;
        assert(melee_archive_find(&a,"yakumono_param",&root));
        u32 words[18];for(unsigned i=0;i<18;i++)assert(melee_archive_u32(&a,root+4*i,words+i));
        u8 colors[4];memcpy(colors,bytes+32+root+28,4);
        u16 halves[5];for(unsigned i=0;i<5;i++){const u8* h=bytes+32+root+72+2*i;halves[i]=((u16)h[0]<<8)|h[1];}
        u8 text[22][32];
        if(!transform){u32 table;assert(melee_archive_find(&a,"SIS_GrPStadiumData",&table));for(unsigned i=0;i<22;i++){u32 target=stage_ref(&a,table+4*i);assert(target<a.data_size-32);memcpy(text[i],bytes+32+target,32);}}
        HSD_Archive* bridge=melee_stadium_stage_decode(&a,transform);assert(bridge);
        memset(bytes,0xa5,size);free(bytes);
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head&&head->unkC==10);
        u8* params=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");assert(params);
        for(unsigned i=0;i<18;i++)if(i!=7){u32 value;memcpy(&value,params+4*i,4);assert(value==words[i]);}
        assert(!memcmp(params+28,colors,4)&&!memcmp(params+72,halves,10));
        assert(HSD_ArchiveGetPublicAddress(bridge,"coll_data")&&HSD_ArchiveGetPublicAddress(bridge,"grGroundParam"));
        assert(HSD_ArchiveGetPublicAddress(bridge,"ALDYakuAll"));
        assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        void** bank=HSD_ArchiveGetPublicAddress(bridge,"SIS_GrPStadiumData");
        if(transform){assert(!bank&&!HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&!HSD_ArchiveGetPublicAddress(bridge,"map_plit"));}
        else{
            assert(bank);for(unsigned i=0;i<22;i++)assert(!memcmp(bank[i],text[i],32));
            HSD_ImageDesc* screen=HSD_ArchiveGetPublicAddress(bridge,"GrdPStadiumBG_OVDummy_mat6962_GrdPStadiumDummy_0_image_desc");assert(screen&&screen->image_ptr);
            assert(HSD_ArchiveGetPublicAddress(bridge,"map_ptcl")&&HSD_ArchiveGetPublicAddress(bridge,"map_plit"));
        }
        bridge->native_destroy(bridge);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Stadium archive: scalar/packed parameters, owned text and resources passed");return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--japes-archive")||!strcmp(argv[1],"--story-archive")||!strcmp(argv[1],"--dreamland-archive")||!strcmp(argv[1],"--fountain-archive"))){
        bool japes=!strcmp(argv[1],"--japes-archive");bool story=!strcmp(argv[1],"--story-archive");bool fountain=!strcmp(argv[1],"--fountain-archive");unsigned n=japes?8:story?9:fountain?21:11,skip=(japes||story||fountain)?0:8;
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        u32 root;assert(melee_archive_find(&a,"yakumono_param",&root));
        u32 words[21];for(unsigned i=0;i<n;i++)assert(melee_archive_u32(&a,root+skip+4*i,&words[i]));
        HSD_Archive* bridge=japes?melee_japes_stage_decode(&a):story?melee_story_stage_decode(&a):fountain?melee_fountain_stage_decode(&a):melee_dreamland_stage_decode(&a);assert(bridge);
        memset(bytes,0xa5,size);free(bytes);
        const char* symbols[]={"map_head","coll_data","grGroundParam","yakumono_param","itemdata","ALDYakuAll","quake_model_set","map_plit","map_ptcl","map_texg"};
        for(unsigned i=0;i<10;i++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[i]));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");assert(head->unkC==(japes?7:story?4:fountain?5:8));
        if(fountain){
            HSD_ImageDesc* mirror=HSD_ArchiveGetPublicAddress(bridge,"GrdIzumi_cd_wt_GrdIzumiDummy1_1_image_desc");assert(mirror&&mirror->image_ptr);
            HSD_Joint* stack[256];unsigned pending=0,matches=0;
            stack[pending++]=head->unk8[3].unk0;
            while(pending){
                HSD_Joint* joint=stack[--pending];if(!joint)continue;
                assert(pending+2<=256);stack[pending++]=joint->child;stack[pending++]=joint->next;
                if(joint->flags&(JOBJ_SPLINE|JOBJ_PTCL))continue;
                for(HSD_DObjDesc* d=joint->u.dobjdesc;d;d=d->next)
                    for(HSD_TObjDesc* t=d->mobjdesc?d->mobjdesc->texdesc:NULL;t;t=t->next)if(t->imagedesc==mirror)matches++;
            }
            assert(matches>0);
        }
        u8* params=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");
        const s16 halves[]={3000,4000,30,0};if(!japes&&!fountain&&!story)assert(!memcmp(params,halves,8));
        for(unsigned i=0;i<n;i++){u32 actual;memcpy(&actual,params+skip+4*i,4);assert(actual==words[i]);}
        if(story){
            struct GroundItemData** items=HSD_ArchiveGetPublicAddress(bridge,"itemdata");
            assert(items[0]&&!items[1]&&items[0]->unk0==It_Kind_Heiho);
            itHeihoAttributes* attrs=items[0]->unk4->x4_specialAttributes;
            assert(attrs->damage_threshold==&attrs->owned_damage_threshold);
            assert(*attrs->damage_threshold==15&&fabsf(attrs->values[0]-0.3f)<0.00001f&&attrs->values[5]==30.0f);
        }else assert(!*(void**)HSD_ArchiveGetPublicAddress(bridge,"itemdata"));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"missing"));
        bridge->native_destroy(bridge);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_MtxGetAllocData()->used&&!HSD_VecGetAllocData()->used);
        puts("Stage scalar archive: native symbols, wind parameters, source disposal and HSD cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--battle-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));HSD_Archive* bridge=melee_battle_stage_decode(&a);assert(bridge);
        memset(bytes,0xa5,size);free(bytes);
        const char* symbols[]={"map_head","coll_data","grGroundParam","yakumono_param","itemdata","ALDYakuAll","quake_model_set","map_plit","map_ptcl","map_texg"};
        for(unsigned i=0;i<10;i++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[i]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"missing"));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");LightList** lights=HSD_ArchiveGetPublicAddress(bridge,"map_plit");
        LightOverrideEntry* overrides=head->unk18;
        for(unsigned i=0;lights[i];i++){int found=0;for(int j=0;j<head->unk1C;j++)if(lights[i]->desc==overrides[j].desc)found=1;assert(found);}
        union ColorOverlay_x8_t** colors=HSD_ArchiveGetPublicAddress(bridge,"yakumono_param");
        assert(colors[0]&&colors[1]&&colors[0]!=colors[1]);
        for(unsigned i=0;i<2;i++){
            assert(colors[i][0].unk.unk==18&&colors[i][2].unk.unk==19);
            assert(colors[i][2].unk.timer==200&&colors[i][4].unk.unk==11);
            assert(colors[i][1].light_color.a==(i?0:255)&&colors[i][3].light_color.a==(i?255:0));
            assert(colors[i][5].unk.unk==12&&colors[i][6].unk.unk==10);
        }
        bridge->native_destroy(bridge);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_MtxGetAllocData()->used&&!HSD_VecGetAllocData()->used);
        puts("Battlefield archive: ten native symbols, particle-light identity, source disposal and HSD cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--onett-archive")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));HSD_Archive* bridge=melee_onett_stage_decode(&a);assert(bridge);
        memset(bytes,0xa5,size);free(bytes);
        const char* symbols[]={"map_head","coll_data","grGroundParam","yakumono_param","itemdata","ALDYakuAll","quake_model_set","map_plit","map_ptcl","map_texg"};
        for(unsigned i=0;i<10;i++)assert(HSD_ArchiveGetPublicAddress(bridge,symbols[i]));
        assert(!HSD_ArchiveGetPublicAddress(bridge,"missing"));
        UnkStageDat* head=HSD_ArchiveGetPublicAddress(bridge,"map_head");LightList** lights=HSD_ArchiveGetPublicAddress(bridge,"map_plit");
        LightOverrideEntry* overrides=head->unk18;
        for(unsigned i=0;lights[i];i++){int found=0;for(int j=0;j<head->unk1C;j++)if(lights[i]->desc==overrides[j].desc)found=1;assert(found);}
        bridge->native_destroy(bridge);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_MtxGetAllocData()->used&&!HSD_VecGetAllocData()->used);
        puts("Onett archive: ten native symbols, particle-light identity, source disposal and HSD cleanup passed");return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--dynamic-model")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive archive;assert(melee_archive_open(&archive,bytes,size));
        MeleeDynamicModel* owner=melee_dynamic_model_decode(&archive,argv[3]);assert(owner);
        MeleeArchive truncated=archive;u32 model_root;assert(melee_archive_find(&archive,argv[3],&model_root));truncated.data_size=model_root+15;
        assert(!melee_dynamic_model_decode(&truncated,argv[3]));
        memset(bytes,0xa5,size);free(bytes);DynamicModelDesc* d=melee_dynamic_model_descriptor(owner);assert(d&&d->joint);
        unsigned counts[3],n=1;for(unsigned k=0;k<3;k++){counts[k]=melee_dynamic_model_animation_count(owner,k);if(counts[k]>n)n=counts[k];}
        unsigned moving=0;
        for(unsigned i=0;i<n;i++){
            HSD_JObj* joint=HSD_JObjLoadJoint(d->joint);assert(joint);
            HSD_JObjAddAnimAll(joint,i<counts[0]?d->anims[i]:NULL,i<counts[1]?d->matanims[i]:NULL,i<counts[2]?d->shapeanims[i]:NULL);
            Mtx first;int changed=0;
            for(unsigned frame=0;frame<=60;frame++){
                HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);HSD_JObjSetupMatrix(joint);
                for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)assert(isfinite(joint->mtx[r][c]));
                if(!frame)memcpy(first,joint->mtx,sizeof(first));else if(memcmp(first,joint->mtx,sizeof(first)))changed=1;
            }
            moving+=changed;HSD_JObjRemoveAll(joint);
        }
        if(!strcmp(argv[3],"quake_model_set"))assert(n==4&&moving==4);
        assert(!d->anims||!d->anims[counts[0]]);assert(!d->matanims||!d->matanims[counts[1]]);assert(!d->shapeanims||!d->shapeanims[counts[2]]);
        melee_dynamic_model_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_MtxGetAllocData()->used&&!HSD_VecGetAllocData()->used);
        printf("Dynamic model: %u/%u/%u animation variants sampled through frame 60; owned lifetime and cleanup passed\n",counts[0],counts[1],counts[2]);return 0;
    }
    if(argc==4&&(!strcmp(argv[1],"--stage-light-playback")||!strcmp(argv[1],"--stage-light-colors"))){
        bool color_test=!strcmp(argv[1],"--stage-light-colors");
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==(size_t)size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        MeleeStageModels* owner=melee_stage_models_decode(&a);assert(owner);
        unsigned index=strtoul(argv[3],NULL,10);memset(bytes,0xa5,size);free(bytes);
        struct UnkStageDat_x8_t* model=melee_stage_models_get(owner,index);assert(model&&model->x18);
        unsigned moving=0,colored=0;
        for(unsigned i=0;model->x18[i];i++){
            LightList* entry=model->x18[i];HSD_LObj* light=HSD_LObjLoadDesc(entry->desc);assert(light);
            if(entry->anims&&entry->anims[0]){
                HSD_LObjAddAnim(light,entry->anims[0]);HSD_LObjReqAnim(light,0);HSD_LObjAnim(light);
                GXColor previous_color=light->color;unsigned color_changes=0;
                Vec3 previous;bool has_position=HSD_LObjGetPosition(light,&previous);unsigned changes=0;
                for(unsigned frame=1;frame<=600;frame++){
                    HSD_LObjAnim(light);Vec3 position;
                    if(memcmp(&previous_color,&light->color,sizeof(GXColor)))color_changes++;previous_color=light->color;
                    if(has_position){assert(HSD_LObjGetPosition(light,&position));assert(isfinite(position.x)&&isfinite(position.y)&&isfinite(position.z));
                        if(position.x!=previous.x||position.y!=previous.y||position.z!=previous.z)changes++;
                        previous=position;
                    }
                }
                if(entry->anims[0]->position_anim){assert(changes>0);moving++;}
                if(entry->anims[0]->aobjdesc){assert(color_changes>0);colored++;}
            }
            HSD_LObjRemoveAll(light);
        }
        if(color_test)assert(colored==1);else assert(moving==2);melee_stage_models_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Stage lighting: two moving spline lights, 600 playback frames, source disposal and HSD cleanup passed");return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--stage-empty-camera")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==(size_t)size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));unsigned index=strtoul(argv[3],NULL,10);
        u32 root,table,anim;MeleeHostBool present;
        assert(melee_archive_find(&a,"map_head",&root)&&melee_archive_pointer(&a,root+8,&table,&present)&&present);
        assert(melee_archive_pointer(&a,table+52*index+20,&anim,&present)&&present);
        u32 camera;assert(melee_archive_pointer(&a,table+52*index+16,&camera,&present)&&present);
        MeleeCamera* owner=melee_camera_decode(&a,camera);assert(owner);
        HSD_CameraAnim animdesc;assert(melee_camera_empty_animation_decode(&a,anim,&animdesc));
        for(unsigned i=0;i<3;i++){
            u32 raw;assert(melee_archive_u32(&a,anim+4*i,&raw)&&raw==0);
            bytes[32+anim+4*i+3]=1;
            assert(!melee_camera_empty_animation_decode(&a,anim,&animdesc));
            assert(!animdesc.aobjdesc&&!animdesc.eye_anim&&!animdesc.interest_anim);
            bytes[32+anim+4*i+3]=0;
        }
        MeleeArchive truncated=a;truncated.data_size=anim+11;
        assert(!melee_camera_empty_animation_decode(&truncated,anim,&animdesc));
        memset(bytes,0xa5,size);free(bytes);
        HSD_CObj* cobj=HSD_CObjLoadDesc(melee_camera_descriptor(owner));assert(cobj);
        HSD_CObjAddAnim(cobj,&animdesc);HSD_CObjReqAnim(cobj,0);HSD_CObjAnim(cobj);
        hsdDelete(cobj);melee_camera_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts("Stage empty camera: owned descriptor, invalid fields, source disposal and HSD cleanup passed");return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--stage-models")||!strcmp(argv[1],"--stage-playback"))){
        bool playback=!strcmp(argv[1],"--stage-playback");
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>32);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive archive;assert(melee_archive_open(&archive,bytes,size));
        if(archive.extern_count){
            size_t owned_size;u8* local=melee_archive_copy_null_externals(&archive,&owned_size);assert(local);
            free(bytes);bytes=local;size=owned_size;assert(melee_archive_open(&archive,bytes,size));
        }
        MeleeStageModels* owner=melee_stage_models_decode(&archive);assert(owner);
        u32 header,map_table,disk_count;MeleeHostBool present;
        assert(melee_archive_find(&archive,"map_head",&header));
        assert(melee_archive_pointer(&archive,header,&map_table,&present));
        assert(melee_archive_u32(&archive,header+4,&disk_count));
        GroundJointMapEntry* initial_maps=NULL;unsigned initial_count=0;
        if(melee_stage_models_point_maps(owner,&initial_maps,&initial_count)&&disk_count){
            assert(present&&initial_count>=disk_count);
            unsigned output=0;u32 model_table;assert(melee_archive_pointer(&archive,header+8,&model_table,&present)&&present);
            for(unsigned i=0;i<disk_count;i++){
                u32 joint,pairs,np;assert(melee_archive_pointer(&archive,map_table+12*i,&joint,&present)&&present);
                assert(melee_archive_pointer(&archive,map_table+12*i+4,&pairs,&present));
                assert(melee_archive_u32(&archive,map_table+12*i+8,&np));
                for(unsigned k=0;k<melee_stage_models_count(owner);k++){
                    u32 root;assert(melee_archive_pointer(&archive,model_table+52*k,&root,&present));if(!present||root!=joint)continue;
                    assert(output<initial_count&&initial_maps[output].pair_count==np);
                    assert(initial_maps[output].joint==melee_stage_models_get(owner,k)->unk0);
                    for(unsigned j=0;j<2*np;j++){
                        const u8* d=bytes+32+pairs+2*j;u16 expected=(d[0]<<8)|d[1],actual;
                        memcpy(&actual,initial_maps[output].pairs+j,2);assert(actual==expected);
                    }output++;
                }
            }assert(output==initial_count);
            u32 pairs,np;assert(melee_archive_pointer(&archive,map_table+4,&pairs,&present));assert(melee_archive_u32(&archive,map_table+8,&np));
            if(np){
                for(unsigned field=0;field<2;field++){
                    u8* target=bytes+32+pairs+2*field;u8 save[2]={target[0],target[1]};target[0]=0x7f;target[1]=0xff;
                    MeleeStageModels* bad=melee_stage_models_decode(&archive);assert(bad);
                    GroundJointMapEntry* untouched=initial_maps;unsigned unchanged=initial_count;
                    assert(!melee_stage_models_point_maps(bad,&untouched,&unchanged));
                    assert(untouched==initial_maps&&unchanged==initial_count);melee_stage_models_free(bad);
                    target[0]=save[0];target[1]=save[1];
                }
            }
        }
        u32 overrides=stage_ref(&archive,header+40),override_count,model_table=stage_ref(&archive,header+8);
        assert(melee_archive_u32(&archive,header+44,&override_count));unsigned checked_materials=0;
        for(unsigned k=0;k<melee_stage_models_count(owner);k++){
            struct UnkStageDat_x8_t* m=melee_stage_models_get(owner,k);
            if(m)checked_materials+=check_stage_materials(&archive,stage_ref(&archive,model_table+52*k),m->unk0,overrides,override_count);
        }
        printf("Stage material modes: %u descriptors compared with source plus overrides\n",checked_materials);
        LightOverrideEntry* light_entries=NULL;unsigned light_count=0;
        int lights_okay=melee_stage_models_light_overrides(owner,&light_entries,&light_count);
        if(lights_okay){
            u32 lt=stage_ref(&archive,header+24),words;assert(melee_archive_u32(&archive,header+28,&words));
            unsigned resolved=0;for(unsigned q=0;q<words/2;q++)resolved+=stage_ref(&archive,lt+8*q)!=UINT32_MAX;
            assert(light_count>=resolved);unsigned checked=0;
            for(unsigned k=0;k<melee_stage_models_count(owner);k++){
                struct UnkStageDat_x8_t* m=melee_stage_models_get(owner,k);if(!m||!m->x18)continue;
                u32 dl=stage_ref(&archive,model_table+52*k+24);
                for(unsigned j=0;m->x18[j];j++){
                    u32 entry=stage_ref(&archive,dl+4*j),desc=stage_ref(&archive,entry);
                    for(unsigned q=0;q<words/2;q++)if(stage_ref(&archive,lt+8*q)==desc){
                        u32 flags;assert(melee_archive_u32(&archive,lt+8*q+4,&flags));int found=0;
                        for(unsigned x=0;x<light_count;x++)if(light_entries[x].desc==m->x18[j]->desc){
                            assert(light_entries[x].a==((flags>>31)&1)&&light_entries[x].b==((flags>>30)&1)&&light_entries[x].c==((flags>>29)&1));found=1;
                        }assert(found);checked++;
                    }
                }
            }
            u8* counter=bytes+32+header+28;u8 save=counter[3];counter[3]|=1;
            MeleeStageModels* bad=melee_stage_models_decode(&archive);assert(bad);
            LightOverrideEntry* unchanged=light_entries;unsigned unchanged_count=light_count;
            assert(!melee_stage_models_light_overrides(bad,&unchanged,&unchanged_count));
            assert(unchanged==light_entries&&unchanged_count==light_count);melee_stage_models_free(bad);counter[3]=save;
            printf("Stage light overrides: %u owned entries, %u model light identities/flags checked\n",light_count,checked);
        }else fprintf(stderr,"Stage light overrides unsupported\n");
        u32 expected_splines,expected_shadows,shadow_table=stage_ref(&archive,header+32),shadow_flags[4096];
        assert(melee_archive_u32(&archive,header+20,&expected_splines)&&melee_archive_u32(&archive,header+36,&expected_shadows)&&expected_shadows<=4096);
        for(unsigned i=0;i<expected_shadows;i++)assert(melee_archive_u32(&archive,shadow_table+8*i+4,&shadow_flags[i]));
        memset(bytes,0xa5,size);free(bytes);
        if(lights_okay){LightOverrideEntry* again=NULL;unsigned count=0;assert(melee_stage_models_light_overrides(owner,&again,&count)&&again==light_entries&&count==light_count);}
        unsigned n=melee_stage_models_count(owner),loaded=0;
        for(unsigned i=0;i<n;i++){
            struct UnkStageDat_x8_t* model=melee_stage_models_get(owner,i);
            if(!model){fprintf(stderr,"Stage model %u/%u unsupported\n",i,n);continue;}
            assert(model==melee_stage_models_get(owner,i));
            if(model->unk0){
                HSD_JObj* joint=HSD_JObjLoadJoint(model->unk0);assert(joint);
                if(playback)grAnime_801C6C0C(joint,model->unk4?model->unk4[0]:NULL,
                    model->unk8?model->unk8[0]:NULL,model->unkC?model->unkC[0]:NULL);
                else HSD_JObjAddAnimAll(joint,model->unk4?model->unk4[0]:NULL,
                    model->unk8?model->unk8[0]:NULL,model->unkC?model->unkC[0]:NULL);
                HSD_JObjReqAnimAll(joint,0);HSD_JObjAnimAll(joint);
                if(playback){fprintf(stderr,"Stage playback model %u\n",i);for(unsigned frame=1;frame<=120;frame++)HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);
            }
            loaded++;
        }
        GroundJointMapEntry* maps=NULL;unsigned map_count=0;
        int maps_okay=melee_stage_models_point_maps(owner,&maps,&map_count);
        if(maps_okay){
            for(unsigned i=0;i<map_count;i++){
                int found=0;for(unsigned k=0;k<n;k++){
                    struct UnkStageDat_x8_t* m=melee_stage_models_get(owner,k);
                    if(m&&m->unk0==maps[i].joint)found=1;
                }assert(found);
            }
            GroundJointMapEntry* again=NULL;unsigned again_count=0;
            assert(melee_stage_models_point_maps(owner,&again,&again_count)&&again==maps&&again_count==map_count);
            printf("Stage point mappings: %u native model references passed\n",map_count);
        }else fprintf(stderr,"Stage point mappings unsupported\n");
        UnkStageDat* head=melee_stage_models_header(owner);
        if(head){
            assert(head==melee_stage_models_header(owner)&&head->unkC==n);
            assert(head->unk0==maps&&head->unk4==map_count&&head->unk18==light_entries&&head->unk1C==light_count);
            assert(head->unk14==expected_splines&&head->unk24==expected_shadows&&!head->unk28&&!head->unk2C);
            for(unsigned i=0;i<expected_splines;i++){
                assert(head->unk10[i]);for(unsigned step=0;step<=100;step++){Vec3 point;splArcLengthPoint(&point,head->unk10[i],step/100.0f);assert(isfinite(point.x)&&isfinite(point.y)&&isfinite(point.z));}
            }
            for(unsigned i=0;i<expected_shadows;i++){
                assert(head->unk20[i].flag==(shadow_flags[i]>>31));int found=0;
                for(unsigned j=0;j<n;j++)for(unsigned k=0;head->unk8[j].x18&&head->unk8[j].x18[k];k++){
                    HSD_LightAnim** animations=head->unk8[j].x18[k]->anims;
                    for(unsigned q=0;animations&&animations[q];q++)if(animations[q]==head->unk20[i].unk0)found=1;
                }assert(found);
            }
            printf("Stage header: %u sampled splines, %u shared shadow-animation records passed\n",expected_splines,expected_shadows);
            for(unsigned i=0;i<n;i++)assert(!memcmp(&head->unk8[i],melee_stage_models_get(owner,i),sizeof(head->unk8[i])));
            printf("Stage map_head: %u contiguous native model records passed\n",n);
        }else fprintf(stderr,"Stage map_head unsupported\n");
        melee_stage_models_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_MtxGetAllocData()->used&&!HSD_VecGetAllocData()->used);
        printf("Stage models: %u/%u decoded; owned lifetime and HSD cleanup passed\n",loaded,n);return loaded==n&&maps_okay&&lights_okay&&head?0:2;
    }
    if(argc==4&&!strcmp(argv[1],"--effects")){
        FILE* f=fopen(argv[2],"rb");assert(f);assert(!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive archive;assert(melee_archive_open(&archive,bytes,size));
        MeleeEffects* e=melee_effects_decode(&archive,argv[3]);assert(e);
        if(!strcmp(argv[3],"effCommonDataTable")){
            u32 table;assert(melee_archive_find(&archive,argv[3],&table));u32 root=stage_ref(&archive,table+8+36*20+4);
            MeleeJointGraph* graph=melee_joint_decode(&archive,root);assert(graph);unsigned splines=0;
            for(size_t k=0;k<melee_joint_count(graph);k++){
                const MeleeJointNode* node=melee_joint_node(graph,k);if(!(node->flags&JOBJ_SPLINE))continue;splines++;
                u32 spline=node->payload_offset,header,saved;assert(melee_archive_u32(&archive,spline,&header));
                u32 lengths=stage_ref(&archive,spline+16),end=lengths+4*((header&65535)-1),cv=stage_ref(&archive,spline+8);
                assert(melee_archive_u32(&archive,end,&saved));word(bytes+32+end,0x3f000000);assert(!melee_scene_decode(&archive,root));word(bytes+32+end,saved);
                assert(melee_archive_u32(&archive,cv,&saved));word(bytes+32+cv,0x7fc00000);assert(!melee_scene_decode(&archive,root));word(bytes+32+cv,saved);
            }
            assert(splines==1);melee_joint_free(graph);
        }
        memset(bytes,0xa5,size);free(bytes);
        unsigned n=melee_effects_count(e),loaded=0;
        if(!strcmp(argv[3],"effNessDataTable"))assert(n==3&&!melee_effects_particles(e));
        if(!strcmp(argv[3],"effKirbyFoxDataTable"))assert(n==1&&!melee_effects_particles(e));
        if(!strcmp(argv[3],"effKirbySamusDataTable")){
            assert(n==1&&melee_effects_particles(e));
            HSD_PSTexGroup* tg=melee_particle_bank_textures(melee_effects_particles(e))[0];
            assert(tg->num==1&&tg->fmt==9);
            assert(!melee_particle_bank_palette_resolved(tg->texTable[1]));
        }
        if(!strcmp(argv[3],"effKirbyIceDataTable"))assert(n==1&&melee_effects_particles(e));
        if(!strcmp(argv[3],"effKirbyDonkeyDataTable")||!strcmp(argv[3],"effKirbyMarsDataTable")||!strcmp(argv[3],"effKirbyEmblemDataTable")||!strcmp(argv[3],"effKirbyZeldaDataTable"))assert(n==2&&!melee_effects_particles(e));
        if(!strcmp(argv[3],"effKirbyCaptainDataTable")||!strcmp(argv[3],"effKirbyGanonDataTable"))assert(n==2&&melee_effects_particles(e));
        for(unsigned i=0;i<n;i++){
            EF_EffectDesc* d=melee_effects_model(e,i);
            if(!d){fprintf(stderr,"Effects model %u/%u unsupported\n",i,n);continue;}
            assert(d==melee_effects_model(e,i));loaded++;
            if(i==36&&!strcmp(argv[3],"effCommonDataTable")){
                StaticModelDesc* m=&d->model_desc;
                HSD_JObj* joint=HSD_JObjLoadJoint(m->joint);assert(joint);
                HSD_JObjAddAnimAll(joint,m->animjoint,m->matanim_joint,m->shapeanim_joint);
                HSD_JObjReqAnimAll(joint,0);HSD_JObjAnimAll(joint);
                Mtx first[256],current[256];size_t count=0;matrices(joint,first,&count);assert(count<=256);bool changed=false;
                for(unsigned frame=1;frame<=120;frame++){
                    HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);size_t n=0;matrices(joint,current,&n);assert(n==count);
                    if(memcmp(first,current,n*sizeof(Mtx)))changed=true;
                    for(size_t k=0;k<n;k++)for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)assert(isfinite(current[k][r][c]));
                }
                assert(changed);HSD_JObjRemoveAll(joint);
                puts("Common effect 36: source-free spline animation, 120 frames and changing finite matrices passed");
            }
        }
        melee_effects_free(e);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_MtxGetAllocData()->used&&!HSD_VecGetAllocData()->used);
        printf("Effects: %u/%u lazy models decoded; owned particle banks and HSD cleanup passed\n",loaded,n);
        return loaded==n?0:2;
    }
    if(argc==3&&(!strcmp(argv[1],"--character-select")||!strcmp(argv[1],"--stage-select"))){
        bool stage=!strcmp(argv[1],"--stage-select");
        const char* symbol=stage?"MnSelectStageDataTable":"MnSelectChrDataTable";
        unsigned count=stage?12:9;
        FILE* f=fopen(argv[2],"rb");assert(f);assert(!fseek(f,0,SEEK_END));
        long size=ftell(f);assert(size>32);rewind(f);uint8_t* bytes=malloc(size);assert(bytes);
        assert(fread(bytes,1,size,f)==(size_t)size);fclose(f);
        MeleeArchive archive;assert(melee_archive_open(&archive,bytes,size));
        HSD_Archive* css=stage?melee_stage_selection_decode(&archive):melee_character_select_decode(&archive);assert(css);
        uint32_t root,fog,old;MeleeHostBool present;
        assert(melee_archive_find(&archive,symbol,&root));
        assert(melee_archive_pointer(&archive,root+12,&fog,&present)&&present);
        assert(melee_archive_u32(&archive,fog+8,&old));word(bytes+32+fog+8,0x7fc00000);
        assert(!(stage?melee_stage_selection_decode(&archive):melee_character_select_decode(&archive)));word(bytes+32+fog+8,old);
        assert(melee_archive_u32(&archive,root+16+16*(count-1),&old));word(bytes+32+root+16+16*(count-1),archive.data_size);
        assert(!(stage?melee_stage_selection_decode(&archive):melee_character_select_decode(&archive)));word(bytes+32+root+16+16*(count-1),old);

        if(stage){
            MeleeStageSelectionData* descriptors=HSD_ArchiveGetPublicAddress(css,symbol);
            uint32_t model;assert(melee_archive_u32(&archive,root+16+16*10,&model));
            assert(check_morph_data(&archive,model,descriptors->animations[10].joint)==2);
        }
        memset(bytes,0xa5,size);free(bytes);
        void* data=HSD_ArchiveGetPublicAddress(css,symbol);assert(data);
        MeleeCSSModels* models=stage?&((MeleeStageSelectionData*)data)->models:&((MeleeCSSData*)data)->models;
        MeleeCSSAnimation* animations=stage?((MeleeStageSelectionData*)data)->animations:((MeleeCSSData*)data)->animations;
        assert(data==HSD_ArchiveGetPublicAddress(css,symbol));
        assert(!HSD_ArchiveGetPublicAddress(css,"missing"));
        HSD_CObj* camera=HSD_CObjLoadDesc(models->cam);assert(camera);HSD_OBJECT_METHOD(camera)->release((HSD_Class*)camera);HSD_OBJECT_METHOD(camera)->destroy((HSD_Class*)camera);
        for(unsigned i=0;i<2;i++){HSD_LObj* light=HSD_LObjLoadDesc(i==0?models->light0:models->light1);assert(light);HSD_LObjRemoveAll(light);}
        for(unsigned i=0;i<count;i++){
            MeleeCSSAnimation* a=&animations[i];

            HSD_JObj* j=HSD_JObjLoadJoint(a->joint);assert(j);
            HSD_JObjAddAnimAll(j,a->anim,a->matanim,a->shapeanim);
            /* Stage icon bundle 2 is a seven-entry selector. The game uses
             * stage ids minus 0x16; later timeline frames are not valid icons. */
            unsigned last=stage&&i==2?6:1600,step=stage&&i==2?1:20;
            for(unsigned frame=0;frame<=last;frame+=step){
                HSD_JObjReqAnimAll(j,frame);HSD_JObjAnimAll(j);size_t d=0,p=0,m=0;inspect(j,&d,&p,&m);
            }
            HSD_JObjRemoveAll(j);
        }
        css->native_destroy(css);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        printf("Selection scene: %u animated model bundles, camera/lights and owned lifetime passed\n",count);return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--menu-archive")||!strcmp(argv[1],"--menu-extra")||!strcmp(argv[1],"--trophy-archive"))){
        bool trophy=!strcmp(argv[1],"--trophy-archive");
        bool extra=trophy||!strcmp(argv[1],"--menu-extra");
        FILE* f=fopen(argv[2],"rb");assert(f);assert(!fseek(f,0,SEEK_END));
        long size=ftell(f);assert(size>32);rewind(f);uint8_t* bytes=malloc(size);assert(bytes);
        assert(fread(bytes,1,size,f)==(size_t)size);fclose(f);
        MeleeArchive archive;assert(melee_archive_open(&archive,bytes,size));
        HSD_Archive* menu=trophy?melee_trophy_decode(&archive):melee_menu_decode(&archive);assert(menu);
        if(!extra){
        uint32_t mat_root,mat,next;MeleeHostBool present;
        assert(melee_archive_find(&archive,"MenMainPhotoSn_Top_matanim_joint",&mat_root));
        assert(melee_archive_pointer(&archive,mat_root+8,&mat,&present)&&present);
        assert(melee_archive_u32(&archive,mat,&next));word(bytes+32+mat,mat);
        HSD_Archive* bad=melee_menu_decode(&archive);assert(bad);
        for(unsigned attempt=0;attempt<3;attempt++)
            assert(!HSD_ArchiveGetPublicAddress(bad,"MenMainPhotoSn_Top_joint"));
        bad->native_destroy(bad);word(bytes+32+mat,next);
        }
        memset(bytes,0xa5,size);free(bytes);
        unsigned models=0;
        for(unsigned i=0;i<menu->header.nb_public;i++){
            const char* name=menu->symbols+menu->public_info[i].symbol;size_t n=strlen(name);
            if(!((n>10&&!strcmp(name+n-10,"_Top_joint"))||(trophy&&n>11&&!strcmp(name+n-11,"_TopN_joint"))))continue;
            HSD_Joint* joint=HSD_ArchiveGetPublicAddress(menu,name);assert(joint);
            assert(joint==HSD_ArchiveGetPublicAddress(menu,name));
            char base[96],symbol[128];assert(n-6<sizeof(base));memcpy(base,name,n-6);base[n-6]=0;
            snprintf(symbol,sizeof(symbol),"%s_animjoint",base);HSD_AnimJoint* a=HSD_ArchiveGetPublicAddress(menu,symbol);if(!trophy)assert(a);
            snprintf(symbol,sizeof(symbol),"%s_matanim_joint",base);HSD_MatAnimJoint* m=HSD_ArchiveGetPublicAddress(menu,symbol);if(!trophy)assert(m);
            snprintf(symbol,sizeof(symbol),"%s_shapeanim_joint",base);HSD_ShapeAnimJoint* sh=HSD_ArchiveGetPublicAddress(menu,symbol);if(!trophy)assert(sh);
            HSD_JObj* j=HSD_JObjLoadJoint(joint);assert(j);HSD_JObjAddAnimAll(j,a,m,sh);
            for(unsigned frame=0;frame<=1600;frame+=100){
                HSD_JObjReqAnimAll(j,frame);HSD_JObjAnimAll(j);size_t d=0,p=0,mat=0;inspect(j,&d,&p,&mat);
            }
            HSD_JObjRemoveAll(j);models++;
        }
        if(trophy)assert(models>0);else assert(models==(extra?24:57));
        if(!trophy){
        assert(HSD_ArchiveGetPublicAddress(menu,"ScMenMain_cam_int1_camera"));
        assert(HSD_ArchiveGetPublicAddress(menu,"ScMenMain_scene_lights"));
        assert(HSD_ArchiveGetPublicAddress(menu,"ScMenMain_fog"));
        }
        assert(!HSD_ArchiveGetPublicAddress(menu,"missing_symbol"));
        assert(!HSD_ArchiveGetPublicAddress(menu,"mnNameAutoName"));
        menu->native_destroy(menu);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        printf("Menu archive: %u lazy model bundles, original lookups, animation sampling and owned lifetime passed\n",models);return 0;
    }
    if(argc==4&&!strcmp(argv[1],"--animated-model")){
        FILE* f=fopen(argv[2],"rb");assert(f);assert(!fseek(f,0,SEEK_END));
        long size=ftell(f);assert(size>32);rewind(f);uint8_t* bytes=malloc(size);assert(bytes);
        assert(fread(bytes,1,size,f)==(size_t)size);fclose(f);
        MeleeArchive archive;assert(melee_archive_open(&archive,bytes,size));
        char symbol[128];uint32_t root;
        snprintf(symbol,sizeof(symbol),"%s_joint",argv[3]);assert(melee_archive_find(&archive,symbol,&root));
        MeleeScene* owner=melee_scene_decode(&archive,root);assert(owner);
        const char* suffixes[]={"animjoint","matanim_joint","shapeanim_joint"};
        for(unsigned i=0;i<3;i++){
            snprintf(symbol,sizeof(symbol),"%s_%s",argv[3],suffixes[i]);
            assert(melee_archive_find(&archive,symbol,&root));
            bool bound=i==0?melee_scene_bind_joints(owner,root):i==1?
                melee_scene_bind_materials(owner,root):melee_scene_bind_empty_shapes(owner,root);
            if(!bound){fprintf(stderr,"Cannot bind %s\n",symbol);melee_scene_free(owner);free(bytes);return 2;}
        }
        melee_scene_release_objects(owner);memset(bytes,0xa5,size);free(bytes);
        HSD_JObj* j=HSD_JObjLoadJoint(melee_scene_joint_descriptor(owner));assert(j);
        HSD_JObjAddAnimAll(j,melee_scene_animation_descriptor(owner),
            melee_scene_material_descriptor(owner),melee_scene_shape_descriptor(owner));
        for(unsigned frame=0;frame<=1600;frame+=20){
            HSD_JObjReqAnimAll(j,frame);HSD_JObjAnimAll(j);
            size_t d=0,p=0,m=0;inspect(j,&d,&p,&m);
        }
        HSD_JObjRemoveAll(j);melee_scene_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        printf("Animated model %s: owned descriptors and animation sampling passed\n",argv[3]);return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--title-models")){
        FILE* f=fopen(argv[2],"rb");assert(f);assert(!fseek(f,0,SEEK_END));
        long size=ftell(f);assert(size>32);rewind(f);uint8_t* bytes=malloc(size);assert(bytes);
        assert(fread(bytes,1,size,f)==(size_t)size);fclose(f);
        MeleeArchive archive;assert(melee_archive_open(&archive,bytes,size));
        MeleeTitle* title=melee_title_decode(&archive);assert(title);
        uint32_t fog_root,fog_start;
        assert(melee_archive_find(&archive,"ScTitle_fog",&fog_root));
        assert(melee_archive_u32(&archive,fog_root+8,&fog_start));
        word(bytes+32+fog_root+8,0x7fc00000);
        for(unsigned attempt=0;attempt<3;attempt++)assert(!melee_title_decode(&archive));
        word(bytes+32+fog_root+8,fog_start);
        uint32_t mark_root,image_root,pixel_root;MeleeHostBool present;
        assert(melee_archive_find(&archive,"TitleMark_sobjdesc",&mark_root));
        assert(melee_archive_pointer(&archive,mark_root,&image_root,&present)&&present);
        assert(melee_archive_pointer(&archive,image_root,&pixel_root,&present)&&present);
        HSD_ImageDesc* mark_image=melee_title_data(title)->mark.image;
        size_t mark_size=melee_texture_level_size(mark_image->width,mark_image->height,mark_image->format);
        assert(mark_size);uint8_t* mark_copy=malloc(mark_size);assert(mark_copy);
        memcpy(mark_copy,bytes+32+pixel_root,mark_size);
        word(bytes+32+image_root,archive.data_size);
        for(unsigned attempt=0;attempt<3;attempt++)assert(!melee_title_decode(&archive));
        word(bytes+32+image_root,pixel_root);
        MeleeScene* owners[2]={0};const char* names[]={"TtlMoji_Top","TtlBg_Top"};
        for(unsigned i=0;i<2;i++){
            char symbol[80];uint32_t root;
            snprintf(symbol,sizeof(symbol),"%s_joint",names[i]);
            assert(melee_archive_find(&archive,symbol,&root));
            owners[i]=melee_scene_decode(&archive,root);assert(owners[i]);
            snprintf(symbol,sizeof(symbol),"%s_animjoint",names[i]);
            assert(melee_archive_find(&archive,symbol,&root));
            assert(melee_scene_bind_joints(owners[i],root));
            snprintf(symbol,sizeof(symbol),"%s_matanim_joint",names[i]);
            assert(melee_archive_find(&archive,symbol,&root));
            assert(melee_scene_bind_materials(owners[i],root));
            snprintf(symbol,sizeof(symbol),"%s_shapeanim_joint",names[i]);
            assert(melee_archive_find(&archive,symbol,&root));
            /* A cycle must fail without leaking validation objects or preventing
             * subsequent use of an independent valid owner. */
            uint32_t child;assert(melee_archive_u32(&archive,root,&child));
            word(bytes+32+root,root);
            char joint_symbol[80];uint32_t joint_root;
            snprintf(joint_symbol,sizeof(joint_symbol),"%s_joint",names[i]);
            assert(melee_archive_find(&archive,joint_symbol,&joint_root));
            for(unsigned attempt=0;attempt<3;attempt++){
                MeleeScene* bad=melee_scene_decode(&archive,joint_root);assert(bad);
                assert(!melee_scene_bind_empty_shapes(bad,root));melee_scene_free(bad);
            }
            word(bytes+32+root,child);
            assert(melee_scene_bind_empty_shapes(owners[i],root));
            melee_scene_release_objects(owners[i]);
        }
        memset(bytes,0xa5,size);free(bytes);
        for(unsigned i=0;i<2;i++){
            HSD_JObj* j=HSD_JObjLoadJoint(melee_scene_joint_descriptor(owners[i]));assert(j);
            HSD_JObjAddAnimAll(j,melee_scene_animation_descriptor(owners[i]),
                melee_scene_material_descriptor(owners[i]),melee_scene_shape_descriptor(owners[i]));
            Mtx initial[256],current[256];size_t count=0;int changed=0,texture_changed=0;
            TextureFrame first_textures[128],current_textures[128];size_t textures=0;
            HSD_JObjReqAnimAll(j,0);HSD_JObjAnimAll(j);matrices(j,initial,&count);assert(count<=256);
            texture_frames(j,first_textures,&textures);assert(textures);
            for(unsigned frame=10;frame<=1600;frame+=10){
                HSD_JObjReqAnimAll(j,frame);HSD_JObjAnimAll(j);
                size_t n=0;matrices(j,current,&n);assert(n==count);
                if(memcmp(initial,current,count*sizeof(Mtx)))changed=1;
                size_t nt=0;texture_frames(j,current_textures,&nt);assert(nt==textures);
                if(memcmp(first_textures,current_textures,nt*sizeof(TextureFrame)))texture_changed=1;
            }
            assert(changed&&texture_changed);HSD_JObjRemoveAll(j);melee_scene_free(owners[i]);
        }
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        MeleeTitleData* title_data=melee_title_data(title);
        assert(title_data->camera&&title_data->lights[0]&&title_data->mark.image);
        assert(title_data->fog.start==80&&title_data->fog.end==300);
        HSD_CObj* camera=HSD_CObjLoadDesc(title_data->camera);assert(camera);
        HSD_OBJECT_METHOD(camera)->release((HSD_Class*)camera);
        HSD_OBJECT_METHOD(camera)->destroy((HSD_Class*)camera);
        for(unsigned i=0;title_data->lights[i];i++){
            HSD_LObj* l=HSD_LObjLoadDesc(title_data->lights[i]->desc);assert(l);HSD_LObjRemoveAll(l);
        }
        assert(title_data->mark.image->width>0&&title_data->mark.image->height>0);
        assert(!memcmp(mark_copy,title_data->mark.image->image_ptr,mark_size));free(mark_copy);
        melee_title_free(title);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        puts("Title bundle: owned models, animations, camera, lights, fog and mark passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--message-scene")){
        FILE* f=fopen(argv[2],"rb");assert(f);assert(!fseek(f,0,SEEK_END));
        long size=ftell(f);assert(size>32);rewind(f);uint8_t* bytes=malloc(size);assert(bytes);
        assert(fread(bytes,1,size,f)==(size_t)size);fclose(f);
        MeleeArchive archive;uint32_t root;
        assert(melee_archive_open(&archive,bytes,size));
        assert(melee_archive_find(&archive,"ScNtcCommon_scene_data",&root));
        MeleeSceneDesc* owner=melee_scene_desc_decode(&archive,root);assert(owner);
        uint32_t models,model,anims,anim,child;MeleeHostBool present;
        assert(melee_archive_pointer(&archive,root,&models,&present)&&present);
        assert(melee_archive_pointer(&archive,models+4,&model,&present)&&present);
        assert(melee_archive_pointer(&archive,model+4,&anims,&present)&&present);
        assert(melee_archive_pointer(&archive,anims,&anim,&present)&&present);
        assert(melee_archive_u32(&archive,anim,&child));
        word(bytes+32+anim,anim);
        for(unsigned attempt=0;attempt<3;attempt++)assert(!melee_scene_desc_decode(&archive,root));
        word(bytes+32+anim,child);

        memset(bytes,0xa5,size);free(bytes);
        SceneDesc* desc=melee_scene_desc_data(owner);
        assert(desc->models[0]&&desc->models[1]&&!desc->models[2]);
        size_t joints=0,drawables=0,polygons=0,materials=0;int changed=0;
        for(unsigned i=0;i<2;i++){
            DynamicModelDesc* model=desc->models[i];
            HSD_JObj* j=HSD_JObjLoadJoint(model->joint);assert(j);
            HSD_JObjAddAnimAll(j,model->anims?model->anims[0]:NULL,NULL,NULL);
            HSD_JObjReqAnimAll(j,0);HSD_JObjAnimAll(j);
            Mtx initial[256],current[256];size_t count=0;matrices(j,initial,&count);assert(count<=256);
            for(unsigned frame=1;frame<=60;frame++){
                HSD_JObjReqAnimAll(j,frame);HSD_JObjAnimAll(j);
                size_t n=0;matrices(j,current,&n);assert(n==count);
                if(memcmp(initial,current,count*sizeof(Mtx)))changed=1;
            }
            joints+=inspect(j,&drawables,&polygons,&materials);HSD_JObjRemoveAll(j);
        }
        HSD_CObj* camera=HSD_CObjLoadDesc(desc->cameras[0].desc);assert(camera);
        HSD_OBJECT_METHOD(camera)->release((HSD_Class*)camera);
        HSD_OBJECT_METHOD(camera)->destroy((HSD_Class*)camera);
        for(unsigned i=0;desc->lights&&desc->lights[i];i++){
            HSD_LObj* l=HSD_LObjLoadDesc(desc->lights[i]->desc);assert(l);HSD_LObjRemoveAll(l);
        }
        assert(changed);melee_scene_desc_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        printf("Message scene: two models, %zu joints, %zu polygons, changing joint animation and owned lifetime passed\n",joints,polygons);
        return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--hud")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        HSD_Archive* owner=melee_hud_decode(&a);assert(owner);
        u32 table,saved;assert(melee_archive_find(&a,"ScInfCnt_scene_models",&table));
        assert(melee_archive_u32(&a,table+7*4,&saved));word(bytes+32+table+7*4,0);
        assert(!melee_hud_decode(&a));word(bytes+32+table+7*4,saved);
        memset(bytes,0xa5,size);free(bytes);
        const char* names[]={"DmgMrk_scene_models","DmgNum_scene_models","ScInfCnt_scene_models","ScInfPnm_scene_models","ScInfStc_scene_models","ScInfTim_scene_models","Stc_rarwmdls","Stc_scemdls","lupe","tdsce"};
        unsigned models=0,animations=0;
        for(unsigned i=0;i<10;i++){DynamicModelDesc** table=HSD_ArchiveGetPublicAddress(owner,names[i]);assert(table);
            for(unsigned k=0;table[k];k++){DynamicModelDesc* m=table[k];HSD_JObj* j=HSD_JObjLoadJoint(m->joint);assert(j);
                if(i==1){
                    HSD_JObj* child=j->child;
                    for(unsigned digit=0;digit<4;digit++){assert(child&&ifStatus_802F6194((HSD_GObj*)j,digit)==(HSD_GObj*)child);child=child->next;}
                    assert(!ifStatus_802F6194((HSD_GObj*)j,-1)&&!ifStatus_802F6194(NULL,0));
                    HSD_JObj* percent=(HSD_JObj*)ifStatus_802F6194((HSD_GObj*)j,3);
                    HSD_TObj* texture=percent->u.dobj->mobj->tobj;
                    HSD_TObjAddAnimAll(texture,m->matanims[0]->child->next->next->next->matanim->texanim);
                    HSD_TObjReqAnimAll(texture,1);HSD_TObjAnim(texture);HSD_TObjRemoveAnimAll(texture);
                }
                for(unsigned kind=0;kind<3;kind++){
                    void** list=kind==0?(void**)m->anims:kind==1?(void**)m->matanims:(void**)m->shapeanims;
                    for(unsigned n=0;list&&list[n];n++){
                        HSD_JObjAddAnimAll(j,kind==0?list[n]:NULL,kind==1?list[n]:NULL,kind==2?list[n]:NULL);
                        for(unsigned frame=0;frame<3;frame++){HSD_JObjReqAnimAll(j,frame);HSD_JObjAnimAll(j);}
                        HSD_JObjRemoveAnimAll(j);animations++;
                    }
                }
                HSD_JObjRemoveAll(j);models++;}}
        assert(HSD_ArchiveGetPublicAddress(owner,"ScInfDmg_scene_data"));assert(!HSD_ArchiveGetPublicAddress(owner,"missing"));
        owner->native_destroy(owner);assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("HUD: %u models in ten tables, %u animation variants, damage scene, source-free reloads and cleanup passed\n",models,animations);return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--intro-archive")||!strcmp(argv[1],"--score-archive"))){
        bool score=!strcmp(argv[1],"--score-archive");
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        assert(a.public_count<=64);char names[64][128];unsigned count=a.public_count;
        for(unsigned i=0;i<count;i++){const char* name;u32 root;assert(melee_archive_public(&a,i,&name,&root));assert(strlen(name)<128);strcpy(names[i],name);}
        HSD_Archive* bridge=score?melee_single_scene_decode(&a,"ScGamRegClear_scene_data"):melee_intro_decode(&a);assert(bridge);
        memset(bytes,0xa5,size);free(bytes);
        unsigned motions=0;
        for(unsigned i=0;i<count;i++){
            void* data=HSD_ArchiveGetPublicAddress(bridge,names[i]);assert(data);
            assert(data==HSD_ArchiveGetPublicAddress(bridge,names[i]));
            if(!strcmp(names[i],score?"ScGamRegClear_scene_data":"ScItrAllstar_scene_data")){
                SceneDesc* scene=data;assert(scene->models&&scene->models[0]);
                DynamicModelDesc* model=scene->models[0];
                HSD_JObj* joint=HSD_JObjLoadJoint(model->joint);assert(joint);
                HSD_JObjAddAnimAll(joint,model->anims?model->anims[0]:NULL,
                    model->matanims?model->matanims[0]:NULL,model->shapeanims?model->shapeanims[0]:NULL);
                for(unsigned frame=0;frame<120;frame++){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);
            }else{
                const u8* b=data;u32 length=((u32)b[0]<<24)|((u32)b[1]<<16)|((u32)b[2]<<8)|b[3];
                MeleeArchive nested;assert(melee_archive_open(&nested,data,length));assert(nested.public_count);motions++;
            }
        }
        assert(!HSD_ArchiveGetPublicAddress(bridge,"missing"));bridge->native_destroy(bridge);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("Intro archive: scene and %u embedded motion archives survive source disposal and clean up\n",motions);return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--results-scenes")||!strcmp(argv[1],"--prize-scene")||!strcmp(argv[1],"--intro-scene"))){
        bool intro=!strcmp(argv[1],"--intro-scene");
        bool prize=intro||!strcmp(argv[1],"--prize-scene");unsigned count=prize?1:2;
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        const char* names[]={intro?"ScItrAllstar_scene_data":prize?"ScInfPrize_scene_data":"pnlsce","flmsce"};MeleeSceneDesc* owners[2];
        for(unsigned i=0;i<count;i++){u32 root;assert(melee_archive_find(&a,names[i],&root));owners[i]=melee_scene_desc_decode(&a,root);assert(owners[i]);}
        memset(bytes,0xa5,size);free(bytes);
        unsigned models=0;
        for(unsigned i=0;i<count;i++){
            SceneDesc* scene=melee_scene_desc_data(owners[i]);assert(scene->models);
            if(!i)assert(scene->cameras&&scene->cameras->desc&&(intro||scene->lights));
            for(unsigned k=0;scene->models[k];k++){
                DynamicModelDesc* m=scene->models[k];HSD_JObj* j=HSD_JObjLoadJoint(m->joint);assert(j);
                HSD_JObjAddAnimAll(j,m->anims?m->anims[0]:NULL,m->matanims?m->matanims[0]:NULL,m->shapeanims?m->shapeanims[0]:NULL);
                for(unsigned frame=0;frame<120;frame++){HSD_JObjReqAnimAll(j,frame);HSD_JObjAnimAll(j);}
                HSD_JObjRemoveAll(j);models++;
            }
            melee_scene_desc_free(owners[i]);
        }
        assert(models>=(prize?1:5)&&!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("%s scenes: %u models, 120 animation frames, source disposal and HSD cleanup passed\n",prize?"Prize":"Results",models);return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--pause-scene")||!strcmp(argv[1],"--coget-scene"))){
        bool coget=!strcmp(argv[1],"--coget-scene");
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;u32 root;assert(melee_archive_open(&a,bytes,size)&&melee_archive_find(&a,coget?"ScInfCgt_scene_data":"ScGamPause_scene_data",&root));
        MeleeSceneDesc* owner=melee_scene_desc_decode(&a,root);assert(owner);
        u32 fogs=stage_ref(&a,root+12);float start=0,end=0;
        if(fogs!=UINT32_MAX){u32 fog=stage_ref(&a,fogs),saved;
        assert(melee_archive_u32(&a,fog+8,&saved));word(bytes+32+fog+8,0x7fc00000);
        assert(!melee_scene_desc_decode(&a,root));word(bytes+32+fog+8,saved);
        assert(melee_archive_f32(&a,fog+8,&start)&&melee_archive_f32(&a,fog+12,&end));}
        memset(bytes,0xa5,size);free(bytes);SceneDesc* d=melee_scene_desc_data(owner);
        if(fogs==UINT32_MAX)assert(!d->fogs);else assert(d->fogs&&d->fogs->desc&&d->fogs->desc->start==start&&d->fogs->desc->end==end&&!d->fogs->anims);
        assert(d->models&&d->models[0]&&!d->models[1]);DynamicModelDesc* m=d->models[0];
        HSD_JObj* j=HSD_JObjLoadJoint(m->joint);assert(j);
        HSD_JObjAddAnimAll(j,m->anims?m->anims[0]:NULL,m->matanims?m->matanims[0]:NULL,m->shapeanims?m->shapeanims[0]:NULL);
        for(unsigned frame=0;frame<(coget?101:5);frame++){HSD_JObjReqAnimAll(j,frame);HSD_JObjAnimAll(j);}
        HSD_JObjRemoveAll(j);melee_scene_desc_free(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        puts(coget?"Coin-get scene: source-free model load, 101 count frames and HSD cleanup passed":"Pause scene: source-free model load, five background selection frames and HSD cleanup passed");return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--trophy-files")){
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        HSD_Archive* bridge=melee_trophy_files_decode(&a);assert(bridge);
        const char* names[]={"tyModelFileTbl","tyModelFileUsTbl"};const unsigned counts[]={293,5};
        for(unsigned i=0;i<2;i++){
            u32 at;assert(melee_archive_find(&a,names[i],&at));u8* table=HSD_ArchiveGetPublicAddress(bridge,names[i]);assert(table);
            for(unsigned j=0;j<counts[i];j++){
                u32 expected,actual;assert(melee_archive_u32(&a,at+84*j,&expected));memcpy(&actual,table+84*j,4);assert(actual==expected);
                assert(!memcmp(table+84*j+4,bytes+32+at+84*j+4,80));
            }
        }
        memset(bytes,0xa5,size);free(bytes);
        assert(!strcmp((char*)HSD_ArchiveGetPublicAddress(bridge,names[0])+4,"TyMario.dat"));
        assert(melee_trophy_files_contains(bridge,"TyMario.dat"));assert(!melee_trophy_files_contains(bridge,"TyDataf.dat"));
        bridge->native_destroy(bridge);puts("Trophy file tables: 298 native IDs and inline filename/model strings passed");return 0;
    }
    if(argc==3&&(!strcmp(argv[1],"--ending-archive")||!strcmp(argv[1],"--staffroll-archive")||!strcmp(argv[1],"--approach-archive"))){
        int approach=!strcmp(argv[1],"--approach-archive");
        int staff=!strcmp(argv[1],"--staffroll-archive");
        FILE* f=fopen(argv[2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
        u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
        MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
        HSD_Archive* bridge=approach?melee_approach_decode(&a):staff?melee_staffroll_decode(&a):melee_ending_decode(&a);assert(bridge);
        memset(bytes,0xa5,size);free(bytes);
        const char* names[]={"cut1CanimScene","cut2CanimScene","cut3CanimScene","cut3BgScene"};
        unsigned models=0,cameras=0,animations=0;
        for(unsigned i=0;i<((staff||approach)?1:4);i++){
            SceneDesc* scene=HSD_ArchiveGetPublicAddress(bridge,approach?"ScNtcApproach_scene_data":staff?"ScGamRegStaffroll_scene_data":names[i]);assert(scene);
            for(unsigned j=0;scene->models&&scene->models[j];j++){
                HSD_JObj* joint=HSD_JObjLoadJoint(scene->models[j]->joint);assert(joint);
                DynamicModelDesc* d=scene->models[j];
                HSD_JObjAddAnimAll(joint,d->anims?d->anims[0]:NULL,d->matanims?d->matanims[0]:NULL,d->shapeanims?d->shapeanims[0]:NULL);
                for(unsigned frame=0;frame<=600;frame+=1){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);models++;
            }
            for(unsigned j=0;scene->cameras&&scene->cameras[j].desc;j++){
                HSD_CObj* camera=HSD_CObjLoadDesc(scene->cameras[j].desc);assert(camera);cameras++;
                for(unsigned k=0;scene->cameras[j].anims&&scene->cameras[j].anims[k];k++){
                    HSD_CObjAddAnim(camera,scene->cameras[j].anims[k]);animations++;
                    for(unsigned frame=0;frame<=600;frame+=10){HSD_CObjReqAnim(camera,frame);HSD_CObjAnim(camera);}
                }
                HSD_OBJECT_METHOD(camera)->release((HSD_Class*)camera);HSD_OBJECT_METHOD(camera)->destroy((HSD_Class*)camera);
            }
            for(unsigned j=0;scene->lights&&scene->lights[j];j++){
                HSD_LObj* light=HSD_LObjLoadDesc(scene->lights[j]->desc);assert(light);
                for(unsigned k=0;scene->lights[j]->anims&&scene->lights[j]->anims[k];k++){
                    HSD_LObjAddAnimAll(light,scene->lights[j]->anims[k]);
                    for(unsigned frame=0;frame<=600;frame+=10){HSD_LObjReqAnimAll(light,frame);HSD_LObjAnimAll(light);}
                }
                HSD_LObjRemoveAll(light);
            }
            if(scene->fogs){
                HSD_Fog* fog=HSD_FogLoadDesc(scene->fogs->desc);assert(fog);
                for(unsigned k=0;scene->fogs->anims&&scene->fogs->anims[k];k++){
                    HSD_Fog_8037DE7C(fog,scene->fogs->anims[k]->aobjdesc);
                    for(unsigned frame=0;frame<=600;frame+=10){HSD_FogReqAnim(fog,frame);HSD_FogInterpretAnim(fog);assert(isfinite(fog->start)&&isfinite(fog->end));}
                }
                hsdDelete(fog);
            }
        }
        if(staff){
            DynamicModelDesc** names=HSD_ArchiveGetPublicAddress(bridge,"ScGamRegStaffrollNames_scene_modelset");assert(names);
            for(unsigned i=0;i<10;i++){
                assert(names[i]);HSD_JObj* joint=HSD_JObjLoadJoint(names[i]->joint);assert(joint);
                HSD_JObjAddAnimAll(joint,names[i]->anims?names[i]->anims[0]:NULL,names[i]->matanims?names[i]->matanims[0]:NULL,names[i]->shapeanims?names[i]->shapeanims[0]:NULL);
                for(unsigned frame=0;frame<=600;frame+=10){HSD_JObjReqAnimAll(joint,frame);HSD_JObjAnimAll(joint);}
                HSD_JObjRemoveAll(joint);models++;
            }
            assert(!names[10]);
        }else if(!approach){
        size_t motion_size;const void* motion=melee_intro_motion_bytes(bridge,"ftDemoEndingMotionFileFox",&motion_size);assert(motion&&motion_size>32);
        }
        bridge->native_destroy(bridge);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        printf("%s archive: %u models, %u cameras, %u animations sampled after source disposal\n",approach?"Approach":staff?"Staff roll":"Ending",models,cameras,animations);return 0;
    }
    if(argc==6&&!strcmp(argv[1],"--gameover-archives")){
        for(unsigned i=0;i<4;i++){
            FILE* f=fopen(argv[i+2],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);
            u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
            MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
            const char* symbol=i==3?"standScene":"ScGamRegGover_scene_data";
            HSD_Archive* bridge=melee_single_scene_decode(&a,symbol);assert(bridge);
            memset(bytes,0xa5,size);free(bytes);
            SceneDesc* scene=HSD_ArchiveGetPublicAddress(bridge,symbol);assert(scene&&scene->models);
            unsigned models=0;
            for(;scene->models[models];models++){
                HSD_Joint* desc=scene->models[models]->joint;
                if(desc){HSD_JObj* joint=HSD_JObjLoadJoint(desc);assert(joint);HSD_JObjRemoveAll(joint);}
            }
            printf("Game-over archive %u: %u owned model entries after source disposal\n",i,models);
            bridge->native_destroy(bridge);
            assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        }
        return 0;
    }
    if(argc==3&&!strcmp(argv[1],"--scene-desc")){
        FILE* f=fopen(argv[2],"rb");assert(f);assert(!fseek(f,0,SEEK_END));
        long size=ftell(f);assert(size>32);rewind(f);uint8_t* bytes=malloc(size);assert(bytes);
        assert(fread(bytes,1,size,f)==(size_t)size);fclose(f);
        MeleeArchive archive;uint32_t root;
        assert(melee_archive_open(&archive,bytes,size));
        assert(melee_archive_find(&archive,"ScNtcCommon_scene_data",&root));
        MeleeSceneDesc* owner=melee_scene_desc_decode(&archive,root);assert(owner);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        uint32_t cameras,animations,animation;MeleeHostBool present;
        assert(melee_archive_pointer(&archive,root+4,&cameras,&present)&&present);
        assert(melee_archive_pointer(&archive,cameras+4,&animations,&present)&&present);
        assert(melee_archive_pointer(&archive,animations,&animation,&present)&&present);
        word(bytes+32+animation,1);
        for(unsigned attempt=0;attempt<3;attempt++)assert(!melee_scene_desc_decode(&archive,root));
        word(bytes+32+animation,0);

        memset(bytes,0xa5,size);free(bytes);
        SceneDesc* desc=melee_scene_desc_data(owner);
        assert(desc->models&&desc->models[0]&&!desc->models[1]);
        assert(desc->cameras&&desc->cameras[0].desc&&!desc->cameras[1].desc);
        HSD_JObj* model=HSD_JObjLoadJoint(desc->models[0]->joint);assert(model);
        HSD_JObjAddAnimAll(model,NULL,desc->models[0]->matanims[0],NULL);
        HSD_JObjReqAnimAll(model,0);HSD_JObjAnimAll(model);
        HSD_MObj* animated=model->child->u.dobj->next->mobj;assert(animated&&animated->aobj);
        GXColor first=animated->mat->diffuse;int changed=0;
        for(unsigned frame=1;frame<=20;frame++){
            HSD_JObjReqAnimAll(model,frame);HSD_JObjAnimAll(model);
            if(memcmp(&first,&animated->mat->diffuse,sizeof(first)))changed=1;
        }
        assert(changed);

        size_t drawables=0,polygons=0,materials=0;
        size_t joints=inspect(model,&drawables,&polygons,&materials);
        HSD_CObj* camera=HSD_CObjLoadDesc(desc->cameras[0].desc);assert(camera);
        HSD_CObjAddAnim(camera,desc->cameras[0].anims[0]);
        size_t lights=0;
        for(;desc->lights[lights];lights++){
            HSD_LObj* light=HSD_LObjLoadDesc(desc->lights[lights]->desc);assert(light);
            HSD_LObjAddAnimAll(light,desc->lights[lights]->anims[0]);
            HSD_LObjReqAnimAll(light,0);HSD_LObjAnimAll(light);HSD_LObjRemoveAll(light);
        }
        assert(lights==2&&joints>0&&drawables>0&&polygons>0);
        HSD_OBJECT_METHOD(camera)->release((HSD_Class*)camera);
        HSD_OBJECT_METHOD(camera)->destroy((HSD_Class*)camera);
        HSD_JObjRemoveAll(model);melee_scene_desc_free(owner);
        printf("Retail scene descriptors loaded and released: %zu joints, %zu drawables, %zu polygons, %zu lights\n",joints,drawables,polygons,lights);
        assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        return 0;
    }
    if(argc==2&&!strcmp(argv[1],"--training")){
        melee_training_probe();assert(!HSD_IDGetAllocData()->used&&!HSD_AObjGetAllocData()->used&&!HSD_MtxGetAllocData()->used);
        puts("Original training HUD digit selection preserves native pointer arrays");return 0;
    }
    if(argc==2&&!strcmp(argv[1],"--stock")){
        melee_stock_probe();assert(!HSD_IDGetAllocData()->used&&!HSD_MtxGetAllocData()->used&&!HSD_VecGetAllocData()->used);
        puts("Original stock-steal HUD uses native player records and full-width joint pointers");return 0;
    }
    if(argc>=3&&argc<=6){
        FILE* f=fopen(argv[1],"rb");assert(f);assert(!fseek(f,0,SEEK_END));
        long size=ftell(f);assert(size>32);rewind(f);uint8_t* bytes=malloc(size);assert(bytes);
        assert(fread(bytes,1,size,f)==(size_t)size);fclose(f);
        MeleeArchive archive;uint32_t root;
        assert(melee_archive_open(&archive,bytes,size)&&melee_archive_find(&archive,argv[2],&root));
        MeleeScene* scene=melee_scene_decode(&archive,root);assert(scene);
        if(fighterData){
            char materialName[256];size_t length=strlen(argv[2]);
            assert(length>6&&length+8<sizeof(materialName)&&!strcmp(argv[2]+length-6,"_joint"));
            memcpy(materialName,argv[2],length-6);strcpy(materialName+length-6,"_matanim_joint");
            uint32_t materialRoot;
            if(melee_archive_find(&archive,materialName,&materialRoot)){
                /* A material-tree cycle must fail before invoking HSD's
                 * recursive loader, including repeated rollback attempts. */
                uint32_t child;MeleeHostBool present;
                assert(melee_archive_pointer(&archive,materialRoot,&child,&present)&&present);
                word(bytes+32+materialRoot,materialRoot);
                MeleeScene* malformed=melee_scene_decode(&archive,root);assert(malformed);
                word(bytes+32+materialRoot,child);
                for(unsigned attempt=0;attempt<3;attempt++)assert(!melee_scene_bind_materials(malformed,materialRoot));
                melee_scene_free(malformed);
                assert(!melee_scene_bind_materials(scene,archive.data_size));
                assert(melee_scene_bind_materials(scene,materialRoot));
                assert(!melee_scene_bind_materials(scene,materialRoot));
                puts("Owned costume material animation attached through original HSD");
            }
        }
        free(bytes);
        if(fighterData)printf("Exercised every integer expression frame on %zu original HSD texture objects\n",check_expressions(melee_scene_root(scene)));
        size_t d=0,p=0,m=0,n=inspect(melee_scene_root(scene),&d,&p,&m);
        assert(n==melee_scene_joint_count(scene));
        printf("Original HSD asset scene: %zu joints, %zu drawables, %zu polygons, %zu compiled materials\n",n,d,p,m);
        if(argc>=4){
            FILE* motion=fopen(argv[3],"rb");assert(motion);uint8_t header[4];assert(fread(header,1,4,motion)==4);
            uint32_t length=(uint32_t)header[0]<<24|(uint32_t)header[1]<<16|(uint32_t)header[2]<<8|header[3];
            assert(length>=32&&length<64*1024*1024);uint8_t* data=malloc(length);assert(data);rewind(motion);
            assert(fread(data,1,length,motion)==length);fclose(motion);
            MeleeArchive anim;const char* name;uint32_t offset;
            assert(melee_archive_open(&anim,data,length)&&melee_archive_public(&anim,0,&name,&offset));
            assert(melee_scene_bind(scene,&anim,offset));
            FighterSelectionProbe* selection=fighterData?fighter_selection_probe_create(scene,fighterData,fighterSymbol,name):NULL;
            assert(!fighterData||selection);free(data);
            assert(melee_scene_request(scene,0));
            assert(melee_scene_step(scene));if(selection)assert(fighter_selection_probe_step(selection,0,0));Mtx* first=malloc(n*sizeof(Mtx));Mtx* last=malloc(n*sizeof(Mtx));assert(first&&last);
            size_t count=0;matrices(melee_scene_root(scene),first,&count);assert(count==n);
            for(unsigned frame=1;frame<60;frame++){
                assert(melee_scene_step(scene));if(selection)assert(fighter_selection_probe_step(selection,frame,1));
            }
            fighter_selection_probe_free(selection);
            count=0;matrices(melee_scene_root(scene),last,&count);assert(count==n&&memcmp(first,last,n*sizeof(Mtx)));
            free(first);free(last);
            puts("60 animation frames through original HSD_JObjAnimAll passed after source buffers were freed");
        }
        if(argc>=5){
            int render=argc==6;
            HSD_LObj* ambient=NULL;
            assert(!strcmp(argv[4],render?"--render":"--record"));
            if(render)assert(melee_gpu_begin("native/build/aurora-cache"));
            /* GXInit emits SDK state only. Never begin a backend frame or drain
             * this FIFO: this mode checks submission without a GPU/window. */
            GXInit(NULL,0);if(render)melee_hsd_video_gpu_init();HSD_StateInvalidate(-1);Mtx view;PSMTXIdentity(view);
            if(render){
                GXSetCopyClear((GXColor){13,18,26,255},GX_MAX_Z24);
                Vec eye={38,20,45},up={0,1,0},interest={0,10,0};C_MTXLookAt(view,&eye,&up,&interest);
                Mtx44 projection;MTXPerspective(projection,35,640.0f/480.0f,1,200);
                ambient=HSD_LObjAlloc();ambient->flags=LOBJ_AMBIENT|LOBJ_DIFFUSE|LOBJ_ALPHA;ambient->color=(GXColor){255,255,255,255};
                HSD_LObjSetCurrentAll(ambient);HSD_CObj camera={0};memcpy(camera.view_mtx,view,sizeof(Mtx));HSD_LObjSetupInit(&camera);
                GXSetProjection(projection,GX_PERSPECTIVE);GXSetViewport(0,0,640,480,0,1);GXSetScissor(0,0,640,480);
            }
            size_t before=render?0:aurora_fifo_copy(NULL,0);
            HSD_JObjDispAll(melee_scene_root(scene),view,HSD_TRSP_ALL,0);
            if(render && getenv("MELEE_GPU_REPEAT_FRAMES")){
                char* end;unsigned long frames=strtoul(getenv("MELEE_GPU_REPEAT_FRAMES"),&end,10);
                assert(*end=='\0'&&frames>=1&&frames<=120);
                /* Repeat this owned pose to exercise frame-slot reuse. This is
                 * not an additional animation or game simulation loop. */
                for(unsigned long frame=1;frame<frames;frame++){
                    GXFlush();assert(melee_gpu_next_frame());HSD_StateInvalidate(-1);
                    HSD_JObjDispAll(melee_scene_root(scene),view,HSD_TRSP_ALL,0);
                }
            }
            if(render && getenv("MELEE_GX_DIAGNOSTIC_TRIANGLE")){
                Mtx identity;PSMTXIdentity(identity);GXLoadPosMtxImm(identity,GX_PNMTX0);GXSetCurrentMtx(GX_PNMTX0);
                Mtx44 projection;MTXOrtho(projection,1,-1,-1,1,0,10);GXSetProjection(projection,GX_ORTHOGRAPHIC);
                GXSetZMode(GX_FALSE,GX_ALWAYS,GX_FALSE);GXSetCullMode(GX_CULL_NONE);
                GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);GXSetColorUpdate(GX_TRUE);GXSetAlphaUpdate(GX_TRUE);
                GXSetBlendMode(GX_BM_NONE,GX_BL_ONE,GX_BL_ZERO,GX_LO_COPY);
                GXSetNumChans(1);GXSetChanCtrl(GX_COLOR0A0,GX_FALSE,GX_SRC_REG,GX_SRC_VTX,GX_LIGHT_NULL,GX_DF_NONE,GX_AF_NONE);
                GXSetNumTexGens(0);GXSetNumTevStages(1);GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD_NULL,GX_TEXMAP_NULL,GX_COLOR0A0);GXSetTevOp(GX_TEVSTAGE0,GX_PASSCLR);
                GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_CLR0,GX_DIRECT);
                GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(GX_VTXFMT0,GX_VA_CLR0,GX_CLR_RGBA,GX_RGBA8,0);
                GXBegin(GX_TRIANGLES,GX_VTXFMT0,3);
                GXPosition3f32(-0.5f,-0.5f,-1);GXColor4u8(255,0,0,255);
                GXPosition3f32(0.5f,-0.5f,-1);GXColor4u8(255,0,0,255);
                GXPosition3f32(0,0.5f,-1);GXColor4u8(255,0,0,255);GXEnd();
            }
            GXFlush();
            if(render){assert(melee_gpu_finish(argv[5]));HSD_LObjSetCurrentAll(NULL);HSD_LObjRemoveAll(ambient);}
            else {size_t after=aurora_fifo_copy(NULL,0);assert(after>before+1000);
                printf("Original HSD draw submission recorded %zu GX command bytes (not executed)\n",after-before);}

        }
        melee_scene_free(scene);assert(!HSD_IDGetAllocData()->used);
        assert(!HSD_AObjGetAllocData()->used&&!HSD_FObjGetAllocData()->used);
        assert(!HSD_MtxGetAllocData()->used&&!HSD_VecGetAllocData()->used);
        return 0;
    }
    assert(argc==1);
    archive_checks();
    HSD_Joint child={.scale={1,1,1},.position={2,0,0}};
    uint8_t pixels[64]={0};
    HSD_ImageDesc image={.image_ptr=pixels,.width=4,.height=4,.format=GX_TF_RGBA8};
    HSD_TexLODDesc lod={.minFilt=GX_LINEAR,.LODBias=-0.5f,.max_anisotropy=GX_ANISO_2};
    HSD_TObjDesc texture={.id=GX_TEXMAP0,.src=GX_TG_TEX0,.scale={1,1,1},
        .wrap_s=GX_REPEAT,.wrap_t=GX_CLAMP,.repeat_s=1,.repeat_t=1,
        .blend_flags=TEX_COLORMAP_MODULATE|TEX_ALPHAMAP_MODULATE,
        .blending=1,.magFilt=GX_LINEAR,.imagedesc=&image,.lod=&lod};
    HSD_Material material={.ambient={128,128,128,255},.diffuse={255,64,32,255},.alpha=1};
    HSD_MObjDesc matdesc={.rendermode=RENDER_CONSTANT|RENDER_TEX0,.texdesc=&texture,.mat=&material};
    HSD_VtxDescList verts[]={{.attr=GX_VA_NULL}};
    uint8_t display[32]={0};
    HSD_PObjDesc polygon={.verts=verts,.flags=POBJ_SKIN,.n_display=1,.display=display,.u.joint=&child};
    HSD_DObjDesc drawable={.mobjdesc=&matdesc,.pobjdesc=&polygon};
    child.u.dobjdesc=&drawable;
    HSD_Joint root={.child=&child,.scale={1,1,1},.position={3,0,0}};
    for (int iteration=0;iteration<64;iteration++) {
    HSD_JObj* scene=HSD_JObjLoadJoint(&root);
    assert(scene && scene->child && scene->child->parent==scene);
    HSD_JObjSetupMatrix(scene->child);
    assert(scene->child->mtx[0][3]==5);
    HSD_DObj* d=scene->child->u.dobj;
    assert(d && d->mobj && d->pobj && d->mobj->tobj);
    assert(d->mobj->mat!=&material && d->mobj->mat->diffuse.g==64);
    assert(d->mobj->tobj->imagedesc==&image && d->mobj->tobj->lod==&lod);
    assert(d->pobj->u.jobj==scene->child && d->pobj->display==display);
    assert(d->mobj->tevdesc); /* Original TEV expression compiler ran. */
    HSD_ClassInfo* classes[]={HSD_CLASS_METHOD(scene),HSD_CLASS_METHOD(d),
        HSD_CLASS_METHOD(d->pobj),HSD_CLASS_METHOD(d->mobj),HSD_CLASS_METHOD(d->mobj->tobj)};
    assert(classes[0]->head.nb_exist==2);
    for (int i=1;i<5;i++) assert(classes[i]->head.nb_exist==1);
    HSD_JObjRemoveAll(scene);
    for (int i=0;i<5;i++) assert(classes[i]->head.nb_exist==0);
    assert(!HSD_IDGetAllocData()->used);
    assert(!HSD_MtxGetAllocData()->used && !HSD_VecGetAllocData()->used);
    }
    puts("Original HSD joint/drawable/polygon/material/texture loading, TEV compilation, reference resolution and destruction passed for 64 scene lifecycles");
}
