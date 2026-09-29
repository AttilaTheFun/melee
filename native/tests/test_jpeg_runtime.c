#include <sysdolphin/baselib/hsd_3A94.h>
#include <sysdolphin/baselib/hsd_3B33.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
u8* hsd_804D79A0;
u8* hsd_804D79A4;
s32 hsd_804D79A8;
static void expect_block_failure(void* src,size_t n)
{
    if(HSD_SETJMP(&hsd_804D2648)==0){hsd_803B3398(src,n);abort();}
}
int main(void)
{
    HSD_JumpBuffer local;
    switch(HSD_SETJMP(&local)){case 0:HSD_LONGJMP(&local,9);case 9:break;default:abort();}
    memset(hsd_native_jpeg_work.bytes,0xa5,sizeof(hsd_native_jpeg_work.bytes));
    u8* buffer=malloc(16);assert(buffer);
#if UINTPTR_MAX > UINT32_MAX
    assert((uintptr_t)buffer>UINT32_MAX);
#endif
    memset(buffer,0xcc,16);
    hsd_804D79A4=hsd_804D79A0=buffer;hsd_804D79A8=8;
    u8 input[8]={1,2,3,4,5,6,7,8};
    expect_block_failure(input,8);assert(hsd_804D79A0==buffer && buffer[0]==0xcc);
    hsd_803B3398(input,7);assert(hsd_804D79A0==buffer+7&&!memcmp(buffer,input,7));
    hsd_803B3344(8);assert(hsd_804D79A0==buffer+8&&!memcmp(buffer,input,8));
    if(HSD_SETJMP(&hsd_804D2648)==0){hsd_803B3344(9);abort();}
    assert(hsd_804D79A0==buffer+8);
    for(unsigned i=8;i<16;i++)assert(buffer[i]==0xcc);
    hsd_804D79A0=buffer;expect_block_failure(input,SIZE_MAX);expect_block_failure(NULL,1);
    hsd_803B3398(NULL,0);assert(hsd_804D79A0==buffer);
    hsd_804D79A8=-1;expect_block_failure(input,1);
    hsd_804D79A8=8;hsd_804D79A0=(u8*)((uintptr_t)buffer-1);expect_block_failure(input,1);
    for(size_t i=sizeof(HSD_JumpBuffer);i<sizeof(hsd_native_jpeg_work.bytes);i++)assert(hsd_native_jpeg_work.bytes[i]==0xa5);
    free(buffer);puts("Host JPEG jumps and full-width bounded output writes passed.");
}
