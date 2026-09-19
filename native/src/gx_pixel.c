#include "melee_gx_pixel.h"
#include <dolphin/gx.h>
#include <dolphin/gx/GXPixel.h>
#include <dolphin/gx/GXTev.h>
#include <dolphin/os.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/state.h>
#include <string.h>

static MeleeGXPixelState registers;
void GXSetColorUpdate(GXBool v){registers.color_update=!!v;}
void GXSetAlphaUpdate(GXBool v){registers.alpha_update=!!v;}
void GXSetDstAlpha(GXBool enabled,u8 alpha){registers.dst_alpha_enable=!!enabled;registers.dst_alpha=alpha;}
void GXSetBlendMode(GXBlendMode type,GXBlendFactor src,GXBlendFactor dst,GXLogicOp op)
{registers.blend_type=type;registers.src_factor=src;registers.dst_factor=dst;registers.logic_op=op;}
void GXSetZMode(GXBool enabled,GXCompare compare,GXBool update)
{registers.z_enable=!!enabled;registers.z_compare=compare;registers.z_update=!!update;}
void GXSetZCompLoc(GXBool before){registers.before_texture=!!before;}
void GXSetAlphaCompare(GXCompare c0,u8 r0,GXAlphaOp op,GXCompare c1,u8 r1)
{registers.alpha_compare0=c0;registers.alpha_ref0=r0;registers.alpha_operation=op;registers.alpha_compare1=c1;registers.alpha_ref1=r1;}
void GXSetDither(GXBool v){registers.dither=!!v;}
void melee_gx_pixel_snapshot(MeleeGXPixelState* output)
{if(output){BOOL level=OSDisableInterrupts();*output=registers;OSRestoreInterrupts(level);}}
MeleeHostBool melee_gx_pixel_material(uint32_t mode,const uint8_t* descriptor,MeleeGXPixelState* output)
{
    if(!output)return false;
    HSD_PEDesc pe;
    if(descriptor){
        _Static_assert(sizeof(pe)==12,"on-disk pixel descriptor layout");
        memcpy(&pe,descriptor,12);
        if(pe.type>3 || pe.src_factor>7 || pe.dst_factor>7 || pe.logic_op>15 ||
           pe.z_comp>7 || pe.alpha_comp0>7 || pe.alpha_comp1>7 || pe.alpha_op>3)return false;
    }
    BOOL level=OSDisableInterrupts();
    _HSD_StateInvalidateRenderMode();
    HSD_SetupPEMode(mode,descriptor?&pe:NULL);
    *output=registers;
    OSRestoreInterrupts(level);
    return true;
}
