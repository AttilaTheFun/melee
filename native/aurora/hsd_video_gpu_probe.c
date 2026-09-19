/* Original HSD video ownership exercised with the real GX/VI backends. */
#include <sysdolphin/baselib/video.h>
#include <dolphin/vi.h>
#include <dolphin/os.h>
#include "melee_vi.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

static char* buffers;
static const size_t frame_bytes=640*480*2;

void melee_hsd_video_gpu_init(void)
{
    assert(!buffers);
    assert(!posix_memalign((void**)&buffers,32,frame_bytes*2));
    HSD_VIStatus status={0};
    status.rmode=GXNtsc480Prog;
    status.clear_clr=(GXColor){13,18,26,255};
    status.clear_z=GX_MAX_Z24;
    status.update_clr=status.update_alpha=status.update_z=1;
    HSD_VIInit(&status,buffers,buffers+frame_bytes,NULL);
    HSD_VISetUserGXDrawDoneCallback(HSD_VIDrawDoneXFB);
}

void* melee_hsd_video_gpu_copy(void)
{
    BOOL enabled=OSDisableInterrupts();
    assert(buffers && HSD_VIData.xfb[0].status==HSD_VI_XFB_FREE);
    OSRestoreInterrupts(enabled);
    HSD_VICopyXFBAsync(HSD_RP_SCREEN);
    enabled=OSDisableInterrupts();
    assert(HSD_VIData.drawdone.waiting);
    assert(HSD_VIData.xfb[0].status==HSD_VI_XFB_WAITDONE);
    OSRestoreInterrupts(enabled);
    return buffers;
}
void melee_hsd_video_gpu_before_submit(void)
{
    BOOL enabled=OSDisableInterrupts();
    assert(HSD_VIData.drawdone.waiting);
    assert(HSD_VIData.xfb[0].status==HSD_VI_XFB_WAITDONE);
    OSRestoreInterrupts(enabled);
}
void melee_hsd_video_gpu_wait(void)
{
    BOOL enabled=OSDisableInterrupts();
    GXWaitDrawDone();
    assert(!HSD_VIData.drawdone.waiting);
    unsigned waits=0;
    while(HSD_VIData.xfb[0].status!=HSD_VI_XFB_DISPLAY && waits++<120)
        VIWaitForRetrace();
    MeleeVIPresentation presentation=melee_vi_presentation();
    assert(HSD_VIData.xfb[0].status==HSD_VI_XFB_DISPLAY);
    assert(presentation.framebuffer==buffers && !presentation.black);
    assert(HSD_VIData.xfb[1].status==HSD_VI_XFB_FREE);
    OSRestoreInterrupts(enabled);
    puts("Original HSD video: GPU copy, draw completion and native retrace reached DISPLAY");
}
void melee_hsd_video_gpu_shutdown(void)
{
    BOOL enabled=OSDisableInterrupts();
    VISetPreRetraceCallback(NULL);VISetPostRetraceCallback(NULL);
    GXSetDrawDoneCallback(NULL);
    OSRestoreInterrupts(enabled);
    melee_vi_shutdown();
    free(buffers);buffers=NULL;
}
