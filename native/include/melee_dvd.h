#ifndef MELEE_NATIVE_DVD_H
#define MELEE_NATIVE_DVD_H
#include "melee_host_types.h"
#include "melee_disc.h"
/* Mount/unmount require no open DVDFileInfo handles or outstanding reads.
 * Async reads run on a serial worker. Callbacks run under the cooperative
 * native interrupt gate, and may submit another request or close the file.
 * Buffers and DVDFileInfo storage must live through completion. */
MeleeHostBool melee_dvd_mount(const char* path);
MeleeHostBool melee_dvd_mount_reader(uint64_t size, MeleeDiscReader reader, void* context);
MeleeHostBool melee_dvd_unmount(void);
#endif
