#include "../../src/sysdolphin/baselib/particle.c"
#include <assert.h>
#include <stdio.h>

static unsigned freed, hooks, app_removals, unrefs;
static HSD_JObj joint;
void HSD_ObjFree(HSD_ObjAllocData* pool, void* object)
{
    assert(pool == &hsd_804D0F60.alloc_data);
    assert(object != NULL);
    ++freed;
}
void HSD_JObjUnref(HSD_JObj* object) { assert(object == &joint); ++unrefs; }
int psRemoveParticleAppSRT(HSD_Particle* particle) { assert(particle->appsrt); ++app_removals; return 0; }
static int deleted(HSD_Particle* particle) { assert(particle->idnum == 42); ++hooks; return 0; }

int main(void)
{
    HSD_PSUserFunc callbacks = {0}; callbacks.hookDelete = deleted;
    HSD_Generator owner = {0}, other = {0};
    owner.idnum = 42; owner.linkNo = 15; owner.numChild = 4; owner.userfunc = &callbacks;
    HSD_Particle particles[5] = {0};
    for(unsigned i=0;i<5;++i){
        particles[i].next = i<4 ? &particles[i+1] : NULL;
        particles[i].gen = &owner;
        particles[i].idnum = 42;
    }
    particles[1].gen = &other;
    particles[3].idnum = 99;
    HSD_psAppSRT app = {0};
    particles[2].appsrt = &app;
    particles[4].kind = 0x8000 | (3<<12);
    hsd_804D08E8[3] = &joint;
    hsd_804D0908[15] = particles;
    hsd_804D78E2 = 5;
    hsd_8039D0A0(&owner);
    assert(hsd_804D0908[15] == &particles[1]);
    assert(particles[1].next == &particles[3] && particles[3].next == NULL);
    assert(freed == 3 && hooks == 3 && app_removals == 1 && unrefs == 1);
    assert(owner.numChild == 1 && hsd_804D78E2 == 2 && hsd_804D08E8[3] == NULL);
    hsd_8039D0A0(&owner);
    assert(freed == 3 && owner.numChild == 1);
    owner.linkNo = 0;
    hsd_8039D0A0(&owner);
    assert(freed == 3);
    puts("Particle cleanup: head/middle/tail deletion, owner filtering, hooks and allocator routing passed");
}
