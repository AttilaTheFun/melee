#include "melee_item_special.h"
#include <melee/it/itCommonItems.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t ref(const MeleeArchive* a,uint32_t at){uint32_t v;MeleeHostBool p;assert(melee_archive_pointer(a,at,&v,&p)&&p);return v;}
int main(int argc,char** argv){
    assert(argc==2);FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);rewind(f);uint8_t* bytes=malloc(n);assert(bytes&&fread(bytes,1,n,f)==n);fclose(f);
    MeleeArchive a;assert(melee_archive_open(&a,bytes,n));uint32_t root;assert(melee_archive_find(&a,"itPublicData",&root));uint32_t table=ref(&a,root+4);unsigned checked=0;
    for(unsigned kind=0;kind<It_Kind_Old_Kuri;kind++){
        if(kind>=43&&kind<It_PKind_Start)continue;
        size_t size=melee_item_special_size(kind);if(!size)continue;
        uint32_t entries=kind<43?table:ref(&a,root+12);
        unsigned index=kind<43?kind:kind-It_PKind_Start;
        uint32_t at=ref(&a,ref(&a,entries+4*index)+4);uint8_t* result=melee_item_special_decode(&a,kind,at);assert(result);
        if(kind==42){
            itEvYoshiEgg_DatAttrs* attrs=(void*)result;uint32_t value;assert(melee_archive_u32(&a,at,&value));assert(attrs->x0==(s32)value&&!attrs->x4);
            MeleeArchive short_view=a;short_view.data_size=at+7;assert(!melee_item_special_decode(&short_view,kind,at));free(result);checked++;continue;
        }
        for(unsigned i=0;i<size/4;i++){
            if((kind==It_PKind_Houou&&i==1)||(kind==12&&i>=9)||(kind==4&&i==9)||(kind==5&&i==1)||(kind==14&&i==6)||(kind==15&&(i==12||i==13))||(kind==38&&i>=27&&i<=29)){assert(!memcmp(result+4*i,bytes+32+at+4*i,4));continue;}
            uint32_t expected,actual;assert(melee_archive_u32(&a,at+4*i,&expected));memcpy(&actual,result+4*i,4);assert(actual==expected);
        }
        MeleeArchive short_view=a;short_view.data_size=at+size-1;assert(!melee_item_special_decode(&short_view,kind,at));
        free(result);checked++;
    }
    assert(checked==83&&!melee_item_special_decode(&a,43,0));
    /* Float-only flame schema rejects NaN, while integer capsule fields retain bits. */
    uint32_t at=ref(&a,ref(&a,table+41*4)+4);uint8_t saved[4];memcpy(saved,bytes+32+at,4);memcpy(bytes+32+at,"\x7f\xc0\0\0",4);
    assert(!melee_item_special_decode(&a,41,at));void* integer=melee_item_special_decode(&a,0,at);assert(integer);free(integer);memcpy(bytes+32+at,saved,4);
    void* owned=melee_item_special_decode(&a,41,at);assert(owned);uint32_t expected;assert(melee_archive_u32(&a,at,&expected));memset(bytes,0xa5,n);free(bytes);uint32_t actual;memcpy(&actual,owned,4);assert(actual==expected);free(owned);
    printf("Scalar item special attributes: %u schemas, every word/padding, bounds, NaN and owned lifetime passed\n",checked);
}
