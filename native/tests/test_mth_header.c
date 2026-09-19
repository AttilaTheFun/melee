#include "../../src/melee/lb/lbmthp.c"
#include "melee_disc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#undef __assert
static unsigned char header_bytes[64];
static size_t file_size;
void OSReport(char* format, ...){(void)format;abort();}
s32 DVDConvertPathToEntrynum(const char* path){(void)path;return 12;}
size_t lbFile_8001634C(int file){assert(file==12);return file_size;}
void lbFile_800161C4(int file,uintptr_t offset,uintptr_t output,size_t size,int type,int pri)
{
    assert(file==12&&!offset&&size==64&&type==0x21&&pri==1);
    assert(output>UINT32_MAX&&!(output&31));memcpy((void*)output,header_bytes,64);
}
void __assert(char* file,u32 line,char* expression)
{fprintf(stderr,"%s:%u %s\n",file,line,expression);abort();}
static void word(unsigned offset,unsigned value)
{for(unsigned i=0;i<4;i++)header_bytes[offset+i]=value>>(24-8*i);}
static void check(void)
{
    MeleeMTHHeader expected;assert(melee_mth_header(header_bytes,64,file_size,&expected));
    THPDecComp player={0};assert(fn_8001EB14(&player,"movie")==1);
    assert(player.width==expected.width&&player.height==expected.height);
    assert(player.unk_40==expected.frame_count&&player.unk_100==((expected.buffer_size+4+31)&~31u));
    assert(player.first_frame==expected.first_frame&&player.first_frame_size==expected.first_frame_size);
    assert(player.frame_rate==expected.frame_rate&&player.version==2);
    for(size_t n=0;n<64;n++){
        MeleeMTHHeader unchanged=expected;
        assert(!melee_mth_header(header_bytes,n,file_size,&unchanged));
        assert(!memcmp(&expected,&unchanged,sizeof(expected)));
    }
    assert(!melee_mth_header(header_bytes,64,expected.first_frame+expected.first_frame_size-1,&expected));
}
int main(int argc,char** argv)
{
    memcpy(header_bytes,"MTHP",4);word(8,2);word(12,128);word(16,640);word(20,480);
    word(24,30);word(28,2);word(32,64);word(40,64);file_size=192;check();
    word(16,0);MeleeMTHHeader invalid;assert(!melee_mth_header(header_bytes,64,file_size,&invalid));
    if(argc==2){MeleeDisc* disc=melee_disc_open(argv[1]);assert(disc);unsigned count=0;
        for(uint32_t i=0;i<melee_disc_entry_count(disc);i++){
            const MeleeDiscEntry* e=melee_disc_entry(disc,i);size_t n=strlen(e->name);
            if(!e->directory&&n>4&&!strcmp(e->name+n-4,".mth")){
                assert(melee_disc_read(disc,i,header_bytes,64,0));file_size=e->length;check();count++;
            }
        }
        printf("Retail MTH headers: %u original-loader checks passed\n",count);assert(count==28);melee_disc_close(disc);
    }
    puts("MTH headers: bounded big-endian parsing and original loader passed");
}
