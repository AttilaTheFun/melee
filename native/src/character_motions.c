#include "melee_character_motions.h"
#include "melee_action.h"
#include <melee/ft/types.h>
#include <stdlib.h>
#include <string.h>
struct MeleeCharacterMotions {
    unsigned count;
    MeleeAction* scripts;
    struct Fighter_WaitAnimData* records;
    MeleeCharacterMotion* entries;
    MeleeFighterAnimation** animations;
    u8* aj;
    size_t aj_size;
};
void melee_character_motions_free(MeleeCharacterMotions* o)
{
    if(o){for(unsigned i=0;i<o->count;i++){if(o->entries)free((void*)o->entries[i].name);if(o->animations)melee_fighter_animation_free(o->animations[i]);}
        melee_action_free(o->scripts);
        free(o->records);
        free(o->entries);free(o->animations);free(o->aj);free(o);}
}
const MeleeCharacterMotion* melee_character_motions_entry(MeleeCharacterMotions* o,unsigned i)
{return o&&i<o->count?o->entries+i:NULL;}
_Static_assert(sizeof(struct Fighter_WaitAnimData)==sizeof(struct ftData_80085FD4_ret),"native motion aliases");
_Static_assert(offsetof(struct Fighter_WaitAnimData,x14)==offsetof(struct ftData_80085FD4_ret,x14),"native motion identity offset");
struct Fighter_WaitAnimData* melee_character_motions_records(MeleeCharacterMotions* o)
{return o?o->records:NULL;}
union CmdUnion* melee_character_motions_script(MeleeCharacterMotions* o,unsigned i)
{
    if(!o||i>=o->count||o->entries[i].script_offset==UINT32_MAX)return NULL;
    return melee_fighter_actions_script(o->scripts,o->entries[i].script_offset);
}
FigaTree* melee_character_motions_tree(MeleeCharacterMotions* o,unsigned i)
{
    if(!o||i>=o->count||!o->entries[i].size)return NULL;
    if(!o->animations[i]){
        MeleeCharacterMotion* e=o->entries+i;MeleeArchive a;u32 root;
        if(!melee_archive_open(&a,o->aj+e->offset,e->size)||!melee_archive_find(&a,e->name,&root))return NULL;
        o->animations[i]=melee_fighter_animation_decode(&a,root);
    }
    return melee_fighter_animation_tree(o->animations[i]);
}
MeleeCharacterMotions* melee_character_motions_decode_range(const MeleeArchive* a,u32 slot,
    unsigned count,unsigned first,unsigned loaded,const void* aj,size_t size)
{
    u32 table;MeleeHostBool present;
    if(!a||!aj||!size||!count||count>1024||!loaded||first>=count||loaded>count-first||
       !melee_archive_pointer(a,slot,&table,&present)||!present||
       table>a->data_size||(uint64_t)count*24>a->data_size-table)return NULL;
    MeleeCharacterMotions* o=calloc(1,sizeof(*o));if(!o)return NULL;
    o->count=count;o->aj_size=size;o->aj=malloc(size);
    o->entries=calloc(count,sizeof(*o->entries));o->animations=calloc(count,sizeof(*o->animations));
    if(!o->aj||!o->entries||!o->animations)goto fail;memcpy(o->aj,aj,size);
    for(unsigned i=0;i<count;i++)o->entries[i].script_offset=UINT32_MAX;
    for(unsigned i=first;i<first+loaded;i++){
        MeleeCharacterMotion* e=o->entries+i;u32 at=table+24*i,name,reserved;
        if(!melee_archive_pointer(a,at,&name,&present))goto fail;
        if(present){
            if(name>=a->data_size)goto fail;
            const char* s=(const char*)a->bytes+32+name;const char* end=memchr(s,0,a->data_size-name);
            if(!end)goto fail;size_t n=end-s+1;char* copy=malloc(n);if(!copy)goto fail;memcpy(copy,s,n);e->name=copy;
        }
        if(!melee_archive_u32(a,at+4,&e->offset)||!melee_archive_u32(a,at+8,&e->size)||
           !melee_archive_u32(a,at+16,&e->flags)||!melee_archive_u32(a,at+20,&reserved)||reserved)goto fail;
        if(!melee_archive_pointer(a,at+12,&e->script_offset,&present))goto fail;
        if(!present)e->script_offset=UINT32_MAX;
        if(e->size){
            if(!e->name||e->offset>size||e->size>size-e->offset)goto fail;
            MeleeArchive motion;u32 symbol;
            if(!melee_archive_open(&motion,o->aj+e->offset,e->size)||!melee_archive_find(&motion,e->name,&symbol))goto fail;
        }
    }
    u32 roots[1024];size_t root_count=0;
    for(unsigned i=0;i<count;i++)if(o->entries[i].script_offset!=UINT32_MAX)roots[root_count++]=o->entries[i].script_offset;
    if(root_count){o->scripts=melee_fighter_actions_decode(a,roots,root_count);if(!o->scripts)goto fail;}
    o->records=calloc(count,sizeof(*o->records));if(!o->records)goto fail;
    for(unsigned i=0;i<count;i++){
        MeleeCharacterMotion* e=o->entries+i;struct Fighter_WaitAnimData* r=o->records+i;
        r->x0=(char*)e->name;r->x4=e->offset;r->x8=e->size;r->x10_animCurrFlags=e->flags;
        r->xC=melee_character_motions_script(o,i);r->x14=e->size?(uintptr_t)e:0;
        r->native_owner=(i>=first&&i<first+loaded)?o:NULL;r->native_index=i;
    }
    return o;
fail:melee_character_motions_free(o);return NULL;
}

MeleeCharacterMotions* melee_character_motions_decode(const MeleeArchive* a,u32 root,
    unsigned count,const void* aj,size_t size)
{
    if(root>UINT32_MAX-12)return NULL;
    return melee_character_motions_decode_range(a,root+12,count,0,count,aj,size);
}
