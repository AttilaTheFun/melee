/* Call the real Arwing animations through the item predicate ABI. */
#include "../../src/melee/it/kinds/itarwinglaser.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static bool animation_complete, expired;
static unsigned lifetime_calls;
bool it_80272C6C(Item_GObj* object) { (void)object; return animation_complete; }
bool it_80273130(Item_GObj* object) { (void)object; ++lifetime_calls; return expired; }
int main(void)
{
    Item item;
    HSD_GObj object = {0};
    object.user_data = &item;
    HSD_GObjPredicate volatile callbacks[] = {
        (HSD_GObjPredicate)itArwinglaser_UnkMotion2_Anim,
        (HSD_GObjPredicate)itArwinglaser_UnkMotion3_Anim,
    };
    for (unsigned motion=0; motion<2; ++motion) {
        for (unsigned complete=0; complete<2; ++complete) {
            for (unsigned dead=0; dead<2; ++dead) {
                memset(&item,0,sizeof(item));
                item.xDD4_itemVar.arwinglaser.xE18=(Vec3){1,2,3};
                animation_complete=complete;expired=dead;lifetime_calls=0;
                assert(callbacks[motion](&object)==expired);
                assert(lifetime_calls==1);
                assert(item.xDD4_itemVar.arwinglaser.xE24.x==1);
                assert(item.xDD4_itemVar.arwinglaser.xE24.y==2);
                assert(item.xDD4_itemVar.arwinglaser.xE24.z==3);
                assert(item.xDD4_itemVar.arwinglaser.xE30==(motion==1||!complete));
            }
        }
    }
    puts("PASS Arwing item animation predicates preserve state and expiration result");
}
