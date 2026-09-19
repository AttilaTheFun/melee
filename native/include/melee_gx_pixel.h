#ifndef MELEE_NATIVE_GX_PIXEL_H
#define MELEE_NATIVE_GX_PIXEL_H
#include "melee_host_types.h"
#include <stdint.h>
typedef struct {
    uint32_t color_update,alpha_update,dst_alpha_enable,dst_alpha;
    uint32_t blend_type,src_factor,dst_factor,logic_op;
    uint32_t z_enable,z_compare,z_update,before_texture;
    uint32_t alpha_compare0,alpha_ref0,alpha_operation,alpha_compare1,alpha_ref1,dither;
} MeleeGXPixelState;
/* Captures the native GX pixel registers. Original HSD rendering calls must be
 * serialized by the game thread (or the native interrupt gate). */
void melee_gx_pixel_snapshot(MeleeGXPixelState* output);
/* Runs the ORIGINAL HSD_SetupPEMode into the native GX register backend.
 * A null descriptor selects HSD defaults. Non-null descriptors are 12 bytes.
 * Serializes the original HSD state cache and forces all register updates. */
MeleeHostBool melee_gx_pixel_material(uint32_t mode,const uint8_t* descriptor,
                                    MeleeGXPixelState* output);
#endif
