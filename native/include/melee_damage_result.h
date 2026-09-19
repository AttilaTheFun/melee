#ifndef MELEE_NATIVE_DAMAGE_RESULT_H
#define MELEE_NATIVE_DAMAGE_RESULT_H
#include <melee/ft/types.h>
typedef struct MeleeDamageResult {
    float dir;
    int angle, hurt_height;
    float kb;
    Vec3 pos;
    u32 element;
    int sfx_severity;
    HSD_GObj* source;
    float damage;
} MeleeDamageResult;
/* The two console overlays have different native alignment. Store named
 * fields rather than writing a pointer-bearing struct over either prefix. */
static inline void melee_damage_result_store(Fighter* fp,
    const MeleeDamageResult* r, int primary)
{
    if(primary){
        fp->dmg.facing_dir_1=r->dir; fp->dmg.x1848_kb_angle=r->angle;
        fp->dmg.x184c_damaged_hurtbox=r->hurt_height; fp->dmg.kb_applied=r->kb;
        fp->dmg.x1854_collpos=r->pos; fp->dmg.x1860_element=r->element;
        fp->dmg.x1864=r->sfx_severity; fp->dmg.x1868_source=r->source;
        fp->dmg.x186c=r->damage;
    }else{
        fp->dmg.x1870=r->dir; fp->dmg.x1874=r->angle;
        fp->dmg.x1878=r->hurt_height; fp->dmg.x187c=r->kb;
        fp->dmg.x1880=r->pos; fp->dmg.x188c=r->element;
        fp->dmg.x1890=r->sfx_severity; fp->dmg.x1894=r->source;
        fp->dmg.x1898=r->damage;
    }
}
#endif
