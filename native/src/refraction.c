#include "melee_refraction.h"
#include <math.h>
MeleeHostBool melee_refraction_decode(const MeleeArchive* a,float* pairs,size_t capacity,unsigned* count){
    uint32_t root,values;MeleeHostBool present;
    if(!a||!pairs||!count||a->extern_count||!melee_archive_find(a,"lbRefData",&root)||
       root>a->data_size||a->data_size-root<8||!melee_archive_pointer(a,root+4,&values,&present)||!present)return false;
    unsigned n=a->bytes[32+root];
    if(!n||2u*n>capacity||values>a->data_size||a->data_size-values<8u*n)return false;
    for(unsigned i=0;i<2*n;i++){
        float v;if(!melee_archive_f32(a,values+4*i,&v)||!isfinite(v)||v<0)return false;
    }
    for(unsigned i=0;i<2*n;i++)melee_archive_f32(a,values+4*i,&pairs[i]);
    *count=n;return true;
}
