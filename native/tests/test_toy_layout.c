#include <melee/ty/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
int main(void)
{
    unsigned char* bytes=malloc(sizeof(union ToyArchiveStorage)+32);assert(bytes);
    memset(bytes,0,sizeof(union ToyArchiveStorage));memset(bytes+sizeof(union ToyArchiveStorage),0xa5,32);
    ToyED8Data* state=(void*)bytes;
    ToyGlobalsS_* globals=(void*)state;
    struct tyLightData* light=(void*)state;
    TyArchiveData* archive=(void*)state;
    uintptr_t markers[8]={0};assert((uintptr_t)markers>UINT32_MAX);
    for(unsigned i=0;i<3;i++)state->jobjs[i]=(void*)&markers[i];
    state->gobj=(void*)&markers[3];
    globals->x50=&markers[4];assert(state->archive==(void*)&markers[4] && archive->data==globals->x50);
    globals->x54=(void*)&markers[5];assert(state->x54==globals->x54);
    light->x58=&markers[6];assert(state->x58==light->x58);
    light->x0C=(void*)&markers[7];assert((void*)state->xC==(void*)light->x0C);
    archive->gobj=(void*)&markers[0];assert((void*)state->x0==(void*)archive->gobj && globals->x0==archive->gobj);
    globals->x30=&markers[1];assert((void*)state->x30==globals->x30);
    state->archive=NULL;assert(!archive->data && !globals->x50);
    for(unsigned i=0;i<3;i++)assert(state->jobjs[i]==(void*)&markers[i]);
    assert(state->gobj==(void*)&markers[3]);
    for(unsigned i=0;i<32;i++)assert(bytes[sizeof(union ToyArchiveStorage)+i]==0xa5);
    free(bytes);puts("Trophy archive overlays preserve shared full-width pointers within the allocated host state.");
}
