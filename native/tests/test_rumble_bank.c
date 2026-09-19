#include "melee_rumble_bank.h"
#include <melee/lb/types.h>
#include <sysdolphin/baselib/rumble.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(unsigned char* p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int main(int argc,char** argv)
{
    unsigned char* bytes;size_t size;
    if(argc==2){FILE* f=fopen(argv[1],"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);assert(n>0);rewind(f);
        size=n;bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
    }else{
        assert(argc==1);size=69;bytes=calloc(1,size);assert(bytes);
        word(bytes,size);word(bytes+4,12);word(bytes+8,1);word(bytes+12,1);
        bytes[32]=0x20;bytes[33]=2;bytes[40]=3;word(bytes+44,4);word(bytes+48,4);
        memcpy(bytes+56,"lbRumbleData",13);
    }
    MeleeArchive archive;assert(melee_archive_open(&archive,bytes,size));
    MeleeRumbleBank* bank=melee_rumble_bank_create(&archive);assert(bank);
    size_t count=melee_rumble_bank_count(bank);assert(count==(argc==2?40:1));
    bytes[32]=0xe0;assert(!melee_rumble_bank_create(&archive));
    memset(bytes,0xa5,size);free(bytes);
    struct Fighter_804D653C_t* entries=melee_rumble_bank_entries(bank);
    unsigned long long frames=0;
    for(size_t i=0;i<count;i++){
        u16* script=entries[i].unk;assert((uintptr_t)script>UINT32_MAX);
        HSD_PadRumbleListData state={0};state.headp=state.listp=script;state.frame=-2;
        unsigned steps=0;u8 status=0;
        while(!HSD_PadRumbleInterpret1(&state,&status)){assert(++steps<1000000);assert(status<=2);}
        frames+=steps;
    }
    melee_rumble_bank_free(bank);
    printf("Rumble bank: %zu owned scripts, %llu original-interpreter frames, invalid opcode rejection passed\n",count,frames);
}
