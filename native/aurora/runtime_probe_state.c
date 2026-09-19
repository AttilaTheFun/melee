#include <melee/gm/gmmain_lib.h>
#include <melee/gm/gm_1601.h>
/* Invoked by the guarded, paused-runtime diagnostic wrapper. */
int melee_native_probe_rumble(int setting)
{
    if (setting == 0 || setting == 1) gmMainLib_SetRumbleEnabled(0, setting != 0);
    return GetRumbleSettingOfPort(0);
}

int melee_native_probe_unlock(int setting)
{
    unsigned bit=gm_CKindToUnlockIndex(CKind_Luigi);
    if(bit>=16)return -1;
    if(setting==1)*gmMainLib_GetUnlockedCharactersBitmaskPtr()|=(u16)(1u<<bit);
    return gm_IsCKindUnlocked(CKind_Luigi);
}
