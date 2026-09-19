#include "melee_gx_pixel.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <dolphin/gx.h>
#include <dolphin/gx/GXPixel.h>
int main(void)
{
    MeleeGXPixelState s,t;
    for(unsigned mask=0;mask<8;mask++){
        uint32_t mode=((mask&1)?0x40000000:0)|((mask&2)?0x20000000:0)|((mask&4)?0x08000000:0);
        assert(melee_gx_pixel_material(mode,NULL,&s));
        assert(s.color_update && !s.alpha_update && !s.dst_alpha_enable && !s.dither);
        assert(s.blend_type==(mask&1) && s.src_factor==4 && s.dst_factor==5 && s.logic_op==15);
        assert(s.z_enable && s.z_update==!(mask&2) && s.z_compare==((mask&4)?7:3));
        unsigned cutout=(mask&1) && !(mask&2);
        assert(s.before_texture==!cutout && s.alpha_compare0==(cutout?4:7) && s.alpha_compare1==s.alpha_compare0);
        assert(!s.alpha_ref0 && !s.alpha_ref1 && !s.alpha_operation);
        melee_gx_pixel_snapshot(&t);assert(!memcmp(&s,&t,sizeof(s)));
    }
    const uint8_t custom[]={0x7f,23,197,129,3,2,6,8,5,2,3,6};
    assert(melee_gx_pixel_material(0,custom,&s));
    assert(s.color_update && s.alpha_update && s.dst_alpha_enable && s.dst_alpha==129 && s.dither);
    assert(s.z_enable && s.z_update && s.before_texture && s.z_compare==5);
    assert(s.blend_type==3 && s.src_factor==2 && s.dst_factor==6 && s.logic_op==8);
    assert(s.alpha_compare0==2 && s.alpha_ref0==23 && s.alpha_compare1==6 && s.alpha_ref1==197 && s.alpha_operation==3);
    uint8_t zero[12]={0};assert(melee_gx_pixel_material(0,zero,&s));
    memset(&t,0,sizeof(t));assert(!memcmp(&s,&t,sizeof(s)));
    GXSetColorUpdate(1);melee_gx_pixel_snapshot(&s);assert(s.color_update);
    assert(melee_gx_pixel_material(0,zero,&s) && !s.color_update); /* invalidation defeats stale HSD cache */
    for(unsigned i=4;i<12;i++){
        memcpy(zero,custom,12);zero[i]=255;assert(!melee_gx_pixel_material(0,zero,&s));
    }
    assert(!melee_gx_pixel_material(0,NULL,NULL));
    puts("Original HSD pixel setup: default render flags, custom PE, GX registers and cache invalidation passed");
}
