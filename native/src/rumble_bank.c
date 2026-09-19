#include "melee_rumble_bank.h"
#include <melee/lb/types.h>
#include <stdlib.h>

struct MeleeRumbleBank {
    size_t count;
    struct Fighter_804D653C_t* entries;
    uint16_t* words;
};
void melee_rumble_bank_free(MeleeRumbleBank* bank)
{if(bank){free(bank->entries);free(bank->words);free(bank);}}
size_t melee_rumble_bank_count(const MeleeRumbleBank* bank){return bank?bank->count:0;}
struct Fighter_804D653C_t* melee_rumble_bank_entries(MeleeRumbleBank* bank){return bank?bank->entries:NULL;}
static int valid_script(const uint16_t* words,size_t start,size_t count)
{
    int loop=0;
    for(size_t i=start;i<count;i++){
        unsigned op=words[i]>>13,value=words[i]&8191;
        if(op==0)return !loop;
        if(op>=1&&op<=3){if(!value)return 0;}
        /* The SDK has one loop register; a new loop command replaces it.
         * Retail script 6 intentionally does this before its next loop end. */
        else if(op==4){if(!value)return 0;loop=1;}
        else if(op==5){if(!loop)return 0;loop=0;}
        else return 0;
    }
    return 0;
}
MeleeRumbleBank* melee_rumble_bank_create(const MeleeArchive* archive)
{
    uint32_t root;
    if(!archive||!melee_archive_find(archive,"lbRumbleData",&root)||!root||(root&1)||
       root>=archive->data_size||(archive->data_size-root)%8)return NULL;
    size_t count=(archive->data_size-root)/8;
    if(!count||count>256||root>65536)return NULL;
    MeleeRumbleBank* bank=calloc(1,sizeof(*bank));if(!bank)return NULL;
    bank->count=count;bank->words=malloc(root);bank->entries=calloc(count,sizeof(*bank->entries));
    if(!bank->words||!bank->entries)goto fail;
    const uint8_t* data=archive->bytes+32;
    for(size_t i=0;i<root/2;i++)bank->words[i]=(uint16_t)data[2*i]<<8|data[2*i+1];
    for(size_t i=0;i<count;i++){
        uint32_t target;MeleeHostBool present;
        if(!melee_archive_pointer(archive,root+8*i,&target,&present)||!present||
           (target&1)||target>=root||!valid_script(bank->words,target/2,root/2))goto fail;
        bank->entries[i].unk=bank->words+target/2;
        bank->entries[i].unk4=data[root+8*i+4];
        bank->entries[i].unk5=data[root+8*i+5];
    }
    return bank;
fail:
    melee_rumble_bank_free(bank);return NULL;
}
