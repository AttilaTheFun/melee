#include <dolphin/types.h>
#include <dolphin/mcc.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sysdolphin/baselib/hsd_392C.h>

static int found(long channel){(void)channel;assert(!"Unexpected development device");return 1;}
static void event(enum MCC_SYSEVENT event){(void)event;assert(!"Unexpected development event");}
int main(void){
    assert(MCCEnumDevices(found)==1&&MCCGetLastError()==0);
    assert(MCCEnumDevices(NULL)==0&&MCCGetLastError()==13);
    for(int i=-1;i<4;i++)assert(!MCCInit((enum MCC_EXI)i,0,event)&&MCCGetLastError()==4);
    enum MCC_CONNECT connection=MCC_CONNECT_CONNECTED;
    assert(!MCCGetConnectionStatus(MCC_CHANNEL_1,&connection)&&connection==MCC_CONNECT_CONNECTED);
    assert(MCCGetLastError()==1&&!MCCGetFreeBlocks(MCC_MODE_ALL));
    unsigned char data[32],before[32];memset(data,0xa7,sizeof(data));memcpy(before,data,sizeof(data));
    assert(!MCCRead(MCC_CHANNEL_1,0,data,sizeof(data),MCC_SYNC));
    assert(!MCCWrite(MCC_CHANNEL_1,0,data,sizeof(data),MCC_ASYNC));
    assert(!memcmp(data,before,sizeof(data)));
    assert(!MCCOpen(MCC_CHANNEL_1,1,NULL)&&!MCCStreamOpen(MCC_CHANNEL_1,1));
    assert(!MCCNotify(MCC_CHANNEL_1,42)&&!MCCClose(MCC_CHANNEL_1));
    MCCExit();assert(MCCGetLastError()==1);
    assert(!FIOInit(MCC_EXI_0,MCC_CHANNEL_1,1)&&FIOGetLastError()==0x87);
    assert(!FIOQuery()&&FIOGetLastError()==0x87);
    assert(FIOFopen("native-debug-output",0)==-1&&FIOGetLastError()==0x83);
    assert(FIOFopen(NULL,0)==-1&&FIOGetLastError()==0xb0);
    assert(!FIOFclose(1)&&FIOGetLastError()==0x83);
    assert(FIOFwrite(1,data,sizeof(data))==UINT32_MAX&&FIOGetLastError()==0x83);
    assert(FIOFwrite(1,data,0)==0&&FIOGetLastError()==0x83);
    assert(FIOFwrite(0,data,sizeof(data))==UINT32_MAX&&FIOGetLastError()==0xb0);
    assert(!memcmp(data,before,sizeof(data)));FIOExit();assert(FIOGetLastError()==0x87);
    assert(!hsd_803931A4(-1));
    puts("Native devkit: zero-device enumeration, failed initialization and original HSD fallback passed");
}
