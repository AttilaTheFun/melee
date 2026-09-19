#include "melee_card_icons.h"
#include <stdlib.h>
#include <string.h>

struct MeleeCardIcons {
    unsigned char* bytes;
    size_t offsets[4], sizes[4];
    unsigned count;
};
void melee_card_icons_free(MeleeCardIcons* icons)
{if(icons){free(icons->bytes);free(icons);}}
const void* melee_card_icons_data(const MeleeCardIcons* icons,unsigned index)
{return icons&&index<icons->count?icons->bytes+icons->offsets[index]:NULL;}
size_t melee_card_icons_size(const MeleeCardIcons* icons,unsigned index)
{return icons&&index<icons->count?icons->sizes[index]:0;}
static MeleeCardIcons* create(const MeleeArchive* archive,const char* symbol,const char* const* names,unsigned count)
{
    uint32_t root,offsets[5];
    if(!archive||!melee_archive_find(archive,symbol,&root)||
       root>archive->data_size||archive->data_size-root!=4*(count+1))return NULL;
    for(unsigned i=0;i<count;i++){
        uint32_t target;MeleeHostBool present;
        if(!melee_archive_find(archive,names[i],&offsets[i])||
           !melee_archive_pointer(archive,root+4*i,&target,&present)||
           !present||target!=offsets[i]||target>=root)return NULL;
        if(i&&offsets[i]<=offsets[i-1])return NULL;
    }
    uint32_t terminal;MeleeHostBool present;
    if(!melee_archive_pointer(archive,root+4*count,&terminal,&present)||present)return NULL;
    offsets[count]=root;
    /* Retail archives store 96x32 RGB5A3 banners, followed by one
     * 32x32 indexed icon and its 256-entry palette. Reject a mismatched schema. */
    for(unsigned i=0;i<count;i++)if(offsets[i+1]-offsets[i]!=(i+1<count?6144u:1536u))return NULL;
    MeleeCardIcons* icons=calloc(1,sizeof(*icons));if(!icons)return NULL;
    icons->count=count;
    size_t total=root-offsets[0];icons->bytes=malloc(total);
    if(!icons->bytes){free(icons);return NULL;}
    memcpy(icons->bytes,archive->bytes+32+offsets[0],total);
    for(unsigned i=0;i<count;i++){
        icons->offsets[i]=offsets[i]-offsets[0];icons->sizes[i]=offsets[i+1]-offsets[i];
    }
    return icons;
}

MeleeCardIcons* melee_card_icons_create(const MeleeArchive* archive)
{
    static const char* names[]={"MemCardBanner_01","MemCardBanner_02","MemCardBanner_03","MemCardIcon_01"};
    return create(archive,"MemCardIconData",names,4);
}
MeleeCardIcons* melee_card_snapshot_icons_create(const MeleeArchive* archive)
{
    static const char* names[]={"MemSnapBanner_01","MemSnapIcon_01"};
    return create(archive,"MemSnapIconData",names,2);
}
