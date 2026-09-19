/* Exercise the original ending-image loader with only its file/heap boundary
 * supplied by this test. No SDK decoder substitutes are linked. */
#include "../../src/melee/lb/lb_01F8.c"
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#undef __assert

static const void* source;
static size_t source_size;
void lbFile_80016760(const char* name, void** output, size_t* size)
{
    (void) name;
    *output = malloc(source_size);assert(*output);
    memcpy(*output, source, source_size);*size = source_size;
}
void* HSD_MemAlloc(ssize_t size) { return malloc(size); }
void __assert(char* file, u32 line, char* expression)
{
    (void) file;(void) line;(void) expression;abort();
}
void test_game_still(const void* input, size_t length, int width, int height,
                     const void* y, const void* u, const void* v,
                     size_t yn, size_t uvn)
{
    source=input;source_size=length;
    lbMthp8001FAA0("test-image.thp",width,height);
    assert((uintptr_t)lbl_804335B8.x20 > UINT32_MAX);
    assert(lbl_804335B8.x6C==width && lbl_804335B8.x6E==height);
    assert(!memcmp(lbl_804335B8.x20,y,yn));
    assert(!memcmp(lbl_804335B8.x44,u,uvn));
    assert(!memcmp(lbl_804335B8.x68,v,uvn));
    free(lbl_804335B8.unk94);free(lbl_804335B8.x20);
    free(lbl_804335B8.x44);free(lbl_804335B8.x68);
    memset(&lbl_804335B8,0,sizeof(lbl_804335B8));
}
