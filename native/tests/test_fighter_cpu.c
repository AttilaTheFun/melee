#include "melee_fighter_cpu.h"
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftcmdscript.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct Fighter_804D64FC_t* Fighter_804D64FC;
void OSReport(char* text,...){(void)text;}
float HSD_Randf(void){return 0.25f;}
#undef __assert
void __assert(char* file,u32 line,char* message){fprintf(stderr,"%s:%d: %s\n",file,line,message);abort();}
void HSD_Panic(char* file,u32 line,char* message){fprintf(stderr,"%s:%d: %s\n",file,line,message);abort();}
static u32 ref(const MeleeArchive* a,u32 at){u32 v;MeleeHostBool p;assert(melee_archive_pointer(a,at,&v,&p)&&p);return v;}
int main(int argc,char** argv){
    assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>0);rewind(f);
    u8*b=malloc(size);assert(b&&fread(b,1,size,f)==size);fclose(f);MeleeArchive a;assert(melee_archive_open(&a,b,size));
    MeleeFighterCpu* owner=melee_fighter_cpu_decode(&a);assert(owner);Fighter_804D64FC=melee_fighter_cpu_header(owner);
    u32 root;assert(melee_archive_find(&a,"ftLoadCommonData",&root));u32 h=ref(&a,root+88),scripts=ref(&a,h);
    unsigned lengths[62]={0};u8 expected[62][256]={{0}};
    for(unsigned i=1;i<62;i++){
        u32 at=ref(&a,scripts+4*i);unsigned n=0;
        for(;;){u8 op=b[32+at+n];unsigned count=op>191?3:op>127?2:1;assert(n+count<=256);memcpy(expected[i]+n,b+32+at+n,count);n+=count;if(op==127)break;}
        lengths[i]=n;assert(!memcmp(Fighter_804D64FC->cmdscripts[i],expected[i],n));
    }
    void** tables[]={Fighter_804D64FC->x4,Fighter_804D64FC->x8,Fighter_804D64FC->xC,Fighter_804D64FC->x10,Fighter_804D64FC->x14,Fighter_804D64FC->x18,Fighter_804D64FC->x1C};
    unsigned total=0;
    for(unsigned t=0;t<7;t++)for(unsigned k=0;k<32;k++){
        u32 at=ref(&a,ref(&a,h+4*(t+1))+4*k);ftCo_AttackEntry* entries=tables[t][k];
        for(unsigned n=0;entries[n].cmd;n++){for(unsigned j=0;j<9;j++){u32 raw,native;assert(melee_archive_u32(&a,at+36*n+4*j,&raw));memcpy(&native,(u8*)&entries[n]+4*j,4);assert(raw==native);}total++;}
    }
    assert(total==1136);
    for(unsigned i=0;i<32;i++){float expected;assert(melee_archive_f32(&a,ref(&a,h+32)+4*i,&expected));assert(Fighter_804D64FC->x20[i]==expected);}
    for(unsigned i=0;i<6;i++){float expected;assert(melee_archive_f32(&a,ref(&a,h+36)+4*i,&expected));assert(((float*)Fighter_804D64FC->x24)[i]==expected);}
    u32 first=ref(&a,scripts+4);u8 saved=b[32+first];b[32+first]=0xff;assert(!melee_fighter_cpu_decode(&a));b[32+first]=saved;
    memset(b,0xa5,size);free(b);
    Fighter fighter={0};for(unsigned i=1;i<62;i++){ftCo_800B462C(&fighter);ftCo_800B4880(&fighter,i);assert(fighter.cpu.write_pos-fighter.cpu.buffer==lengths[i]);assert(!memcmp(fighter.cpu.buffer,expected[i],lengths[i]));}
    for(unsigned t=0;t<7;t++)for(unsigned k=0;k<32;k++){
        ftCo_AttackEntry* entries=tables[t][k];float sum=0,acc=0;int expected=0;
        for(unsigned i=0;entries[i].cmd;i++)sum+=entries[i].weight;
        if(sum>=0.00001f||sum<=-0.00001f){float inverse=1.0/sum;for(unsigned i=0;entries[i].cmd;i++){acc+=entries[i].weight;if(acc*inverse>=0.25f){expected=entries[i].cmd;break;}}}
        assert(ftCo_800B6208(entries)==expected);
    }
    melee_fighter_cpu_free(owner);Fighter_804D64FC=NULL;
    puts("Fighter CPU: 61 original script copies, 1136 attack records, 224 original weighted selections and source disposal passed");
}
