#ifndef MELEE_NATIVE_CHARACTER_MODELS_H
#define MELEE_NATIVE_CHARACTER_MODELS_H
#include "melee_scene.h"
#include <melee/ft/types.h>
typedef struct MeleeCharacterModels MeleeCharacterModels;
/* Requires initialized HSD pools. Owns guard/metal joint descriptors. */
MeleeCharacterModels* melee_character_models_decode(const MeleeArchive*,u32 ft_data);
void melee_character_models_bind(MeleeCharacterModels*,ftData*);
void melee_character_models_free(MeleeCharacterModels*);
#endif
