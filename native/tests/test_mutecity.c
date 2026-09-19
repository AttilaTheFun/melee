/* Allocation and scale dependencies are controlled test doubles. This tests
 * the original setup routine, not particle spawning or a playable stage. */
#include <melee/gr/grmutecity.h>
#include <melee/gr/grlib.h>
#include <melee/gr/ground.h>
#include <melee/gr/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/psappsrt.h>
#include <sysdolphin/baselib/psstructs.h>
#include <melee/gr/grmutecity.static.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
StageInfo stage_info;
static HSD_Generator generator;
static HSD_psAppSRT transform;
static HSD_Generator* generated;
static HSD_psAppSRT* allocated;
static int allocation_calls,scale_calls;
HSD_Generator* grLib_801C9808(s32 id,s32 group,HSD_JObj* joint)
{ assert(id==0x116&&group==0&&joint==NULL);return generated; }
HSD_psAppSRT* psAddGeneratorAppSRT_begin(HSD_Generator* gen,s32 mode)
{ assert(gen==&generator&&mode==0);allocation_calls++;return allocated; }
float Ground_801C0498(void){scale_calls++;return stage_info.param?stage_info.param->y:1;}
int main(void)
{
    GroundParam param={.y=2.5f};stage_info.param=&param;
    assert(grMuteCity_801F2AB0(0x116,NULL)==0 && !allocation_calls && !scale_calls);
    generated=&generator;
    assert(grMuteCity_801F2AB0(0x116,NULL)==0 && allocation_calls==1 && !scale_calls);
    allocated=&transform;transform.xA2=1;generator.type=0xffff;
    assert(grMuteCity_801F2AB0(0x116,NULL)==1 && allocation_calls==2 && scale_calls==1);
    assert(transform.gp==&generator && transform.xA2==0);
    assert(transform.scale.x==2.5f&&transform.scale.y==2.5f&&transform.scale.z==2.5f);
    assert(!(generator.type&(PSAPPSRT_UNK_B09|PSAPPSRT_UNK_B10)) && (generator.type&PSAPPSRT_UNK_B11));
    generator.appsrt=&transform;allocated=NULL;stage_info.param=NULL;
    assert(grMuteCity_801F2AB0(0x116,NULL)==0 && allocation_calls==2 && scale_calls==2);
    assert(transform.scale.x==1); /* Console marker remains zero with no stage parameters. */
    HSD_GObj object={0};assert((uintptr_t)&object>UINT32_MAX);
    grMc_8049F4B8[29].x24=&object;grMc_8049F4B8[29].x28=1;
    assert(grMc_8049F4B8[29].x24==&object && grMc_8049F4B8[29].x28==1);
    puts("Mute City effect setup preserves the observed result marker and native object pointers.");
}
