#include "melee_onett_params.h"
#include <math.h>
#include <string.h>
_Static_assert(sizeof(struct grOnett_StageParam)==26*sizeof(float),"Onett float-only layout");
_Static_assert(offsetof(struct grOnett_StageParam,x64)==100,"Onett last field offset");
MeleeHostBool melee_onett_params_decode(const MeleeArchive* a,struct grOnett_StageParam* out){
    u32 root;float values[26];
    if(!a||!out||!melee_archive_find(a,"yakumono_param",&root)||root>a->data_size||a->data_size-root<sizeof(values))return false;
    for(unsigned i=0;i<26;i++)if(!melee_archive_f32(a,root+4*i,values+i)||!isfinite(values[i]))return false;
    memcpy(out,values,sizeof(values));return true;
}
