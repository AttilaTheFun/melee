#include "melee_audio_load_data.h"
#include <stdlib.h>
struct MeleeAudioLoadData {int* words;int* groups[4][30];};
_Static_assert(sizeof(int)==4,"Sound id width");
void melee_audio_load_data_free(MeleeAudioLoadData* d){if(d){free(d->words);free(d);}}
int** melee_audio_load_data_group(MeleeAudioLoadData* d,unsigned group){return d&&group<4?d->groups[group]:NULL;}
static int pointer(const MeleeArchive* a,uint32_t slot,uint32_t* target){
    MeleeHostBool present;return melee_archive_pointer(a,slot,target,&present)&&present;
}
MeleeAudioLoadData* melee_audio_load_data_decode(const MeleeArchive* a){
    uint32_t root,first;
    if(!a||a->extern_count||a->reloc_count!=124||!melee_archive_find(a,"lbAudioLoadData",&root)||
       root>a->data_size||a->data_size-root!=16||root<480||!pointer(a,root,&first)||first!=root-480||!first||(first&3))return NULL;
    MeleeAudioLoadData* d=calloc(1,sizeof(*d));if(!d)return NULL;
    d->words=malloc(first);if(!d->words)goto fail;
    for(unsigned i=0;i<first/4;i++){uint32_t word;if(!melee_archive_u32(a,4*i,&word))goto fail;d->words[i]=(int32_t)word;}
    for(unsigned g=0;g<4;g++){
        uint32_t table;if(!pointer(a,root+4*g,&table)||table!=first+120*g)goto fail;
        for(unsigned i=0;i<30;i++){
            uint32_t offset;if(!pointer(a,table+4*i,&offset)||offset>=first||(offset&3))goto fail;
            d->groups[g][i]=d->words+offset/4;
            unsigned k=offset/4;while(k<first/4&&d->words[k]!=0x83d60)k++;
            if(k==first/4)goto fail;
        }
    }
    return d;
fail:melee_audio_load_data_free(d);return NULL;
}
