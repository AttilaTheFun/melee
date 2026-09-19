#include "melee_trophy_data.h"
#include <melee/ty/types.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(struct TrophyData)==36,"Trophy scalar layout");
_Static_assert(sizeof(struct TyDspEntry)==16,"Display scalar layout");
_Static_assert(sizeof(struct ToyNameData)==12,"Sort scalar layout");
struct MeleeTrophyData { void* tables[7];size_t counts[7]; };
void melee_trophy_data_free(MeleeTrophyData* d)
{if(d){for(unsigned i=0;i<7;i++)free(d->tables[i]);free(d);}}
void* melee_trophy_data_table(MeleeTrophyData* d,MeleeTrophyTable t)
{return d&&(unsigned)t<7?d->tables[t]:NULL;}
size_t melee_trophy_data_count(const MeleeTrophyData* d,MeleeTrophyTable t)
{return d&&(unsigned)t<7?d->counts[t]:0;}
MeleeTrophyData* melee_trophy_data_decode(const MeleeArchive* a)
{
    static const char* names[]={"tyInitModelTbl","tyInitModelDTbl","tyNoGetUsTbl",
        "tyExpDifferentTbl","tyModelSortTbl","tyDisplayModelTbl","tyDisplayModelUsTbl"};
    static const unsigned strides[]={36,36,2,2,12,16,16};
    uint32_t offsets[8];
    if(!a||a->reloc_count||a->extern_count||a->public_count!=7)return NULL;
    for(unsigned i=0;i<7;i++)if(!melee_archive_find(a,names[i],&offsets[i])||
        offsets[i]>=a->data_size||(i&&offsets[i]<=offsets[i-1]))return NULL;
    if(offsets[0])return NULL;
    offsets[7]=a->data_size;
    MeleeTrophyData* d=calloc(1,sizeof(*d));if(!d)return NULL;
    for(unsigned t=0;t<7;t++){
        size_t size=offsets[t+1]-offsets[t],stride=strides[t];
        if(size%stride||size/stride>4096)goto fail;
        d->counts[t]=size/stride;d->tables[t]=malloc(size);if(!d->tables[t])goto fail;
        unsigned char* out=d->tables[t];const unsigned char* in=a->bytes+32+offsets[t];
        memcpy(out,in,size);
        for(size_t row=0;row<size;row+=stride){
            if(t>=2&&t<=4){
                for(unsigned j=0;j<stride;j+=2){uint16_t v=(uint16_t)in[row+j]<<8|in[row+j+1];memcpy(out+row+j,&v,2);}
            }else{
                for(unsigned j=0;j<stride;j+=4){
                    if((t<=1&&j==32)||(t>=5&&j==4))continue;
                    uint32_t v;if(!melee_archive_u32(a,offsets[t]+row+j,&v))goto fail;
                    memcpy(out+row+j,&v,4);
                    if(j>=8){float f;memcpy(&f,&v,4);if(!isfinite(f))goto fail;}
                }
            }
        }
        if(t>=2&&t<=4){int16_t end;memcpy(&end,out+size-stride,2);if(end!=-1)goto fail;}
        else{int32_t end;memcpy(&end,out+size-stride,4);if(end!=-1)goto fail;}
    }
    return d;
fail:melee_trophy_data_free(d);return NULL;
}
