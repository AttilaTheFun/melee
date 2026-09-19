#include "melee_character_attributes.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* The two side-special views share the same four 32-bit state words. */
_Static_assert(offsetof(union ftKoopa_MotionVars,unk1.x4)==4,"Koopa state flag offset");
_Static_assert(offsetof(union ftKoopa_MotionVars,unk1.x8)==8,"Koopa direction offset");
_Static_assert(offsetof(union ftKoopa_MotionVars,unk1.xC)==12,"Koopa state flag offset");
int main(int argc,char** argv){
    union ftKoopa_MotionVars state={0};
    state.specials.x4=1;state.specials.facing_dir=-1;state.specials.xC=1;
    state.unk1.x0=0;
    assert(state.specials.x4==1&&state.specials.facing_dir==-1&&state.specials.xC==1);
    state.unk1.x8=0;
    assert(state.specials.facing_dir==0&&state.specials.xC==1);

    assert(argc>1);for(int i=1;i<argc;i++){
        FILE*f=fopen(argv[i],"rb");assert(f&&!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);
        u8*b=malloc(n);assert(b&&fread(b,1,n,f)==n);fclose(f);MeleeArchive a;assert(melee_archive_open(&a,b,n));
        const char* name;u32 root,at;MeleeHostBool present;
        assert(melee_archive_public(&a,0,&name,&root)&&!strncmp(name,"ftData",6));
        assert(melee_archive_pointer(&a,root,&at,&present)&&present);
        ftCo_DatAttrs value;assert(melee_character_attributes_decode(&a,root,&value));ftCo_DatAttrs saved=value;
        for(unsigned j=0;j<0x180;j+=4){u32 expected,actual;assert(melee_archive_u32(&a,at+j,&expected));memcpy(&actual,(u8*)&value+j,4);assert(actual==expected);}
        assert(!memcmp((u8*)&value+0x180,b+32+at+0x180,4));
        u8 first[4];memcpy(first,b+32+at,4);memcpy(b+32+at,"\x7f\xc0\0\0",4);
        assert(!melee_character_attributes_decode(&a,root,&value));assert(!memcmp(&value,&saved,sizeof(value)));memcpy(b+32+at,first,4);
        MeleeArchive truncated=a;truncated.data_size=at+0x183;assert(!melee_character_attributes_decode(&truncated,root,&value));
        if(!strcmp(name,"ftDataFox")||!strcmp(name,"ftDataFalco")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            struct ftFox_DatAttrs fox;assert(melee_fox_attributes_decode(&a,root,&fox));struct ftFox_DatAttrs fox_saved=fox;
            for(unsigned j=0;j<0xd0;j+=4){u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&fox+j,4);assert(actual==expected);}
            assert(!memcmp((u8*)&fox+0xd0,b+32+special+0xd0,4));
            u8 special_first[4];memcpy(special_first,b+32+special,4);memcpy(b+32+special,"\x7f\xc0\0\0",4);
            assert(!melee_fox_attributes_decode(&a,root,&fox));assert(!memcmp(&fox,&fox_saved,sizeof(fox)));memcpy(b+32+special,special_first,4);
            MeleeArchive short_special=a;short_special.data_size=special+0xd3;assert(!melee_fox_attributes_decode(&short_special,root,&fox));
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);assert(!memcmp(&fox,&fox_saved,sizeof(fox)));memcpy(b,backup,n);free(backup);
            printf("%s: 52 special-move scalar words and raw reflector behavior passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataNess")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftNessAttributes ness;assert(melee_ness_attributes_decode(&a,root,&ness));ftNessAttributes ness_saved=ness;
            for(unsigned j=0;j<0xd8;j+=4){u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&ness+j,4);assert(actual==expected);}
            assert(!memcmp((u8*)&ness+0xd8,b+32+special+0xd8,4));
            u8 original[4];memcpy(original,b+32+special+16,4);memcpy(b+32+special+16,"\x7f\xc0\0\0",4);
            assert(!melee_ness_attributes_decode(&a,root,&ness)&&!memcmp(&ness,&ness_saved,sizeof(ness)));memcpy(b+32+special+16,original,4);
            MeleeArchive short_special=a;short_special.data_size=special+0xdb;assert(!melee_ness_attributes_decode(&short_special,root,&ness));
            printf("%s: 54 Ness scalar words, raw reflector behavior and invalid input passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataMario")||!strcmp(name,"ftDataDrmario")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftMario_DatAttrs mario;assert(melee_mario_attributes_decode(&a,root,&mario));
            ftMario_DatAttrs saved_mario=mario;
            for(unsigned j=0;j<0x80;j+=4){u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&mario+j,4);assert(actual==expected);}
            assert(!memcmp((u8*)&mario+0x80,b+32+special+0x80,4));
            assert(mario.specials.cape_kind==(!strcmp(name,"ftDataMario")?It_Kind_Mario_Cape:It_Kind_DrMario_Sheet));
            assert(mario.cape_reflection.x14_size>0&&mario.cape_reflection.x4_max_damage>0);
            for(unsigned j=0;j<0x80;j+=4){
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0x14||j==0x50||j==0x5c||j==0x60||j==0x64;
                assert(melee_mario_attributes_decode(&a,root,&mario)==integer);
                if(!integer)assert(!memcmp(&mario,&saved_mario,sizeof(mario)));
                memcpy(b+32+special+j,original,4);mario=saved_mario;
            }
            for(unsigned length=0;length<sizeof(mario);length++){
                MeleeArchive truncated=a;truncated.data_size=special+length;
                assert(!melee_mario_attributes_decode(&truncated,root,&mario));
                assert(!memcmp(&mario,&saved_mario,sizeof(mario)));
            }
            assert(!melee_mario_attributes_decode(NULL,root,&mario));
            assert(!melee_mario_attributes_decode(&a,root,NULL));
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&mario,&saved_mario,sizeof(mario)));memcpy(b,backup,n);free(backup);
            printf("%s: Mario move/reflector parameters, every float validation, truncation and owned lifetime passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataLuigi")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftLuigiAttributes luigi;assert(melee_luigi_attributes_decode(&a,root,&luigi));
            ftLuigiAttributes saved_luigi=luigi;
            for(unsigned j=0;j<sizeof(luigi);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&luigi+j,4);assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0x88||j==0x94;
                assert(melee_luigi_attributes_decode(&a,root,&luigi)==integer);
                if(!integer)assert(!memcmp(&luigi,&saved_luigi,sizeof(luigi)));
                memcpy(b+32+special+j,original,4);luigi=saved_luigi;
            }
            for(unsigned length=0;length<sizeof(luigi);length++){
                MeleeArchive truncated=a;truncated.data_size=special+length;
                assert(!melee_luigi_attributes_decode(&truncated,root,&luigi));
                assert(!memcmp(&luigi,&saved_luigi,sizeof(luigi)));
            }
            printf("%s: Luigi move parameters, scalar types and all truncation boundaries passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataKoopa")||!strcmp(name,"ftDataGkoopa")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftKoopaAttributes koopa;assert(melee_koopa_attributes_decode(&a,root,&koopa));
            ftKoopaAttributes saved_koopa=koopa;
            for(unsigned j=0;j<sizeof(koopa);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&koopa+j,4);assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==4||j==0x20||j==0x2c||j==0x50;
                assert(melee_koopa_attributes_decode(&a,root,&koopa)==integer);
                if(!integer)assert(!memcmp(&koopa,&saved_koopa,sizeof(koopa)));
                memcpy(b+32+special+j,original,4);koopa=saved_koopa;
            }
            for(unsigned length=0;length<sizeof(koopa);length++){
                MeleeArchive truncated=a;truncated.data_size=special+length;
                assert(!melee_koopa_attributes_decode(&truncated,root,&koopa));
                assert(!memcmp(&koopa,&saved_koopa,sizeof(koopa)));
            }
            printf("%s: Koopa move parameters, scalar types and all truncation boundaries passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataDonkey")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftDonkeyAttributes donkey;assert(melee_donkey_attributes_decode(&a,root,&donkey));
            ftDonkeyAttributes saved_donkey=donkey;
            for(unsigned j=0;j<sizeof(donkey);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&donkey+j,4);assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0||j==4||j==0x2c||j==0x30;
                assert(melee_donkey_attributes_decode(&a,root,&donkey)==integer);
                if(!integer)assert(!memcmp(&donkey,&saved_donkey,sizeof(donkey)));
                memcpy(b+32+special+j,original,4);donkey=saved_donkey;
            }
            for(unsigned length=0;length<sizeof(donkey);length++){
                MeleeArchive truncated=a;truncated.data_size=special+length;
                assert(!melee_donkey_attributes_decode(&truncated,root,&donkey));
                assert(!memcmp(&donkey,&saved_donkey,sizeof(donkey)));
            }
            printf("%s: Donkey move parameters, scalar types and all truncation boundaries passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataCaptain")||!strcmp(name,"ftDataGanon")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            struct ftCaptain_DatAttrs captain;assert(melee_captain_attributes_decode(&a,root,&captain));
            struct ftCaptain_DatAttrs saved_captain=captain;
            for(unsigned j=0;j<sizeof(captain);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&captain+j,4);assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0x64||j==0x6c||j==0x78;
                assert(melee_captain_attributes_decode(&a,root,&captain)==integer);
                if(!integer)assert(!memcmp(&captain,&saved_captain,sizeof(captain)));
                memcpy(b+32+special+j,original,4);captain=saved_captain;
            }
            for(unsigned length=0;length<sizeof(captain);length++){
                MeleeArchive truncated=a;truncated.data_size=special+length;
                assert(!melee_captain_attributes_decode(&truncated,root,&captain));
                assert(!memcmp(&captain,&saved_captain,sizeof(captain)));
            }
            printf("%s: Captain/Ganon move parameters, scalar types and all truncation boundaries passed\n",argv[i]);
        }

        if(!strcmp(name,"ftDataMars")||!strcmp(name,"ftDataEmblem")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            MarsAttributes mars;assert(melee_mars_attributes_decode(&a,root,&mars));MarsAttributes saved_mars=mars;
            for(unsigned j=0;j<sizeof(mars);j+=4){
                int raw=j>=0x80&&j<0x8c;
                if(raw)assert(!memcmp((u8*)&mars+j,b+32+special+j,4));
                else{u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&mars+j,4);assert(actual==expected);}
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0||j==4||j==8||j==0x64||j==0x8c;
                assert(melee_mars_attributes_decode(&a,root,&mars)==(raw||integer));
                if(!raw&&!integer)assert(!memcmp(&mars,&saved_mars,sizeof(mars)));
                memcpy(b+32+special+j,original,4);mars=saved_mars;
            }
            for(unsigned length=0;length<sizeof(mars);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_mars_attributes_decode(&short_a,root,&mars));assert(!memcmp(&mars,&saved_mars,sizeof(mars)));
            }
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&mars,&saved_mars,sizeof(mars)));memcpy(b,backup,n);free(backup);
            printf("%s: Marth/Roy scalar types, sword bytes, truncation and source independence passed\n",argv[i]);
        }

        if(!strcmp(name,"ftDataPikachu")||!strcmp(name,"ftDataPichu")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftPikachuAttributes pika;assert(melee_pikachu_attributes_decode(&a,root,&pika));ftPikachuAttributes saved_pika=pika;
            for(unsigned j=0;j<sizeof(pika);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&pika+j,4);assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0x14||j==0x18||j==0x5c||j==0x60||j==0xa0||j==0xa8||j==0xd4||j==0xd8||j==0xdc;
                assert(melee_pikachu_attributes_decode(&a,root,&pika)==integer);
                if(!integer)assert(!memcmp(&pika,&saved_pika,sizeof(pika)));
                memcpy(b+32+special+j,original,4);pika=saved_pika;
            }
            for(unsigned length=0;length<sizeof(pika);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_pikachu_attributes_decode(&short_a,root,&pika));assert(!memcmp(&pika,&saved_pika,sizeof(pika)));
            }
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&pika,&saved_pika,sizeof(pika)));memcpy(b,backup,n);free(backup);
            printf("%s: Pikachu/Pichu parameters, item IDs, collision box, invalid inputs and source independence passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataLink")||!strcmp(name,"ftDataClink")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            struct ftLk_DatAttrs pika;assert(melee_link_attributes_decode(&a,root,&pika));struct ftLk_DatAttrs saved_pika=pika;
            for(unsigned j=0;j<sizeof(pika);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&pika+j,4);int raw=j==0x6c||j==0x70||j==0x74||j==0xc0;
                if(raw)assert(!memcmp((u8*)&pika+j,b+32+special+j,4));else assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0xc||j==0x10||j==0x2c||j==0x48||j==0x58||j==0x5c||j==0x60||j==0x78||j==0x84||j==0x88||j==0x8c||j==0x90||j==0x94||j==0x98||j==0x9c||j==0xa0||j==0xa4||j==0xa8||j==0xac||j==0xb0||j==0xb8||j==0xbc||j==0xc4;
                assert(melee_link_attributes_decode(&a,root,&pika)==(raw||integer));
                if(!raw&&!integer)assert(!memcmp(&pika,&saved_pika,sizeof(pika)));
                memcpy(b+32+special+j,original,4);pika=saved_pika;
            }
            for(unsigned length=0;length<sizeof(pika);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_link_attributes_decode(&short_a,root,&pika));assert(!memcmp(&pika,&saved_pika,sizeof(pika)));
            }
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&pika,&saved_pika,sizeof(pika)));memcpy(b,backup,n);free(backup);
            printf("%s: Link/Young Link parameters, sword bytes and integer fields, invalid inputs and source independence passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataPopo")||!strcmp(name,"ftDataNana")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftIceClimberAttributes iceclimbers;assert(melee_iceclimbers_attributes_decode(&a,root,&iceclimbers));ftIceClimberAttributes saved_iceclimbers=iceclimbers;
            for(unsigned j=0;j<sizeof(iceclimbers);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&iceclimbers+j,4);
                int raw=j==0xcc||(j>=0xd4&&j<0x12c)||j>=0x150;if(raw)assert(!memcmp((u8*)&iceclimbers+j,b+32+special+j,4));else assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0x1c||j==0x68;
                assert(melee_iceclimbers_attributes_decode(&a,root,&iceclimbers)==(integer||raw));
                if(!integer&&!raw)assert(!memcmp(&iceclimbers,&saved_iceclimbers,sizeof(iceclimbers)));
                memcpy(b+32+special+j,original,4);iceclimbers=saved_iceclimbers;
            }
            for(unsigned length=0;length<sizeof(iceclimbers);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_iceclimbers_attributes_decode(&short_a,root,&iceclimbers));assert(!memcmp(&iceclimbers,&saved_iceclimbers,sizeof(iceclimbers)));
            }
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&iceclimbers,&saved_iceclimbers,sizeof(iceclimbers)));memcpy(b,backup,n);free(backup);
            printf("%s: Ice Climbers parameters and integer fields, invalid inputs and source independence passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataKirby")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            struct ftKb_DatAttrs kb;assert(melee_kirby_attributes_decode(&a,root,&kb));
            struct ftKb_DatAttrs saved_kb=kb;
            const unsigned integers[]={0,0x28,0x2c,0x30,0xec,0xf0,0x108,0x10c,0x110,0x118,0x134,0x140,0x15c,0x170,0x190,0x194,0x1a0,0x1a4,0x1a8,0x1ac,0x1d4,0x1d8,0x1f0,0x1f4,0x23c,0x240,0x260,0x264,0x274,0x278,0x288,0x28c,0x2a8,0x2b4,0x2b8,0x2f0,0x31c,0x35c,0x360,0x364,0x370,0x374,0x378,0x390,0x394,0x3d4,0x3ec,0x400,0x404};
            for(unsigned j=0;j<sizeof(kb);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&kb+j,4);
                if(j==0x34){assert((u16)kb.jumpaerial_unk==(expected>>16));assert(!memcmp((u8*)&kb+j+2,b+32+special+j+2,2));}
                else if(j==0x420)assert(!memcmp((u8*)&kb+j,b+32+special+j,4));
                else assert(actual==expected);
                int scalar_integer=0;for(unsigned k=0;k<sizeof(integers)/sizeof(*integers);k++)if(j==integers[k])scalar_integer=1;
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                assert(melee_kirby_attributes_decode(&a,root,&kb)==(scalar_integer||j==0x34||j==0x420));
                if(!scalar_integer&&j!=0x34&&j!=0x420)assert(!memcmp(&kb,&saved_kb,sizeof(kb)));
                memcpy(b+32+special+j,original,4);kb=saved_kb;
            }
            for(unsigned length=0;length<sizeof(kb);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_kirby_attributes_decode(&short_a,root,&kb)&&!memcmp(&kb,&saved_kb,sizeof(kb)));
            }
            /* Distinct halfword, padding and behavior bytes catch accidental word swaps. */
            u8 originals[8];memcpy(originals,b+32+special+0x34,4);memcpy(originals+4,b+32+special+0x420,4);
            memcpy(b+32+special+0x34,"\xff\x80\x12\x34",4);memcpy(b+32+special+0x420,"\x01\x56\x78\x9a",4);
            assert(melee_kirby_attributes_decode(&a,root,&kb));assert(kb.jumpaerial_unk==-128);
            assert(!memcmp((u8*)&kb+0x36,"\x12\x34",2)&&!memcmp((u8*)&kb+0x420,"\x01\x56\x78\x9a",4));
            memcpy(b+32+special+0x34,originals,4);memcpy(b+32+special+0x420,originals+4,4);kb=saved_kb;
            assert(!melee_kirby_attributes_decode(NULL,root,&kb)&&!melee_kirby_attributes_decode(&a,root,NULL));
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&kb,&saved_kb,sizeof(kb)));memcpy(b,backup,n);free(backup);
            printf("%s: Kirby base/copy move parameters, signed halfword, raw behavior, nonfinite rejection and truncation passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataGamewatch")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftGameWatchAttributes gamewatch;assert(melee_gamewatch_attributes_decode(&a,root,&gamewatch));ftGameWatchAttributes saved_gamewatch=gamewatch;
            for(unsigned j=0;j<sizeof(gamewatch);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&gamewatch+j,4);
                int raw=j>=4&&j<=0x14;if(raw)assert(!memcmp((u8*)&gamewatch+j,b+32+special+j,4));else assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=(j>=0x34&&j<=0x54)||j==0x80;
                assert(melee_gamewatch_attributes_decode(&a,root,&gamewatch)==(integer||raw));
                if(!integer&&!raw)assert(!memcmp(&gamewatch,&saved_gamewatch,sizeof(gamewatch)));
                memcpy(b+32+special+j,original,4);gamewatch=saved_gamewatch;
            }
            for(unsigned length=0;length<sizeof(gamewatch);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_gamewatch_attributes_decode(&short_a,root,&gamewatch));assert(!memcmp(&gamewatch,&saved_gamewatch,sizeof(gamewatch)));
            }
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&gamewatch,&saved_gamewatch,sizeof(gamewatch)));memcpy(b,backup,n);free(backup);
            printf("%s: GameWatch parameters and integer fields, invalid inputs and source independence passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataMewtwo")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftMewtwoAttributes mewtwo;assert(melee_mewtwo_attributes_decode(&a,root,&mewtwo));ftMewtwoAttributes saved_mewtwo=mewtwo;
            for(unsigned j=0;j<sizeof(mewtwo);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&mewtwo+j,4);
                int raw=j==0x3c;if(raw)assert(!memcmp((u8*)&mewtwo+j,b+32+special+j,4));else assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0xc||j==0x10||j==0x1c||j==0x20||j==0x50||j==0x68;
                assert(melee_mewtwo_attributes_decode(&a,root,&mewtwo)==(integer||raw));
                if(!integer&&!raw)assert(!memcmp(&mewtwo,&saved_mewtwo,sizeof(mewtwo)));
                memcpy(b+32+special+j,original,4);mewtwo=saved_mewtwo;
            }
            for(unsigned length=0;length<sizeof(mewtwo);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_mewtwo_attributes_decode(&short_a,root,&mewtwo));assert(!memcmp(&mewtwo,&saved_mewtwo,sizeof(mewtwo)));
            }
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&mewtwo,&saved_mewtwo,sizeof(mewtwo)));memcpy(b,backup,n);free(backup);
            printf("%s: Mewtwo parameters and integer fields, invalid inputs and source independence passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataPeach")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftPe_DatAttrs peach;assert(melee_peach_attributes_decode(&a,root,&peach));ftPe_DatAttrs saved_peach=peach;
            for(unsigned j=0;j<sizeof(peach);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&peach+j,4);
                assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=(j>=0x10&&j<=0x30)||j==0x90||j==0xac;
                assert(melee_peach_attributes_decode(&a,root,&peach)==integer);
                if(!integer)assert(!memcmp(&peach,&saved_peach,sizeof(peach)));
                memcpy(b+32+special+j,original,4);peach=saved_peach;
            }
            for(unsigned length=0;length<sizeof(peach);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_peach_attributes_decode(&short_a,root,&peach));assert(!memcmp(&peach,&saved_peach,sizeof(peach)));
            }
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&peach,&saved_peach,sizeof(peach)));memcpy(b,backup,n);free(backup);
            printf("%s: Peach parameters and integer fields, invalid inputs and source independence passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataSamus")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftSs_DatAttrs samus;assert(melee_samus_attributes_decode(&a,root,&samus));ftSs_DatAttrs saved_samus=samus;
            for(unsigned j=0;j<sizeof(samus);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&samus+j,4);
                assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0x20||(j>=0x9c&&j<=0xc8)||j==0xd0;
                assert(melee_samus_attributes_decode(&a,root,&samus)==integer);
                if(!integer)assert(!memcmp(&samus,&saved_samus,sizeof(samus)));
                memcpy(b+32+special+j,original,4);samus=saved_samus;
            }
            for(unsigned length=0;length<sizeof(samus);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_samus_attributes_decode(&short_a,root,&samus));assert(!memcmp(&samus,&saved_samus,sizeof(samus)));
            }
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&samus,&saved_samus,sizeof(samus)));memcpy(b,backup,n);free(backup);
            printf("%s: Samus parameters and integer fields, invalid inputs and source independence passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataSeak")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftSeakAttributes pika;assert(melee_sheik_attributes_decode(&a,root,&pika));ftSeakAttributes saved_pika=pika;
            for(unsigned j=0;j<sizeof(pika);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&pika+j,4);int raw=0;
                if(raw)assert(!memcmp((u8*)&pika+j,b+32+special+j,4));else assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0x38||j==0x50;
                assert(melee_sheik_attributes_decode(&a,root,&pika)==(raw||integer));
                if(!raw&&!integer)assert(!memcmp(&pika,&saved_pika,sizeof(pika)));
                memcpy(b+32+special+j,original,4);pika=saved_pika;
            }
            for(unsigned length=0;length<sizeof(pika);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_sheik_attributes_decode(&short_a,root,&pika));assert(!memcmp(&pika,&saved_pika,sizeof(pika)));
            }
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&pika,&saved_pika,sizeof(pika)));memcpy(b,backup,n);free(backup);
            printf("%s: Sheik parameters and integer fields, invalid inputs and source independence passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataZelda")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftZelda_DatAttrs pika;assert(melee_zelda_attributes_decode(&a,root,&pika));ftZelda_DatAttrs saved_pika=pika;
            for(unsigned j=0;j<sizeof(pika);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&pika+j,4);int raw=j==0xa4;
                if(raw)assert(!memcmp((u8*)&pika+j,b+32+special+j,4));else assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==4||j==0x10||j==0x14||j==0x18||j==0x1c||j==0x48||j==0x60||j==0x84||j==0x88;
                assert(melee_zelda_attributes_decode(&a,root,&pika)==(raw||integer));
                if(!raw&&!integer)assert(!memcmp(&pika,&saved_pika,sizeof(pika)));
                memcpy(b+32+special+j,original,4);pika=saved_pika;
            }
            for(unsigned length=0;length<sizeof(pika);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_zelda_attributes_decode(&short_a,root,&pika));assert(!memcmp(&pika,&saved_pika,sizeof(pika)));
            }
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&pika,&saved_pika,sizeof(pika)));memcpy(b,backup,n);free(backup);
            printf("%s: Zelda parameters, reflector behavior byte, invalid inputs and source independence passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataYoshi")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftYoshiAttributes pika;assert(melee_yoshi_attributes_decode(&a,root,&pika));ftYoshiAttributes saved_pika=pika;
            for(unsigned j=0;j<sizeof(pika);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&pika+j,4);int raw=j>=0x12c;
                if(raw)assert(!memcmp((u8*)&pika+j,b+32+special+j,4));else assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0||j==0x38||j==0x48||j==0x4c||j==0x50||j==0xa4||j==0xdc;
                assert(melee_yoshi_attributes_decode(&a,root,&pika)==(raw||integer));
                if(!raw&&!integer)assert(!memcmp(&pika,&saved_pika,sizeof(pika)));
                memcpy(b+32+special+j,original,4);pika=saved_pika;
            }
            for(unsigned length=0;length<sizeof(pika);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_yoshi_attributes_decode(&short_a,root,&pika));assert(!memcmp(&pika,&saved_pika,sizeof(pika)));
            }
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&pika,&saved_pika,sizeof(pika)));memcpy(b,backup,n);free(backup);
            printf("%s: Yoshi parameters, shared views, preserved byte table, invalid inputs and source independence passed\n",argv[i]);
        }
        if(!strcmp(name,"ftDataPurin")){
            u32 special;assert(melee_archive_pointer(&a,root+4,&special,&present)&&present);
            ftPurinAttributes pika;assert(melee_purin_attributes_decode(&a,root,&pika));ftPurinAttributes saved_pika=pika;
            for(unsigned j=0;j<sizeof(pika);j+=4){
                u32 expected,actual;assert(melee_archive_u32(&a,special+j,&expected));memcpy(&actual,(u8*)&pika+j,4);int raw=j==0x48||j==0x60||j==0x64||j==0xb0||j>=0xf8;
                if(raw)assert(!memcmp((u8*)&pika+j,b+32+special+j,4));else assert(actual==expected);
                u8 original[4];memcpy(original,b+32+special+j,4);memcpy(b+32+special+j,"\x7f\xc0\0\0",4);
                int integer=j==0||j==0x28||j==0x2c||j==0x30||j==0x34||j==0x38||j==0x70||j==0x9c||j==0xe8||j==0xec;
                assert(melee_purin_attributes_decode(&a,root,&pika)==(raw||integer));
                if(!raw&&!integer)assert(!memcmp(&pika,&saved_pika,sizeof(pika)));
                memcpy(b+32+special+j,original,4);pika=saved_pika;
            }
            for(unsigned length=0;length<sizeof(pika);length++){
                MeleeArchive short_a=a;short_a.data_size=special+length;
                assert(!melee_purin_attributes_decode(&short_a,root,&pika));assert(!memcmp(&pika,&saved_pika,sizeof(pika)));
            }
            u8* backup=malloc(n);assert(backup);memcpy(backup,b,n);memset(b,0xa5,n);
            assert(!memcmp(&pika,&saved_pika,sizeof(pika)));memcpy(b,backup,n);free(backup);
            printf("%s: Jigglypuff parameters, multi-jump scalars, unused bytes, invalid inputs and source independence passed\n",argv[i]);
        }
        memset(b,0xa5,n);free(b);assert(!memcmp(&value,&saved,sizeof(value)));
        printf("%s: 96 scalar words, raw throw mask/padding, invalid input and owned lifetime passed\n",argv[i]);
    }
}
