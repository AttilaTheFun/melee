#include "melee_mth.h"
#include <string.h>

static uint32_t be32(const unsigned char* p)
{
    return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];
}
int melee_mth_header(const void* bytes, size_t length, size_t file_size,
                     MeleeMTHHeader* output)
{
    if(!bytes || !output || length<64 || file_size<64 || file_size>UINT32_MAX) return 0;
    const unsigned char* b=bytes;
    if(memcmp(b,"MTHP",4))return 0;
    MeleeMTHHeader h={be32(b+8),be32(b+12),be32(b+16),be32(b+20),
        be32(b+24),be32(b+28),be32(b+32),be32(b+36),be32(b+40)};
    if(h.version!=2 || h.frame_offsets || !h.width || !h.height ||
       h.width>4096 || h.height>4096 || (h.width&15) || (h.height&15) ||
       !h.frame_rate || h.frame_rate>60 || !h.frame_count ||
       h.frame_count>file_size/4 || h.buffer_size<32 ||
       h.buffer_size>16*1024*1024 || (h.buffer_size&31) ||
       h.first_frame<64 || (h.first_frame&31) || h.first_frame>file_size ||
       h.first_frame_size<32 || (h.first_frame_size&31) ||
       h.first_frame_size>((h.buffer_size+4+31)&~31u) || h.first_frame_size>file_size-h.first_frame)
        return 0;
    *output=h;
    return 1;
}
