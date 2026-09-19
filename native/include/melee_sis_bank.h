#ifndef MELEE_NATIVE_SIS_BANK_H
#define MELEE_NATIVE_SIS_BANK_H
#include "melee_archive.h"
typedef struct MeleeSisBank MeleeSisBank;
/* Convert a flat SIS relocation table, retaining packed glyph/command bytes.
 * Includes the first two font-data entries. A one-past-end target is preserved
 * as such and must not be dereferenced. No input archive pointers survive. */
MeleeSisBank* melee_sis_bank_decode(const MeleeArchive* archive,const char* symbol);
/* Embedded table with a caller-supplied schema count. */
MeleeSisBank* melee_sis_bank_decode_table(const MeleeArchive*,const char* symbol,size_t count);
void** melee_sis_bank_table(MeleeSisBank* bank);
size_t melee_sis_bank_count(const MeleeSisBank* bank);
void melee_sis_bank_free(MeleeSisBank* bank);
#endif
