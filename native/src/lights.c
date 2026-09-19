#include "melee_lights.h"
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/wobj.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct NativeLight {
    HSD_LightDesc desc;
    HSD_WObjDesc position, interest;
    union {
        float shininess;
        HSD_LightPointDesc point;
        HSD_LightSpotDesc spot;
        HSD_LightAttn attn;
    } params;
    uint32_t offset;
    struct NativeLight* next;
} NativeLight;
struct MeleeLights { NativeLight* first; size_t count; };
static int range(const MeleeArchive* a, uint32_t at, size_t n)
{ return at<=a->data_size && n<=a->data_size-at; }
static int absent(const MeleeArchive* a, uint32_t slot)
{
    uint32_t at; MeleeHostBool present;
    return melee_archive_pointer(a,slot,&at,&present) && !present;
}
static int scalar(const MeleeArchive* a, uint32_t at, float* v)
{ return melee_archive_f32(a,at,v) && isfinite(*v); }
static int world(const MeleeArchive* a, uint32_t slot, HSD_WObjDesc* storage,
                 HSD_WObjDesc** out)
{
    uint32_t at; MeleeHostBool present;
    if (!melee_archive_pointer(a,slot,&at,&present)) return 0;
    if (!present) { *out=NULL; return 1; }
    if (!range(a,at,20) || !absent(a,at) || !absent(a,at+16) ||
        !scalar(a,at+4,&storage->pos.x) || !scalar(a,at+8,&storage->pos.y) ||
        !scalar(a,at+12,&storage->pos.z)) return 0;
    *out=storage; return 1;
}
static int parameters(const MeleeArchive* a, NativeLight* n)
{
    HSD_LightDesc* d=&n->desc; uint32_t at; MeleeHostBool present;
    unsigned type=d->flags & LOBJ_TYPE_MASK;
    if (!melee_archive_pointer(a,n->offset+24,&at,&present)) return 0;
    if (!present) return type==LOBJ_AMBIENT || type==LOBJ_INFINITE;
    if (type==LOBJ_AMBIENT || type==LOBJ_INFINITE) {
        d->u.shininess=&n->params.shininess;
        return scalar(a,at,d->u.shininess);
    }
    if ((type==LOBJ_POINT && (d->attnflags & LOBJ_LIGHT_ATTN)) ||
        (type==LOBJ_SPOT && d->attnflags)) {
        HSD_LightAttn* p=&n->params.attn; d->u.attn=p;
        return range(a,at,24) && scalar(a,at,&p->a0) && scalar(a,at+4,&p->a1) &&
            scalar(a,at+8,&p->a2) && scalar(a,at+12,&p->k0) &&
            scalar(a,at+16,&p->k1) && scalar(a,at+20,&p->k2);
    }
    if (type==LOBJ_POINT) {
        HSD_LightPointDesc* p=&n->params.point; d->u.point=p;
        return range(a,at,12) && scalar(a,at,&p->ref_br) &&
            scalar(a,at+4,&p->ref_dist) && melee_archive_u32(a,at+8,&p->dist_func) &&
            p->dist_func<=GX_DA_STEEP;
    }
    HSD_LightSpotDesc* p=&n->params.spot; d->u.spot=p;
    return range(a,at,20) && scalar(a,at,&p->cutoff) &&
        melee_archive_u32(a,at+4,&p->spot_func) && p->spot_func<=GX_SP_RING2 &&
        scalar(a,at+8,&p->ref_br) && scalar(a,at+12,&p->ref_dist) &&
        melee_archive_u32(a,at+16,&p->dist_func) && p->dist_func<=GX_DA_STEEP;
}
void melee_lights_free(MeleeLights* lights)
{
    if (!lights) return;
    NativeLight* n=lights->first;
    while (n) { NativeLight* next=n->next; free(n); n=next; }
    free(lights);
}
size_t melee_lights_count(const MeleeLights* lights) { return lights?lights->count:0; }
HSD_LightDesc* melee_lights_descriptor(MeleeLights* lights)
{ return lights && lights->first ? &lights->first->desc : NULL; }
MeleeLights* melee_lights_decode(const MeleeArchive* a, uint32_t root)
{
    if (!a) return NULL;
    MeleeLights* lights=calloc(1,sizeof(*lights)); if (!lights) return NULL;
    NativeLight* previous=NULL;
    for (;;) {
        if (lights->count>=256 || !range(a,root,28) || !absent(a,root)) goto fail;
        for (NativeLight* p=lights->first;p;p=p->next) if (p->offset==root) goto fail;
        NativeLight* n=calloc(1,sizeof(*n)); if (!n) goto fail;
        n->offset=root;
        if (previous) { previous->next=n; previous->desc.next=&n->desc; }
        else lights->first=n;
        lights->count++; previous=n;
        uint32_t flags; if (!melee_archive_u32(a,root+8,&flags)) goto fail;
        n->desc.flags=flags>>16; n->desc.attnflags=flags;
        memcpy(&n->desc.color,a->bytes+32+root+12,4);
        if (!world(a,root+16,&n->position,&n->desc.position) ||
            !world(a,root+20,&n->interest,&n->desc.interest) || !parameters(a,n)) goto fail;
        MeleeHostBool present;
        if (!melee_archive_pointer(a,root+4,&root,&present)) goto fail;
        if (!present) return lights;
    }
fail:
    melee_lights_free(lights); return NULL;
}

HSD_LightDesc* melee_lights_find(MeleeLights* lights,uint32_t offset){
    if(lights)for(NativeLight* p=lights->first;p;p=p->next)if(p->offset==offset)return &p->desc;
    return NULL;
}
