/* Original HSD handoff with the real native VI clock. GX remains a test double. */
#include "melee_vi.h"
#include "../../src/sysdolphin/baselib/video.c"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

static struct Copy {void* destination; u16 top,width,height; GXFBClamp clamp;} copies[16];
static unsigned copy_count;
static u16 source_top, source_width, source_height;
static GXFBClamp clamp_mode;
static GXDrawDoneCallback done;
static unsigned done_requests;

void GXSetCopyFilter(GXBool aa,const u8 sample[12][2],GXBool vf,const u8 filter[7])
{(void)aa;(void)sample;(void)vf;(void)filter;}
void GXSetDispCopyGamma(GXGamma gamma){(void)gamma;}
void HSD_StateSetColorUpdate(int value){(void)value;}
void HSD_StateSetAlphaUpdate(int value){(void)value;}
void HSD_StateSetZMode(int enable,int comparison,int update){(void)enable;(void)comparison;(void)update;}
void GXSetCopyClear(GXColor color,u32 depth){(void)color;(void)depth;}
void GXSetCopyClamp(GXFBClamp clamp){clamp_mode=clamp;}
void GXSetDispCopySrc(u16 left,u16 top,u16 width,u16 height)
{assert(left==0);source_top=top;source_width=width;source_height=height;}
u32 GXSetDispCopyYScale(f32 scale){return (u32)(source_height*scale);}
void GXSetDispCopyDst(u16 width,u16 height){assert(width==source_width && height);}
void GXCopyDisp(void* destination,GXBool clear)
{
    assert(clear && copy_count<16);
    copies[copy_count++]=(struct Copy){destination,source_top,source_width,source_height,clamp_mode};
}
void GXPixModeSync(void){}
void GXSetDrawDone(void){++done_requests;}
void GXWaitDrawDone(void){assert(done);done();}
GXDrawDoneCallback GXSetDrawDoneCallback(GXDrawDoneCallback callback)
{GXDrawDoneCallback old=done;done=callback;return old;}
void VIInit(void){melee_vi_init();}
u32 VIGetTvFormat(void){return VI_NTSC;}
void VIConfigure(GXRenderModeObj* mode){melee_vi_configure(mode->viTVmode, mode->viYOrigin);}
void VIFlush(void){melee_vi_flush();}
static char* expected_memory;
static size_t expected_size;
static unsigned observed;
static void observe_handoff(u32 count)
{
    (void)count;
    if (observed==0) {
        assert(melee_vi_presentation().framebuffer==expected_memory);
        assert(HSD_VIData.xfb[0].status==HSD_VI_XFB_DISPLAY);
        assert(HSD_VIData.xfb[1].status==HSD_VI_XFB_NEXT);
    } else if (observed==1) {
        assert(melee_vi_presentation().framebuffer==expected_memory+expected_size);
        assert(HSD_VIData.xfb[0].status==HSD_VI_XFB_FREE);
        assert(HSD_VIData.xfb[1].status==HSD_VI_XFB_DISPLAY);
    }
    if (observed<2) ++observed;
}
int main(void)
{
    size_t size=640*480*2;
    char* memory=malloc(size*3); assert(memory);
    HSD_VIStatus status={0};
    status.rmode.fbWidth=640;status.rmode.efbHeight=480;status.rmode.xfbHeight=480;
    BOOL enabled=OSDisableInterrupts();
    HSD_VIInit(&status,memory,memory+size,memory+size*2);
    expected_memory=memory;expected_size=size;
    HSD_VISetUserPostRetraceCallback(observe_handoff);
    HSD_VISetUserGXDrawDoneCallback(HSD_VIDrawDoneXFB);
    int first=HSD_VIGetXFBDrawEnable();assert(first==0);
    HSD_VISetXFBWaitDone(first);HSD_VIGXSetDrawDone(first);done();
    int second=HSD_VIGetXFBDrawEnable();assert(second==1);
    HSD_VISetXFBWaitDone(second);HSD_VIGXSetDrawDone(second);done();
    assert(HSD_VIData.xfb[first].status==HSD_VI_XFB_NEXT);
    assert(HSD_VIData.xfb[second].status==HSD_VI_XFB_DRAWDONE);
    while (observed<2) VIWaitForRetrace();
    VISetPreRetraceCallback(NULL);VISetPostRetraceCallback(NULL);
    OSRestoreInterrupts(enabled);
    melee_vi_shutdown();free(memory);
    puts("Original HSD video: native timed retraces advance two completed framebuffers (GX test double)");
}
