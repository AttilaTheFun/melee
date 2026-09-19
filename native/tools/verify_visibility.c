#include "melee_visibility.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char** argv)
{
    if(argc!=4)return 2;
    char* end;unsigned long count=strtoul(argv[3],&end,10);if(*end || count>124)return 2;
    FILE* f=fopen(argv[1],"rb");if(!f)return 1;
    if(fseek(f,0,SEEK_END)){fclose(f);return 1;}long size=ftell(f);rewind(f);
    uint8_t* bytes=size>0?malloc((size_t)size):NULL;
    if(!bytes || fread(bytes,1,(size_t)size,f)!=(size_t)size){free(bytes);fclose(f);return 1;}fclose(f);
    MeleeArchive a;uint32_t root,desc;MeleeHostBool present;
    if(!melee_archive_open(&a,bytes,(size_t)size) || !melee_archive_find(&a,argv[2],&root) || !melee_archive_pointer(&a,root+8,&desc,&present) || !present){free(bytes);return 1;}
    MeleeVisibility* v=melee_visibility_decode(&a,desc,0,0,count);free(bytes);if(!v)return 1;
    size_t groups=melee_visibility_group_count(v),hidden=0,transitions=0;int choices[11];MeleeHostBool initial[124];
    for(size_t i=0;i<count;i++){initial[i]=melee_visibility_hidden(v,i);hidden+=initial[i];}
    printf("%s: %zu groups, %zu/%lu initially hidden drawables; choices",argv[2],groups,hidden,count);
    for(size_t i=0;i<groups;i++){choices[i]=-1;printf(" %zu",melee_visibility_choice_count(v,i));}puts("");
    for(size_t i=0;i<groups;i++)for(size_t j=0;j<melee_visibility_choice_count(v,i);j++){
        choices[i]=(int)j;if(!melee_visibility_select(v,choices,groups)){melee_visibility_free(v);return 1;}transitions++;choices[i]=-1;
    }
    if(!melee_visibility_select(v,choices,groups)){melee_visibility_free(v);return 1;}
    for(size_t i=0;i<count;i++)if(initial[i]!=melee_visibility_hidden(v,i)){melee_visibility_free(v);return 1;}
    melee_visibility_free(v);
    printf("%zu original selector transitions and return to hidden state passed\n",transitions);
}
