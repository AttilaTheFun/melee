#include "melee_item_attributes.h"
#include <math.h>
#include <string.h>
_Static_assert(sizeof(ItemAttr)==0x84,"Item attribute scalar layout");
_Static_assert(offsetof(ItemAttr,x3)==2,"Item trailing header byte");
_Static_assert(offsetof(ItemAttr,x4_throw_speed_mul)==4,"Item attribute scalar offset");
_Static_assert(offsetof(ItemAttr,x80)==0x80,"Item final attribute offset");
MeleeHostBool melee_item_attributes_decode(const MeleeArchive* a,uint32_t at,ItemAttr* out){
    if(!a||!out||at>a->data_size||a->data_size-at<sizeof(*out))return false;
    ItemAttr value;memcpy(&value,a->bytes+32+at,sizeof(value));
    const u8* b=a->bytes+32+at;
    value.x0_is_heavy=b[0]>>7;value.x0_78=(b[0]>>3)&15;value.x0_hold_kind=b[0]&7;
    value.x1_1=b[1]>>6;value.x1_3=(b[1]>>5)&1;value.x1_4=(b[1]>>4)&1;
    value.x1_5=(b[1]>>3)&1;value.x1_67_cam_kind=(b[1]>>1)&3;value.x1_8=b[1]&1;
    for(unsigned off=4;off<sizeof(value);off+=4){
        u32 bits;if(!melee_archive_u32(a,at+off,&bits))return false;
        if(off<=0x60&&off!=8){float f;memcpy(&f,&bits,4);if(!isfinite(f))return false;}
        memcpy((u8*)&value+off,&bits,4);
    }
    *out=value;return true;
}
