#include "melee_camera.h"
#include "melee_animation.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/wobj.h>
#include <math.h>
#include <stdlib.h>

struct MeleeCamera {
    HSD_CObjDesc desc;
    HSD_WObjDesc eye, interest;
    Vec3 up;
};
static int reference(const MeleeArchive* a, uint32_t at, uint32_t* target,
                     MeleeHostBool* present)
{ return melee_archive_pointer(a, at, target, present); }
static int absent(const MeleeArchive* a, uint32_t at)
{
    uint32_t target; MeleeHostBool present;
    return reference(a, at, &target, &present) && !present;
}
static int scalar(const MeleeArchive* a, uint32_t at, float* out)
{ return melee_archive_f32(a, at, out) && isfinite(*out); }
static int vector(const MeleeArchive* a, uint32_t at, Vec3* out)
{
    return at <= a->data_size && a->data_size-at >= 12 &&
        scalar(a, at, &out->x) && scalar(a, at+4, &out->y) &&
        scalar(a, at+8, &out->z);
}
static int world(const MeleeArchive* a, uint32_t slot, HSD_WObjDesc* out)
{
    uint32_t at; MeleeHostBool present;
    return reference(a, slot, &at, &present) && present &&
        at <= a->data_size && a->data_size-at >= 20 &&
        absent(a, at) && vector(a, at+4, &out->pos) && absent(a, at+16);
}
HSD_CObjDesc* melee_camera_descriptor(MeleeCamera* c)
{ return c ? &c->desc : NULL; }
void melee_camera_free(MeleeCamera* c) { free(c); }
MeleeCamera* melee_camera_decode(const MeleeArchive* a, uint32_t at)
{
    if (!a || at > a->data_size || a->data_size-at < 56 || !absent(a, at))
        return NULL;
    uint32_t flags, viewport[2], scissor[2];
    if (!melee_archive_u32(a, at+4, &flags) ||
        !melee_archive_u32(a, at+8, &viewport[0]) ||
        !melee_archive_u32(a, at+12, &viewport[1]) ||
        !melee_archive_u32(a, at+16, &scissor[0]) ||
        !melee_archive_u32(a, at+20, &scissor[1])) return NULL;
    unsigned projection = flags & 65535;
    if (projection < PROJ_PERSPECTIVE || projection > PROJ_ORTHO ||
        (projection != PROJ_PERSPECTIVE && a->data_size-at < 64)) return NULL;
    MeleeCamera* c = calloc(1, sizeof(*c));
    if (!c) return NULL;
    HSD_CameraDescCommon* d = &c->desc.common;
    d->flags = flags >> 16; d->projection_type = projection;
    d->viewport = (HSD_RectS16){(s16)(viewport[0]>>16), (s16)viewport[0],
                               (s16)(viewport[1]>>16), (s16)viewport[1]};
    d->scissor = (Scissor){scissor[0]>>16, scissor[0], scissor[1]>>16, scissor[1]};
    if (!world(a, at+24, &c->eye) || !world(a, at+28, &c->interest) ||
        !scalar(a, at+32, &d->roll) || !scalar(a, at+40, &d->nnear) ||
        !scalar(a, at+44, &d->ffar)) goto fail;
    d->eyepos = &c->eye; d->interest = &c->interest;
    uint32_t up; MeleeHostBool present;
    if (!reference(a, at+36, &up, &present)) goto fail;
    if (present) {
        if (!vector(a, up, &c->up)) goto fail;
        d->up_vector = &c->up;
    }
    if (projection == PROJ_PERSPECTIVE) {
        if (!scalar(a, at+48, &c->desc.perspective.fov) ||
            !scalar(a, at+52, &c->desc.perspective.aspect)) goto fail;
    } else {
        if (!scalar(a, at+48, &c->desc.frustum.top) ||
            !scalar(a, at+52, &c->desc.frustum.bottom) ||
            !scalar(a, at+56, &c->desc.frustum.left) ||
            !scalar(a, at+60, &c->desc.frustum.right)) goto fail;
    }
    return c;
fail:
    free(c); return NULL;
}

MeleeHostBool melee_camera_empty_animation_decode(const MeleeArchive* a,uint32_t at,HSD_CameraAnim* out)
{
    if(!a||!out||at>a->data_size||a->data_size-at<12)return false;
    for(unsigned i=0;i<3;i++)if(!absent(a,at+4*i))return false;
    *out=(HSD_CameraAnim){0};return true;
}

struct MeleeCameraAnimation {
    HSD_CameraAnim desc;
    HSD_WObjAnim world[2];
    HSD_AObjDesc aobj[3];
    MeleeAnimationTracks* tracks[3];
};
void melee_camera_animation_free(MeleeCameraAnimation* c)
{
    if (!c) return;
    for (unsigned i=0;i<3;i++) melee_animation_tracks_free(c->tracks[i]);
    free(c);
}
HSD_CameraAnim* melee_camera_animation_descriptor(MeleeCameraAnimation* c)
{ return c ? &c->desc : NULL; }
static int camera_aobj(MeleeCameraAnimation* c,const MeleeArchive* a,
                       uint32_t slot,unsigned index,HSD_AObjDesc** out)
{
    uint32_t at,tracks; MeleeHostBool present;
    if (!reference(a,slot,&at,&present)) return 0;
    if (!present) return 1;
    HSD_AObjDesc* d=&c->aobj[index];
    if (at>a->data_size || a->data_size-at<16 ||
        !melee_archive_u32(a,at,&d->flags) ||
        !scalar(a,at+4,&d->end_frame) || d->end_frame<0 ||
        !absent(a,at+12) || !reference(a,at+8,&tracks,&present)) return 0;
    if (present) {
        c->tracks[index]=melee_animation_tracks_decode(a,tracks);
        if (!c->tracks[index]) return 0;
        d->fobjdesc=melee_animation_tracks_descriptors(c->tracks[index]);
        for (HSD_FObjDesc* f=d->fobjdesc;f;f=f->next) {
            unsigned t=f->type;
            if (index ? (t<5 || t>7) :
                !(t==1 || t==2 || t==3 || t==5 || t==6 || t==7 ||
                  t==9 || t==10 || t==11 || t==12)) return 0;
        }
    }
    *out=d;return 1;
}
MeleeCameraAnimation* melee_camera_animation_decode(const MeleeArchive* a,uint32_t at)
{
    if (!a || at>a->data_size || a->data_size-at<12) return NULL;
    MeleeCameraAnimation* c=calloc(1,sizeof(*c));
    if (!c) return NULL;
    if (!camera_aobj(c,a,at,0,&c->desc.aobjdesc)) goto fail;
    for (unsigned i=0;i<2;i++) {
        uint32_t w; MeleeHostBool present;
        if (!reference(a,at+4+4*i,&w,&present)) goto fail;
        if (!present) continue;
        if (w>a->data_size || a->data_size-w<8 || !absent(a,w+4) ||
            !camera_aobj(c,a,w,i+1,&c->world[i].aobjdesc)) goto fail;
        if (i) c->desc.interest_anim=&c->world[i];
        else c->desc.eye_anim=&c->world[i];
    }
    return c;
fail:
    melee_camera_animation_free(c);return NULL;
}
