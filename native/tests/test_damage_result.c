#include "melee_damage_result.h"
#include <sysdolphin/baselib/gobj.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
int main(void){
    Fighter* fp=calloc(1,sizeof(*fp)); HSD_GObj source={0};
    assert(fp && (uintptr_t)&source>UINT32_MAX);
    MeleeDamageResult r={-1,45,2,13.5f,{1,2,3},7,4,&source,9.25f};
    fp->dmg.x183C_applied=123; fp->dmg.x189C_unk_num_frames=456;
    melee_damage_result_store(fp,&r,1);
    assert(fp->dmg.facing_dir_1==-1&&fp->dmg.x1848_kb_angle==45);
    assert(fp->dmg.x184c_damaged_hurtbox==2&&fp->dmg.kb_applied==13.5f);
    assert(fp->dmg.x1854_collpos.z==3&&fp->dmg.x1860_element==7&&fp->dmg.x1864==4);
    assert(fp->dmg.x1868_source==&source&&fp->dmg.x186c==9.25f);
    assert(!fp->dmg.x1870&&!fp->dmg.x1898);
    melee_damage_result_store(fp,&r,0);
    assert(fp->dmg.x1870==-1&&fp->dmg.x1874==45&&fp->dmg.x1878==2);
    assert(fp->dmg.x187c==13.5f&&fp->dmg.x1880.x==1&&fp->dmg.x1880.y==2&&fp->dmg.x1880.z==3);
    assert(fp->dmg.x188c==7&&fp->dmg.x1890==4&&fp->dmg.x1894==&source&&fp->dmg.x1898==9.25f);
    assert(fp->dmg.x1868_source==&source&&fp->dmg.x186c==9.25f);
    assert(fp->dmg.x183C_applied==123&&fp->dmg.x189C_unk_num_frames==456);
    free(fp);puts("Native damage results: both destinations, full-width source, scalar fields and neighboring records passed");
}
