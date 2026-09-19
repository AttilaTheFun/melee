#ifndef MELEE_NATIVE_ACTION_H
#define MELEE_NATIVE_ACTION_H
#include "melee_archive.h"
typedef struct MeleeAction MeleeAction;
typedef struct { uint32_t offset,opcode,count,words[7]; } MeleeActionEvent;
typedef MeleeHostBool (*MeleeActionCallback)(void*,const MeleeActionEvent*);
typedef enum { MELEE_ACTION_WAIT,MELEE_ACTION_DONE,MELEE_ACTION_UNHANDLED,
    MELEE_ACTION_INVALID_STACK,MELEE_ACTION_BUDGET } MeleeActionResult;
/* Owns reachable command words and relocated branch targets. Base commands
 * execute using original lbcommand routines; other events require a callback.
 * No fighter-side event is silently skipped by the production API. */
MeleeAction* melee_action_decode(const MeleeArchive*,uint32_t script);
/* Shared owned arena for all supplied fighter script roots. Payloads and
 * branch pointers are native; consumers still must validate runtime indexes
 * and bound execution. Script pointers remain valid until owner destruction. */
union CmdUnion;
MeleeAction* melee_fighter_actions_decode(const MeleeArchive*,const uint32_t* roots,size_t count);
union CmdUnion* melee_fighter_actions_script(MeleeAction*,uint32_t offset);
void melee_action_free(MeleeAction*);
void melee_action_reset(MeleeAction*);
/* First call can use frame=0, delta=0 to dispatch immediate events. Thereafter
 * pass the animation frame and its elapsed delta. Frame wrap releases command
 * 08's animation timer. A fault persists until reset. */
MeleeActionResult melee_action_step(MeleeAction*,float frame,float delta,
    size_t command_budget,MeleeActionCallback,void* context);
#endif
