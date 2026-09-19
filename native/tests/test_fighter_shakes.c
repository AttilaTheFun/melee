#include "melee_fighter_shakes.h"
#include <melee/ft/kinds/ftCommon/ftCo_DamageFall.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Fighter_ShakeTable_t* Fighter_804D6530;
static u32 ref(const MeleeArchive* a, u32 at)
{
    u32 target; MeleeHostBool present;
    assert(melee_archive_pointer(a, at, &target, &present) && present);
    return target;
}
static void word(u8* p, u32 value)
{
    p[0]=value>>24; p[1]=value>>16; p[2]=value>>8; p[3]=value;
}
int main(int argc, char** argv)
{
    assert(argc == 2);
    FILE* file = fopen(argv[1], "rb"); assert(file && !fseek(file, 0, SEEK_END));
    long size = ftell(file); assert(size > 0); rewind(file);
    u8* bytes = malloc(size); assert(bytes && fread(bytes, 1, size, file) == size); fclose(file);
    MeleeArchive a; assert(melee_archive_open(&a, bytes, size));
    MeleeFighterShakes* owner = melee_fighter_shakes_decode(&a); assert(owner);
    struct Fighter_ShakeTable_t* tables = melee_fighter_shakes_tables(owner);
    u32 root; assert(melee_archive_find(&a, "ftLoadCommonData", &root));
    u32 damage = ref(&a, root+36);
    u32 records[] = {damage, damage+8, damage+16, ref(&a,root+40), ref(&a,root+44)};
    Vec2 expected[5][255]; unsigned counts[5];
    for (unsigned i=0; i<5; ++i) {
        u32 at=ref(&a, records[i]); assert(melee_archive_u32(&a,records[i]+4,&counts[i]));
        assert(tables[i].x4==counts[i]);
        for (unsigned j=0; j<counts[i]; ++j) {
            assert(melee_archive_f32(&a, at+8*j, &expected[i][j].x));
            assert(melee_archive_f32(&a, at+8*j+4, &expected[i][j].y));
        }
        assert(!memcmp(expected[i],tables[i].x0,counts[i]*sizeof(Vec2)));
    }
    word(bytes+32+damage+4,0); assert(!melee_fighter_shakes_decode(&a));
    word(bytes+32+damage+4,256); assert(!melee_fighter_shakes_decode(&a));
    word(bytes+32+damage+4,counts[0]);
    u32 data=ref(&a,damage), saved; assert(melee_archive_u32(&a,data,&saved));
    word(bytes+32+data,0x7fc00000); assert(!melee_fighter_shakes_decode(&a));
    word(bytes+32+data,saved);
    MeleeArchive truncated=a; truncated.data_size=ref(&a,records[4])+counts[4]*8-1;
    assert(!melee_fighter_shakes_decode(&truncated));
    memset(bytes,0xa5,size); free(bytes);
    Fighter_804D6530=tables;
    Fighter fighter={0}; fighter.dmg.x18fa_model_shift_frames=1;
    fighter.dmg.x1900=0.6f; fighter.dmg.x1904=0.8f;
    for (unsigned kind=0; kind<3; ++kind) for (unsigned frame=0; frame<counts[kind]; ++frame)
        for (int facing=-1; facing<=1; facing+=2) {
            fighter.dmg.x18F8=kind; fighter.dmg.x18FC=frame; fighter.facing_dir=facing;
            Vec2 actual, wanted=expected[kind][frame];
            if (kind==1) {
                float x=wanted.x*facing;
                wanted.x=0.8f*x; wanted.y=-0.6f*x+wanted.y;
            } else {
                wanted.x *= facing;
            }
            assert(ftCo_80090690(&fighter,&actual)==&actual);
            assert(actual.x==wanted.x && actual.y==wanted.y);
        }
    fighter.dmg.x18fa_model_shift_frames=0; Vec2 untouched={123,456};
    assert(!ftCo_80090690(&fighter,&untouched)); assert(untouched.x==123 && untouched.y==456);
    for (unsigned i=3; i<5; ++i) assert(!memcmp(expected[i],tables[i].x0,counts[i]*sizeof(Vec2)));
    melee_fighter_shakes_free(owner); Fighter_804D6530=NULL;
    puts("Fighter shakes: five owned tables, original damage shifts in both directions, invalid inputs and source disposal passed");
}
