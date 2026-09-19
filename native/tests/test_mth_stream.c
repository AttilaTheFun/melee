#include "../../src/melee/lb/lbmthp.c"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#undef __assert
static u8* movie;
static size_t movie_size;
static HSD_DevComCallback pending;
static unsigned requests;
void __assert(char* file,u32 line,char* expression)
{fprintf(stderr,"%s:%u %s\n",file,line,expression);abort();}
void OSReport(char* format,...){(void)format;abort();}
u32 OSGetTick(void){return 1;}
s32 DVDConvertPathToEntrynum(const char* path){(void)path;return 1;}
size_t lbFile_8001634C(int file){assert(file==1);return movie_size;}
void lbFile_800161C4(int file,uintptr_t offset,uintptr_t output,size_t size,int type,int pri)
{
    assert(file==1&&type==0x21&&pri==1&&offset<=movie_size&&size<=movie_size-offset);
    assert(output>UINT32_MAX&&!(output&31));memcpy((void*)output,movie+offset,size);
}
int HSD_DevComRequest(int file,uintptr_t offset,uintptr_t output,size_t size,
    int type,int pri,HSD_DevComCallback callback,void* arg)
{
    assert(!pending&&!arg);lbFile_800161C4(file,offset,output,size,type,pri);
    pending=callback;++requests;return 1;
}
int main(int argc,char** argv)
{
    assert(argc==2||argc==3);FILE* f=fopen(argv[1],"rb");assert(f);
    assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>64);rewind(f);
    movie_size=n;movie=malloc(movie_size);assert(movie&&fread(movie,1,movie_size,f)==movie_size);fclose(f);
    THPDecComp* p=&MoviePlayer;
    assert(fn_8001EB14(p,"movie"));size_t required=fn_8001EBF0(p);
    p->unk_68=argc==3;
    void* allocation=NULL;assert(!posix_memalign(&allocation,32,required));
    memset(allocation,0xcd,required);fn_8001ECF4(p,allocation);
    assert((u8*)p->unk_58+(size_t)p->width*p->height/4<=(u8*)allocation+required);
    for(unsigned i=0;i<32;i++)assert(p->frame_buffers[i]>UINT32_MAX);
    unsigned frames=p->unk_40+(p->unk_68?40:0);
    for(unsigned i=0;i<frames;i++){
        if(i)fn_8001F06C(p);
        lbMthp_8001F578();assert(p->unk_7C==i%p->unk_40&&p->unk_90==i%32);
        fn_8001EF5C(p);assert(p->unk_94==(s32)p->unk_90);
        unsigned completions=0;
        while(pending){HSD_DevComCallback cb=pending;pending=NULL;
            cb(0,(HSD_DevComArg){0},NULL,0);assert(++completions<33);}
        assert(!p->unk_110);
    }
    if(!p->unk_68){unsigned last=p->unk_78;fn_8001F06C(p);assert(p->unk_78==last);}
    printf("Original movie stream: %u frames decoded, %u deferred refill callbacks, full-width slots passed\n",frames,requests);
    free(allocation);free(movie);
}
