#include "melee_fighter_modifiers.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void word(u8* p,u32 v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int main(int argc,char** argv)
{
    assert(argc==2);
    FILE* file=fopen(argv[1],"rb");assert(file&&!fseek(file,0,SEEK_END));
    long size=ftell(file);assert(size>0);rewind(file);
    u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,file)==size);fclose(file);
    MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
    MeleeFighterModifiers value;assert(melee_fighter_modifiers_decode(&a,&value));
    MeleeFighterModifiers saved=value;
    u32 root;assert(melee_archive_find(&a,"ftLoadCommonData",&root));
    struct Field {unsigned entry;const void* data;size_t size;} fields[]={
        {1,value.throws,sizeof(value.throws)}, {2,value.swing,sizeof(value.swing)},
        {3,value.staling,sizeof(value.staling)}, {12,&value.scale,sizeof(value.scale)},
        {13,&value.bunny,sizeof(value.bunny)}, {14,&value.metal,sizeof(value.metal)},
        {15,&value.gravity,sizeof(value.gravity)}
    };
    unsigned words=0;
    for(unsigned i=0;i<7;i++){
        u32 at;MeleeHostBool present;
        assert(melee_archive_pointer(&a,root+4*fields[i].entry,&at,&present)&&present);
        for(unsigned j=0;j<fields[i].size;j+=4){
            u32 expected,actual;assert(melee_archive_u32(&a,at+j,&expected));
            memcpy(&actual,(u8*)fields[i].data+j,4);assert(actual==expected);++words;
        }
        u32 first;assert(melee_archive_u32(&a,at,&first));
        word(bytes+32+at,0x7fc00000);assert(!melee_fighter_modifiers_decode(&a,&value));
        assert(!memcmp(&value,&saved,sizeof(value)));word(bytes+32+at,first);
        MeleeArchive truncated=a;truncated.data_size=at+fields[i].size-1;
        assert(!melee_fighter_modifiers_decode(&truncated,&value));
        assert(!memcmp(&value,&saved,sizeof(value)));
    }
    memset(bytes,0xa5,size);free(bytes);assert(!memcmp(&value,&saved,sizeof(value)));
    printf("Fighter modifiers: seven tables, %u scalar values, NaN/truncation rejection and source disposal passed\n",words);
}
