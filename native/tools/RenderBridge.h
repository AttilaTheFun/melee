#include "melee_model.h"
#include "melee_gx_pixel.h"
MeleeHostBool melee_render_test_init(void);
MeleeModel* melee_render_test_load(const char* costume, const char* symbol,
                                 const char* motion, float frame);
void melee_render_test_release(MeleeModel* model);
MeleeHostBool melee_render_test_fighter(MeleeModel*,const char* data,const char* symbol,float frame);
