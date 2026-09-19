#ifndef MELEE_NATIVE_ITEM_SCRIPTS_H
#define MELEE_NATIVE_ITEM_SCRIPTS_H
#include "melee_archive.h"
#include <melee/lb/types.h>
typedef struct MeleeItemScripts MeleeItemScripts;
typedef struct MeleeItemScript MeleeItemScript;
/* Owns reachable commands and relocated branch targets. Execution must enforce
 * the original command stack capacity and a per-frame command budget. */
MeleeItemScript* melee_item_script_decode(const MeleeArchive*,uint32_t root);
union CmdUnion* melee_item_script_root(MeleeItemScript*);
void melee_item_script_free(MeleeItemScript*);
/* ALDYakuAll table with reserved null slot zero. Accepts item opcodes 0..25. */
MeleeItemScripts* melee_item_scripts_decode(const MeleeArchive*);
union CmdUnion** melee_item_scripts_table(MeleeItemScripts*);
unsigned melee_item_scripts_count(const MeleeItemScripts*);
void melee_item_scripts_free(MeleeItemScripts*);
#endif
