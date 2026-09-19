// Compatibility functions missing from the pinned Aurora SDK. Use its actual
// object representation rather than duplicating native pointer offsets.
#include "lib/gfx/texture.hpp"
extern "C" {
GXTexFilter GXGetTexObjMinFilt(const GXTexObj* obj) { return reinterpret_cast<const GXTexObj_*>(obj)->min_filter(); }
GXTexFilter GXGetTexObjMagFilt(const GXTexObj* obj) { return reinterpret_cast<const GXTexObj_*>(obj)->mag_filter(); }
float GXGetTexObjMinLOD(const GXTexObj* obj) { return reinterpret_cast<const GXTexObj_*>(obj)->min_lod(); }
float GXGetTexObjMaxLOD(const GXTexObj* obj) { return reinterpret_cast<const GXTexObj_*>(obj)->max_lod(); }
float GXGetTexObjLODBias(const GXTexObj* obj) { return reinterpret_cast<const GXTexObj_*>(obj)->lod_bias(); }
GXBool GXGetTexObjBiasClamp(const GXTexObj* obj) { return reinterpret_cast<const GXTexObj_*>(obj)->bias_clamp(); }
GXBool GXGetTexObjEdgeLOD(const GXTexObj* obj) { return reinterpret_cast<const GXTexObj_*>(obj)->do_edge_lod(); }
GXAnisotropy GXGetTexObjMaxAniso(const GXTexObj* obj) { return reinterpret_cast<const GXTexObj_*>(obj)->max_aniso(); }
void* GXGetTlutObjData(const GXTlutObj* obj) { return const_cast<void*>(reinterpret_cast<const GXTlutObj_*>(obj)->data); }
GXTlutFmt GXGetTlutObjFmt(const GXTlutObj* obj) { return reinterpret_cast<const GXTlutObj_*>(obj)->format; }
u16 GXGetTlutObjNumEntries(const GXTlutObj* obj) { return reinterpret_cast<const GXTlutObj_*>(obj)->numEntries; }
void GXGetTlutObjAll(const GXTlutObj* obj, void** data, GXTlutFmt* format, u16* count) {
    *data = GXGetTlutObjData(obj); *format = GXGetTlutObjFmt(obj); *count = GXGetTlutObjNumEntries(obj);
}
void GXGetTexObjLODAll(const GXTexObj* obj, GXTexFilter* min, GXTexFilter* mag,
                     float* minLOD, float* maxLOD, float* bias, u8* clamp, u8* edge, GXAnisotropy* aniso) {
    *min = GXGetTexObjMinFilt(obj); *mag = GXGetTexObjMagFilt(obj);
    *minLOD = GXGetTexObjMinLOD(obj); *maxLOD = GXGetTexObjMaxLOD(obj); *bias = GXGetTexObjLODBias(obj);
    *clamp = GXGetTexObjBiasClamp(obj); *edge = GXGetTexObjEdgeLOD(obj); *aniso = GXGetTexObjMaxAniso(obj);
}
}
