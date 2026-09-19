#include "melee_item_colors.h"
#include <melee/lb/lb_013B.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void lbBgFlash_80021C48(int a,int b){(void)a;(void)b;abort();}
int main(int argc,char** argv){
    assert(argc==2);FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);rewind(f);u8* bytes=malloc(size);assert(bytes&&fread(bytes,1,size,f)==size);fclose(f);
    MeleeArchive a;assert(melee_archive_open(&a,bytes,size));u32 root,table;MeleeHostBool present;assert(melee_archive_find(&a,"itPublicData",&root)&&melee_archive_pointer(&a,root+20,&table,&present)&&present);
    MeleeItemColors* owner=melee_item_colors_decode(&a,table,7);assert(owner);struct Fighter_804D653C_t* entries=melee_item_colors_entries(owner);assert(!entries[0].unk);
    for(unsigned i=1;i<7;i++){assert(entries[i].unk&&entries[i].unk4==bytes[32+table+8*i+4]&&entries[i].unk5==bytes[32+table+8*i+5]);}
    assert(!melee_item_colors_decode(&a,table,0));MeleeArchive short_view=a;short_view.data_size=table+55;assert(!melee_item_colors_decode(&short_view,table,7));
    u32 script;assert(melee_archive_pointer(&a,table+16,&script,&present)&&present);u8 saved[4];memcpy(saved,bytes+32+script,4);
    memcpy(bytes+32+script,"\x0c\0\0\0",4);assert(!melee_item_colors_decode(&a,table,7));memcpy(bytes+32+script,saved,4);
    memset(bytes,0xa5,size);free(bytes);unsigned finished=0,changed=0;
    for(unsigned i=1;i<7;i++){
        ColorOverlay co={0};assert(lb_800144C8(&co,entries,i,0));GXColor last={0};unsigned transitions=0;
        for(unsigned frame=0;frame<600;frame++){
            int done=lb_80014258(NULL,&co,NULL);if(memcmp(&last,&co.x2C_hex,4)){last=co.x2C_hex;transitions++;}
            if(i==2&&frame==0)assert(co.x2C_hex.r==160&&co.x2C_hex.g==0&&co.x2C_hex.b==0&&co.x2C_hex.a==80);
            if(done){finished++;break;}
        }
        if(i>1)assert(transitions);changed+=transitions;lb_80014498(&co);
    }
    assert(finished==4);melee_item_colors_free(owner);printf("Item colors: six scripts, %u color transitions, four completed/two looping; original interpreter and owned lifetime passed\n",changed);
}
