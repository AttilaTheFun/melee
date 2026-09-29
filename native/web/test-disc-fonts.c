#include "game/disc_fonts.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static uint8_t image[0x30000], text[287*512], debug[128*56];
static int fail_font_reads;
static void word(size_t offset, uint32_t value)
{
    for (unsigned i=0;i<4;++i) image[offset+i]=(uint8_t)(value>>(24-8*i));
}
static MeleeHostBool read_image(void* context, void* out, size_t length, uint64_t offset)
{
    (void)context;
    if (offset>sizeof(image) || length>sizeof(image)-offset ||
        (fail_font_reads && offset>=0x1100)) return false;
    memcpy(out,image+offset,length);return true;
}
static int load_fixture(void)
{
    MeleeDisc* disc=melee_disc_open_reader(sizeof(image),read_image,NULL);
    assert(disc);
    int valid=melee_browser_load_fonts(disc,text,debug);
    melee_disc_close(disc);return valid;
}
static void compare_file(const void* bytes, size_t length, const char* path)
{
    uint8_t* expected=malloc(length);assert(expected);
    FILE* file=fopen(path,"rb");assert(file);
    assert(fread(expected,1,length,file)==length && fgetc(file)==EOF);
    assert(!memcmp(bytes,expected,length));fclose(file);free(expected);
}
int main(int argc, char** argv)
{
    memcpy(image,"GALE01",6);image[7]=2;word(0x1c,0xc2339f3d);
    word(0x420,0x1000);word(0x424,0x440);word(0x428,13);
    word(0x440,0x01000000);word(0x448,1);
    word(0x1000,0x100);word(0x1048,0x804088b8);word(0x1090,0x28288);
    for(size_t i=0x1100;i<sizeof(image);++i)image[i]=(uint8_t)(i*37+11);
    assert(load_fixture());
    assert(!memcmp(debug,image+0x1100,sizeof(debug)));
    assert(!memcmp(text,image+0x1100+0x4488,sizeof(text)));
    image[7]=1;assert(!load_fixture());image[7]=2;
    word(0x1090,0x28287);assert(!load_fixture());word(0x1090,0x28288);
    word(0x1004,0x100);word(0x104c,0x804088b8);word(0x1094,0x28288);
    assert(!load_fixture());word(0x1094,0);
    word(0x1000,0xff);assert(!load_fixture());word(0x1000,0x100);
    fail_font_reads=1;assert(!load_fixture());fail_font_reads=0;
    if(argc==4){
        MeleeDisc* disc=melee_disc_open(argv[1]);assert(disc);
        assert(melee_browser_load_fonts(disc,text,debug));melee_disc_close(disc);
        compare_file(text,sizeof(text),argv[2]);compare_file(debug,sizeof(debug),argv[3]);
        puts("PASS local disc fonts match independently extracted DOL bytes");
    }
    puts("PASS font DOL mapping, bounds, ambiguous sections, wrong revision and failed reads");
}
