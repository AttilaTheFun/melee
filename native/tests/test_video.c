/* Original HSD video logic with explicit GX/VI command-capture test doubles.
 * No GPU copy or actual display presentation is performed by this test. */
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
static void* presented;
static VIRetraceCallback pre,post;
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
void VIInit(void){}
u32 VIGetTvFormat(void){return VI_NTSC;}
void VIConfigure(GXRenderModeObj* mode){assert(mode->fbWidth);}
void VISetBlack(BOOL black){(void)black;}
void VIFlush(void){}
void VISetNextFrameBuffer(void* buffer){presented=buffer;}
VIRetraceCallback VISetPreRetraceCallback(VIRetraceCallback cb){VIRetraceCallback old=pre;pre=cb;return old;}
VIRetraceCallback VISetPostRetraceCallback(VIRetraceCallback cb){VIRetraceCallback old=post;post=cb;return old;}
void VIWaitForRetrace(void){assert(pre&&post);pre(0);post(0);}

static void expected_abort(int signal_number) { _exit(signal_number==SIGABRT?86:90); }
static void invalid_copy(int kind, HSD_VIStatus status, void* buffer)
{
    pid_t child=fork();assert(child>=0);
    if(child==0){
        assert(freopen("/dev/null","w",stderr));signal(SIGABRT,expected_abort);
        if(kind==0)status.rmode.fbWidth=65535;
        else status.rmode.efbHeight=HSD_ANTIALIAS_OVERLAP;
        HSD_VICopyEFB2XFBPtr(&status,buffer,HSD_RP_BOTTOMHALF);
        _exit(91);
    }
    int result;assert(waitpid(child,&result,0)==child);
    assert(WIFEXITED(result)&&WEXITSTATUS(result)==86);
}

int main(void)
{
    size_t size=640*480*2;
    unsigned char* memory=malloc(size*3);
    assert(memory && (uintptr_t)memory>UINT32_MAX);
    HSD_VIStatus status={0};
    status.rmode.fbWidth=640;status.rmode.efbHeight=240;status.rmode.xfbHeight=480;
    status.update_clr=status.update_alpha=status.update_z=1;
    HSD_VICopyEFB2XFBPtr(&status,memory,HSD_RP_SCREEN);
    assert(copy_count==1 && copies[0].destination==memory && copies[0].height==240);
    assert(copies[0].clamp==(GX_CLAMP_TOP|GX_CLAMP_BOTTOM));
    copy_count=0;
    HSD_VICopyEFB2XFBPtr(&status,memory,HSD_RP_TOPHALF);
    assert(copy_count==1 && copies[0].destination==memory);
    assert(copies[0].top==0 && copies[0].height==236 && copies[0].clamp==GX_CLAMP_TOP);
    copy_count=0;
    HSD_VICopyEFB2XFBPtr(&status,memory,HSD_RP_BOTTOMHALF);
    assert(copy_count==2 && copies[0].destination==memory+640*236*2);
    assert(copies[0].top==4 && copies[0].height==236 && copies[0].clamp==GX_CLAMP_BOTTOM);
    assert(copies[1].destination==garbage && copies[1].height==4);
    assert(copies[1].clamp==(GX_CLAMP_TOP|GX_CLAMP_BOTTOM));
    copy_count=0;
    status.rmode.fbWidth=623;
    HSD_VICopyEFB2XFBPtr(&status,memory,HSD_RP_BOTTOMHALF);
    assert(copies[0].destination==memory+624*236*2);
    assert(copies[0].width==623);
    copy_count=0;
    status.rmode.fbWidth=640;
    status.rmode.efbHeight=480;
    HSD_VIInit(&status,memory,memory+size,memory+size*2);
    HSD_VISetUserGXDrawDoneCallback(HSD_VIDrawDoneXFB);
    int first=HSD_VIGetXFBDrawEnable();assert(first==0);
    HSD_VISetXFBWaitDone(first);HSD_VIGXSetDrawDone(first);
    assert(HSD_VIData.xfb[first].status==HSD_VI_XFB_WAITDONE && !presented);
    assert(done_requests==1);done();
    assert(HSD_VIData.xfb[first].status==HSD_VI_XFB_NEXT);
    int second=HSD_VIGetXFBDrawEnable();assert(second==1);
    HSD_VISetXFBWaitDone(second);HSD_VIGXSetDrawDone(second);done();
    assert(HSD_VIData.xfb[second].status==HSD_VI_XFB_DRAWDONE);
    pre(1);assert(presented==memory);post(1);
    assert(HSD_VIData.xfb[first].status==HSD_VI_XFB_DISPLAY);
    assert(HSD_VIData.xfb[second].status==HSD_VI_XFB_NEXT);
    pre(2);assert(presented==memory+size);post(2);
    assert(HSD_VIData.xfb[first].status==HSD_VI_XFB_FREE);
    assert(HSD_VIData.xfb[second].status==HSD_VI_XFB_DISPLAY);
    invalid_copy(0,status,memory);invalid_copy(1,status,memory);
    free(memory);
    puts("Original HSD video: full-width split-frame copy destinations and completion/retrace buffer handoff passed (GX/VI test doubles)");
    return 0;
}
