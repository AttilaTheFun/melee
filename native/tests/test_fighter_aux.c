#include "melee_fighter_aux.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static u32 ref(const MeleeArchive* a,u32 at){u32 target;MeleeHostBool p;assert(melee_archive_pointer(a,at,&target,&p)&&p);return target;}
int main(int argc,char** argv){
    assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);
    u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);MeleeArchive a;assert(melee_archive_open(&a,b,n));
    MeleeFighterAux value;assert(melee_fighter_aux_decode(&a,&value));MeleeFighterAux saved=value;
    u32 root;assert(melee_archive_find(&a,"ftLoadCommonData",&root));
    for(unsigned i=0;i<3;i++){
        u32 at=ref(&a,root+4*(17+i));assert(!memcmp(value.colors[i],b+32+at,20));
        MeleeArchive short_a=a;short_a.data_size=at+19;assert(!melee_fighter_aux_decode(&short_a,&value));assert(!memcmp(&value,&saved,sizeof(value)));
    }
    u32 crowd=ref(&a,root+84);
    for(unsigned i=0;i<68;i+=4){u32 raw,native;assert(melee_archive_u32(&a,crowd+i,&raw));memcpy(&native,(u8*)&value.crowd+i,4);assert(raw==native);}
    u8 first[4];memcpy(first,b+32+crowd,4);memcpy(b+32+crowd,"\x7f\xc0\0\0",4);
    assert(!melee_fighter_aux_decode(&a,&value));assert(!memcmp(&value,&saved,sizeof(value)));memcpy(b+32+crowd,first,4);
    MeleeArchive short_a=a;short_a.data_size=crowd+67;assert(!melee_fighter_aux_decode(&short_a,&value));
    memset(b,0xa5,n);free(b);assert(!memcmp(&value,&saved,sizeof(value)));
    puts("Fighter auxiliary data: 15 color records, 17 crowd parameters, invalid input and owned lifetime passed");
}
