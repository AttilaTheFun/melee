#include "melee_fighter_cpu.h"
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftcmdscript.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { CPU_KINDS=32, CPU_SCRIPTS=62, MAX_ATTACKS=32 };
struct MeleeFighterCpu {
    struct Fighter_804D64FC_t header;
    u8* scripts[CPU_SCRIPTS];
    u8 script_bytes[CPU_SCRIPTS][256];
    void* tables[7][CPU_KINDS];
    ftCo_AttackEntry attacks[7][CPU_KINDS][MAX_ATTACKS+1];
    float distances[CPU_KINDS], reach[6];
};
_Static_assert(sizeof(ftCo_AttackEntry)==36,"CPU attack record schema");
void melee_fighter_cpu_free(MeleeFighterCpu* owner){free(owner);}
struct Fighter_804D64FC_t* melee_fighter_cpu_header(MeleeFighterCpu* owner)
{return owner?&owner->header:NULL;}
static int required(const MeleeArchive* a,u32 at,u32* target)
{MeleeHostBool p;return melee_archive_pointer(a,at,target,&p)&&p;}
static int floats(const MeleeArchive* a,u32 at,float* out,unsigned count)
{
    for(unsigned i=0;i<count;i++)
        if(!melee_archive_f32(a,at+4*i,out+i)||!isfinite(out[i]))return 0;
    return 1;
}
MeleeFighterCpu* melee_fighter_cpu_decode(const MeleeArchive* a)
{
    u32 root,header,refs[10];
    if(!a||!melee_archive_find(a,"ftLoadCommonData",&root)||
       !required(a,root+88,&header))return NULL;
    for(unsigned i=0;i<10;i++)if(!required(a,header+4*i,refs+i))return NULL;
    MeleeFighterCpu* owner=calloc(1,sizeof(*owner));if(!owner)return NULL;
    if(!floats(a,refs[8],owner->distances,CPU_KINDS)||
       !floats(a,refs[9],owner->reach,6))goto fail;
    for(unsigned i=0;i<CPU_SCRIPTS;i++){
        u32 at;MeleeHostBool present;
        if(!melee_archive_pointer(a,refs[0]+4*i,&at,&present))goto fail;
        if(!present){if(i)goto fail;continue;}
        unsigned used=0;
        for(;;){
            if(at>=a->data_size||used>=256)goto fail;
            u8 op=a->bytes[32+at];
            if(op==CpuCmd_Done){owner->script_bytes[i][used]=op;break;}
            if(!((op>=1&&op<=CpuCmd_ReleaseAll)||
                 (op>=CpuCmd_SetLstickX&&op<=CpuCmd_LstickXTowardFighter)||
                 (op>=CpuCmd_LstickTowardDestinationClamped&&op<CpuCmd_Count)))goto fail;
            unsigned n=op>CpuCmd_OneArgEnd?3:op>CpuCmd_ZeroArgEnd?2:1;
            if(n>a->data_size-at||n>255-used)goto fail;
            memcpy(owner->script_bytes[i]+used,a->bytes+32+at,n);at+=n;used+=n;
        }
        owner->scripts[i]=owner->script_bytes[i];
    }
    for(unsigned table=0;table<7;table++)for(unsigned kind=0;kind<CPU_KINDS;kind++){
        u32 at;if(!required(a,refs[table+1]+4*kind,&at))goto fail;
        owner->tables[table][kind]=owner->attacks[table][kind];
        for(unsigned n=0;;n++){
            u32 cmd;if(!melee_archive_u32(a,at,&cmd))goto fail;
            if(!cmd)break;
            if(n>=MAX_ATTACKS)goto fail;
            ftCo_AttackEntry* entry=&owner->attacks[table][kind][n];
            for(unsigned j=0;j<9;j++){
                u32 value;if(!melee_archive_u32(a,at+4*j,&value))goto fail;
                if(j>=2&&j<=6){float f;memcpy(&f,&value,4);if(!isfinite(f))goto fail;}
                memcpy((u8*)entry+4*j,&value,4);
            }
            at+=36;
        }
    }
    owner->header=(struct Fighter_804D64FC_t){owner->scripts,
        owner->tables[0],owner->tables[1],owner->tables[2],owner->tables[3],
        owner->tables[4],owner->tables[5],owner->tables[6],owner->distances,owner->reach};
    return owner;
fail:melee_fighter_cpu_free(owner);return NULL;
}
