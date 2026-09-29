#include "melee_input.h"
#include "melee_pad_backend.h"
#include <sysdolphin/baselib/controller.h>
#include <dolphin/os.h>
extern void melee_browser_poll_input(float* samples);
void melee_browser_update_input(void);
void melee_browser_publish_input(const float* samples) {
    PADStatus pads[4] = {0};
    for(unsigned i=0;i<2;i++) {
        const float* p=samples+i*8;
        pads[i]=melee_analog_pad((uint16_t)p[0],p[1],p[2],p[3],p[4],p[5],p[6]);
        pads[i].err=p[7]?PAD_ERR_NONE:PAD_ERR_NO_CONTROLLER;
    }
    pads[2].err=pads[3].err=PAD_ERR_NO_CONTROLLER;
    melee_pad_publish(pads,0);
}

void melee_browser_resume_input(void) {
    melee_browser_update_input();
    int enabled=OSDisableInterrupts();
    HSD_PadFlushQueue(HSD_PAD_FLUSH_QUEUE_THROWAWAY);
    HSD_PadRenewRawStatus(false);
    OSRestoreInterrupts(enabled);
}

void melee_browser_update_input(void) {
    float samples[16]={0};
    melee_browser_poll_input(samples);
    melee_browser_publish_input(samples);
}
