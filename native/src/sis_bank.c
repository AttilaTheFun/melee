#include "melee_sis_bank.h"
#include <stdlib.h>
#include <string.h>
struct MeleeSisBank { unsigned char* bytes; void** table; size_t count; };
void melee_sis_bank_free(MeleeSisBank* b)
{ if(b){free(b->table);free(b->bytes);free(b);} }
void** melee_sis_bank_table(MeleeSisBank* b){return b?b->table:NULL;}
size_t melee_sis_bank_count(const MeleeSisBank* b){return b?b->count:0;}
MeleeSisBank* melee_sis_bank_decode(const MeleeArchive* a,const char* symbol)
{
    uint32_t root;
    if(!a||!symbol||!melee_archive_find(a,symbol,&root)||root||a->extern_count||
       a->reloc_count<2||a->reloc_count>65536||a->data_size<4*a->reloc_count)return NULL;
    MeleeSisBank* b=calloc(1,sizeof(*b));if(!b)return NULL;
    b->count=a->reloc_count;b->bytes=malloc(a->data_size);
    b->table=calloc(b->count,sizeof(*b->table));
    if(!b->bytes||!b->table)goto fail;
    memcpy(b->bytes,a->bytes+32,a->data_size);
    /* Require exactly one relocation per table word. Looking up by slot also
     * accepts relocation entries in a different order without reordering SIS. */
    for(size_t i=0;i<b->count;i++){
        uint32_t target;MeleeHostBool present;
        if(!melee_archive_pointer(a,4*i,&target,&present)||!present||
           target<4*b->count||target>a->data_size)goto fail;
        b->table[i]=b->bytes+target;
    }
    return b;
fail:melee_sis_bank_free(b);return NULL;
}

MeleeSisBank* melee_sis_bank_decode_table(const MeleeArchive* a,const char* symbol,size_t count)
{
    uint32_t root;
    if(!a||!symbol||!count||count>65536||!melee_archive_find(a,symbol,&root)||
       root>a->data_size||4*count>a->data_size-root)return NULL;
    MeleeSisBank* b=calloc(1,sizeof(*b));if(!b)return NULL;
    b->count=count;b->bytes=malloc(a->data_size);b->table=calloc(count,sizeof(*b->table));
    if(!b->bytes||!b->table)goto fail;
    memcpy(b->bytes,a->bytes+32,a->data_size);
    for(size_t i=0;i<count;i++){
        uint32_t target;MeleeHostBool present;
        if(!melee_archive_pointer(a,root+4*i,&target,&present)||!present||
           target<root+4*count||target>a->data_size)goto fail;
        b->table[i]=b->bytes+target;
    }
    return b;
fail:melee_sis_bank_free(b);return NULL;
}
