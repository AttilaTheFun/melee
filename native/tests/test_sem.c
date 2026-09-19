#include "melee_sem.h"
#include "melee_dvd.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#undef __assert
#include "../../src/sysdolphin/baselib/axdriver.c"

static void store(unsigned char* bytes,unsigned index,uint32_t value)
{
    for(unsigned j=0;j<4;++j) bytes[index*4+j]=(unsigned char)(value>>(24-j*8));
}
int main(int argc,char** argv)
{
    uint32_t words[]={1,123,1,44,1,0,2,44,48,1,52,0x01001234,0x0F000000,0x0E000000};
    unsigned char unaligned[57]; unsigned char* bytes=unaligned+1;
    for(unsigned i=0;i<14;++i) store(bytes,i,words[i]);
    MeleeSEM* sem=melee_sem_open(bytes,56); assert(sem);
    assert(sem->word_count==14 && sem->values[0][0]==123);
    assert(sem->references[1][0]==sem->references[3][0]);
    assert((uintptr_t)sem->references[3][0]>UINT32_MAX);
    assert(*sem->references[3][0]==0x01001234 && *sem->references[3][1]==0x0F000000);
    assert(*sem->references[4][0]==0x0E000000);
    assert(!memcmp(sem->words,words,sizeof(words)));
    uint32_t* target=NULL;
    assert(melee_sem_command_target(sem,sem->words+13,2,&target));
    assert(target==sem->words+11);
    const uint32_t* invalid[]={sem->words,sem->words+10,sem->words+14,
        (const uint32_t*)((uintptr_t)sem->words+45),(const uint32_t*)1};
    for(unsigned i=0;i<5;++i) {
        assert(!melee_sem_command_target(sem,invalid[i],0,&target));
        assert(target==sem->words+11);
    }
    assert(!melee_sem_command_target(sem,sem->words+13,3,&target));
    assert(!melee_sem_command_target(sem,sem->words+13,SIZE_MAX,&target));
    melee_sem_close(sem);
    for(unsigned length=0;length<56;++length) assert(!melee_sem_open(bytes,length));
    for(unsigned variant=0;variant<5;++variant) {
        unsigned char bad[56]; memcpy(bad,bytes,56);
        if(variant==0) store(bad,0,UINT32_MAX);
        if(variant==1) store(bad,7,45);
        if(variant==2) store(bad,7,56);
        if(variant==3) store(bad,7,4);
        if(variant==4) store(bad,5,3);
        assert(!melee_sem_open(bad,56));
    }
    assert(!melee_sem_open(NULL,56));
    if(argc==2) {
        assert(melee_dvd_mount(argv[1]));
        for(unsigned pass=0;pass<2;++pass) {
            AXDriver_8038DA70("audio/us/smash2.sem",NULL);
            assert(native_sem && AXDriver_804D77B0==55 && AXDriver_804D77B8==4035);
            assert(AXDriver_804D77BC==native_sem->references[3]);
            unsigned loops=0;
            for(size_t i=native_sem->payload_word;i<native_sem->word_count;++i) {
                uint32_t* checked=NULL;
                assert(melee_sem_command_target(native_sem,native_sem->words+i,0,&checked));
                if((*checked>>24)==3) {
                    uint32_t backwards=*checked&0xFFFFFF;
                    assert(melee_sem_command_target(native_sem,backwards?checked:checked+1,
                                                     backwards?backwards-1:0,&checked));
                    ++loops;
                }
            }
            assert(loops==371);
            for(unsigned i=0;i<4035;++i) {
                assert((uintptr_t)AXDriver_804D77BC[i]>UINT32_MAX);
                assert(AXDriver_804D77BC[i]>=native_sem->words &&
                       AXDriver_804D77BC[i]<native_sem->words+native_sem->word_count);
            }
            MeleeSEM* saved=native_sem;
            AXDriver_8038DA70("does-not-exist.sem",NULL); assert(native_sem==saved);
            HSD_SM active={.flags=SMSTATE_ACTIVE}; AXDriver_804D7794=&active;
            AXDriver_8038DCFC(); assert(native_sem==saved);
            AXDriver_8038DA70("audio/us/smash2.sem",NULL); assert(native_sem==saved);
            active.flags=0; AXDriver_8038DCFC(); AXDriver_804D7794=NULL;
            assert(!native_sem && !AXDriver_804D77BC && !AXDriver_804D77B8);
        }
        assert(melee_dvd_unmount());
        puts("Retail SEM: real DVD loader, 55 banks, 4035 full-width command pointers, reload/failure/active-voice guards passed");
    }
    puts("SEM decoding: host words, native relocation, malformed counts/offsets and truncation passed");
}
