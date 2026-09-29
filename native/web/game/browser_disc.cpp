#include <emscripten.h>
#include <atomic>
#include <chrono>
#include <thread>
extern "C" {
#include "melee_dvd.h"
MeleeHostBool melee_browser_init_fonts(const MeleeDisc*);
}
// Only the game/DVD workers wait. Browser main remains free to resolve Blob I/O.
static MeleeHostBool read_file(void*, void* output, size_t length, uint64_t offset) {
    std::atomic<int> status{0};
    MAIN_THREAD_EM_ASM({
        const destination=$0; const length=$1; const offset=$2; const status=$3;
        const finish=value=>{Atomics.store(HEAP32,status>>2,value);Atomics.notify(HEAP32,status>>2);};
        const file=Module.discFile;
        if(!file || offset<0 || offset+length>file.size){finish(-1);return;}
        file.slice(offset,offset+length).arrayBuffer().then(bytes=>{
            if(bytes.byteLength!==length){finish(-1);return;}
            // Memory may have grown while the read was outstanding.
            HEAPU8.set(new Uint8Array(bytes),destination);finish(1);
        },()=>finish(-1));
    }, output, length, static_cast<double>(offset), &status);
    // Do not abandon a pending request: its callback still owns these addresses.
    while(!status.load(std::memory_order_acquire))
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return status.load()==1;
}
extern "C" bool melee_browser_mount_disc() {
    double size=MAIN_THREAD_EM_ASM_DOUBLE({return Module.discFile ? Module.discFile.size : 0;});
    if(size<=0)return false;
    MeleeDisc* disc=melee_disc_open_reader(static_cast<uint64_t>(size),read_file,nullptr);
    if(!disc)return false;
    const bool fonts=melee_browser_init_fonts(disc);
    melee_disc_close(disc);
    return fonts && melee_dvd_mount_reader(static_cast<uint64_t>(size),read_file,nullptr);
}
