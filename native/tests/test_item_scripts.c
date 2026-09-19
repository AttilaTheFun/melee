#include "melee_item_scripts.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void word(u8* p,u32 v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void sparse_branch_graph(void){
    const u32 data_size=1024*1024,root=64,sub=data_size-16;
    const size_t size=32+data_size+8;
    u8* bytes=calloc(1,size);assert(bytes);
    word(bytes,size);word(bytes+4,data_size);word(bytes+8,2);
    word(bytes+32+root,5u<<26);word(bytes+32+root+4,sub);
    word(bytes+32+root+8,7u<<26);word(bytes+32+root+12,8);
    word(bytes+32+sub,6u<<26);
    word(bytes+32+data_size,root+4);word(bytes+36+data_size,root+12);
    MeleeArchive a;assert(melee_archive_open(&a,bytes,size));
    MeleeItemScript* owner=melee_item_script_decode(&a,root);assert(owner);
    union CmdUnion* native=melee_item_script_root(owner);
    /* Six reachable words, despite a one-megabyte gap in the disc graph. */
    assert(native[1].Command_05.ptr==native+4);
    assert(native[3].Command_05.ptr==native-1);
    assert(native[4].unk0.opcode==6&&native[-1].unk0.opcode==0);
    word(bytes+32+root+12,root+4);assert(!melee_item_script_decode(&a,root));
    memset(bytes,0xa5,size);free(bytes);
    assert(native[1].Command_05.ptr->unk0.opcode==6);
    melee_item_script_free(owner);
}
int main(int argc,char** argv){
    sparse_branch_graph();
    size_t size=95;u8* b=calloc(1,size);assert(b);word(b,size);word(b+4,40);word(b+8,1);word(b+12,1);
    word(b+36,12);word(b+44,0x2c123abc);word(b+48,0xffedffff);word(b+52,0x80007fff);word(b+56,0xabcdefed);word(b+60,0xdeadbeef);word(b+64,0x3cf45000);
    word(b+72,4);memcpy(b+84,"ALDYakuAll",11);
    if(argc==2){free(b);FILE* f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);size=n;b=malloc(size);assert(b&&fread(b,1,size,f)==size);fclose(f);}else assert(argc==1);
    MeleeArchive a;assert(melee_archive_open(&a,b,size));MeleeItemScripts* s=melee_item_scripts_decode(&a);assert(s);
    union CmdUnion** table=melee_item_scripts_table(s);unsigned n=melee_item_scripts_count(s);assert(n&&!table[0]&&!table[n+1]);u32 root;assert(melee_archive_find(&a,"ALDYakuAll",&root));
    for(unsigned i=1;i<=n;i++){
        u32 at,w[7];MeleeHostBool present;assert(melee_archive_pointer(&a,root+4*i,&at,&present)&&present);
        for(unsigned k=0;k<7;k++)assert(melee_archive_u32(&a,at+4*k,w+k));
        union CmdUnion* d=table[i];
        struct it_create_hitbox_0 h=d[0].it_create_hitbox_0;
        assert(w[0]==(((u32)h.opcode<<26)|((u32)h.id<<23)|((u32)h.hit_group<<20)|((u32)h.bone<<13)|h.damage));
        assert(w[1]==(((u32)d[1].create_hitbox_1.size<<16)|((u32)d[1].create_hitbox_1.z_offset&65535)));
        assert(w[2]==(((u32)d[2].create_hitbox_2.y_offset<<16)|((u32)d[2].create_hitbox_2.x_offset&65535)));
        struct spawn_hitbox_3 k=d[3].create_hitbox_3;
        assert(w[3]==(((u32)k.angle<<23)|((u32)k.knockback_growth<<14)|((u32)k.weight_set_knockback<<5)|((u32)k.item_hit_interaction<<4)|((u32)k.ignore_thrown_fighters<<3)|((u32)k.ignore_fighter_scale<<2)|((u32)k.clank<<1)|k.rebound));
        struct it_create_hitbox_4 q=d[4].it_create_hitbox_4;
        assert(w[4]==(((u32)q.base_knockback<<23)|((u32)q.element<<18)|((u32)q.x40_b0<<17)|(((u32)q.shield_damage&255)<<9)|((u32)q.sfx_severity<<6)|((u32)q.sfx_kind<<2)|((u32)q.x40_b3<<1)|q.x40_b2));
        assert(!memcmp(&d[5],b+32+at+20,4));assert(!w[6]&&!d[6].unk0.opcode);
    }
    u32 first;MeleeHostBool present;assert(melee_archive_pointer(&a,root+4,&first,&present)&&present);
    u32 saved;assert(melee_archive_u32(&a,first,&saved));
    word(b+32+first,0x80000000);assert(!melee_item_scripts_decode(&a));word(b+32+first,0x2e000000);assert(!melee_item_scripts_decode(&a));
    word(b+32+first,saved);a.data_size=first+24;assert(!melee_item_scripts_decode(&a));
    memset(b,0xa5,size);free(b);assert(table[1][0].unk0.opcode==11);melee_item_scripts_free(s);
    printf("Item scripts: %u hitbox scripts, every decoded field/raw flag byte, unsupported opcode/id and owned lifetime passed\n",n);
}
