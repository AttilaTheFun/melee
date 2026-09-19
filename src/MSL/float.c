#ifdef MELEE_NATIVE
/* Native callers declare float arrays. Preserve the original payload bits
 * without accessing an int object through an incompatible float lvalue. */
float MSL_TrigF_80400770[] = { __builtin_nanf("0x7fffff") };
float MSL_TrigF_80400774[] = { __builtin_inff() };
#else
int MSL_TrigF_80400770[] = { 0x7FFFFFFF };
int MSL_TrigF_80400774[] = { 0x7F800000 };

#endif
