#include <melee/ef/eflib.h>
#include <sysdolphin/baselib/gobj.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    HSD_GObj objects[9] = {0};
    memset(efLib_ParamTable, 0, sizeof(efLib_ParamTable));
    /* The animation queue must remain independent of parameter storage. */
    HSD_JObj* marker = (HSD_JObj*)(void*)&objects[8];
    for (unsigned i = 0; i < 32; ++i) efLib_AnimQueue[i] = marker;
    for (unsigned i = 0; i < 8; ++i) {
        efLib_SetParamAlpha(&objects[i], 20 + i);
        efLib_SetParamGfxId(&objects[i], 0x400 + i);
        assert(efLib_ParamTable[i].gobj == &objects[i]);
        assert(efLib_ParamTable[i].alpha == 20 + i);
        assert(efLib_ParamTable[i].gfx_id == 0x400 + i);
    }
    EF_ParamEntry saved[8]; memcpy(saved, efLib_ParamTable, sizeof(saved));
    efLib_SetParamAlpha(&objects[8], 255);
    assert(!memcmp(saved, efLib_ParamTable, sizeof(saved))); /* full table */
    efLib_SetParamAlpha(&objects[2], 200);
    assert(efLib_ParamTable[2].alpha == 200 && efLib_ParamTable[2].gfx_id == 0x402);
    for (unsigned i = 0; i < 32; ++i) assert(efLib_AnimQueue[i] == marker);
    puts("Original effect parameter setters use independent native pointer storage.");
    return 0;
}
