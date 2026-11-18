#include "melee_archive.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void put(unsigned char* p, uint32_t x) {
    p[0]=x>>24;p[1]=x>>16;p[2]=x>>8;p[3]=x;
}
static double now(void) { struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9; }
static void compare(const MeleeArchive* archive) {
    MeleeArchive linear=*archive;linear.reloc_sorted=false;
    for(uint32_t s=0;s<=archive->data_size+1;s++) {
        uint32_t a=0,b=0;MeleeHostBool pa=false,pb=false;
        int ra=melee_archive_pointer(archive,s,&a,&pa), rb=melee_archive_pointer(&linear,s,&b,&pb);
        assert(ra==rb&&a==b&&pa==pb);
    }
}
int main(int argc,char** argv) {
    /* Sorted, unsorted, zero-target, one-past, null and nonrelocated words. */
    unsigned char bytes[76]={0};put(bytes,76);put(bytes+4,32);put(bytes+8,3);
    put(bytes+32,0);put(bytes+36,32);put(bytes+44,9);
    put(bytes+64,0);put(bytes+68,4);put(bytes+72,12);
    MeleeArchive a;assert(melee_archive_open(&a,bytes,sizeof(bytes))&&a.reloc_sorted);compare(&a);
    put(bytes+64,12);put(bytes+72,0);
    assert(melee_archive_open(&a,bytes,sizeof(bytes))&&!a.reloc_sorted);compare(&a);
    for(int f=1;f<argc;f++) {
        FILE* file=fopen(argv[f],"rb");assert(file);fseek(file,0,SEEK_END);size_t n=ftell(file);rewind(file);
        unsigned char* data=malloc(n);assert(data&&fread(data,1,n,file)==n);fclose(file);
        assert(melee_archive_open(&a,data,n));MeleeArchive linear=a;linear.reloc_sorted=false;
        uint32_t random=42;double elapsed[2];uint64_t sums[2]={0};
        for(int pass=0;pass<2;pass++) {
            random=42;double start=now();
            for(int i=0;i<100000;i++) {
                random=random*1664525+1013904223;
                uint32_t slot=(random%(a.data_size/4))*4,target=0;MeleeHostBool present=false;
                int ok=melee_archive_pointer(pass?&a:&linear,slot,&target,&present);
                sums[pass]+=((uint64_t)target<<2)|(present<<1)|ok;
            }
            elapsed[pass]=now()-start;
        }
        assert(sums[0]==sums[1]);
        printf("%s: relocations=%u sorted=%d linear=%.3fms optimized=%.3fms speedup=%.1fx checksum=%llu\n",argv[f],a.reloc_count,a.reloc_sorted,elapsed[0]*1000,elapsed[1]*1000,elapsed[0]/elapsed[1],(unsigned long long)sums[0]);
        free(data);
    }
    puts("Archive lookup equivalence passed");return 0;
}
