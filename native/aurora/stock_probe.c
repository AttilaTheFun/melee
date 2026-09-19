/* Include the original module so this diagnostic can inspect its private HUD
 * state without adding a production accessor. Player and HSD calls are real. */
#include "../../src/melee/if/ifstock.c"
#include <math.h>
#include <string.h>
#include <stdint.h>
#undef NDEBUG
#include <assert.h>
void melee_stock_probe(void)
{
    memset(&ifStock_804A1378,0,sizeof(ifStock_804A1378));
    struct ifStock_804A1378* stock=&ifStock_804A1378;
    assert(offsetof(struct ifStock_804A1378,x204)>0x204);
    for(unsigned player=0;player<6;player++){
        Player_SetStocks(player,3);
        stock->x204[player].player=player;
        stock->x204[player].flag=1;
        for(unsigned slot=0;slot<8;slot++){
            HSD_Joint desc={.scale={1,1,1},.position={player*10+slot,player*2+slot,slot}};
            stock->player[player].x4[slot]=HSD_JObjLoadJoint(&desc);
            assert(stock->player[player].x4[slot]);
            assert((uintptr_t)stock->player[player].x4[slot]>UINT32_MAX);
        }
    }
    Player_SetStocks(0,2);
    assert(ifStock_802F7EFC(0,5)==0);
    assert(stock->x204[0].flag==0 && stock->x204[5].anim[5]==1);
    assert(stock->x204[5].x3[0]==0 && stock->x204[5].anim[6]==0);
    struct IfStockStealAnim* path=&stock->x204[5].steal[0];
    assert(path->start.x==103 && path->start.y==23 && path->start.z==3);
    assert(fabsf(path->end.x-5.8f)<0.0001f && path->end.y==23 && path->end.z==1);
    assert(fabsf(path->mid.x-54.4f)<0.0001f && path->mid.y==33 && path->mid.z==2);
    for(unsigned player=1;player<5;player++)assert(stock->x204[player].flag==1 && stock->x204[player].anim[5]==0);
    assert(ifStock_802F7EFC(1,5)==0 && stock->x204[5].anim[6]==1 && stock->x204[5].x3[1]==1);
    assert(ifStock_802F7EFC(2,5)==2); /* Both animation slots occupied. */
    Player_SetStocks(4,0);assert(ifStock_802F7EFC(2,4)==1);
    /* The native update macros must address the same records, including the
     * other-player lookup used when a stock-steal animation completes. */
    struct IfStockUserData user={.player=5};struct IfStockUserData* user_data=&user;
    assert(ifStock_802F8298_data==&stock->x204[5]);
    assert(ifStock_802F8298_data_in(unused,2)==&stock->x204[2]);
    assert(ifStock_802F8298_player_data(3)==&stock->x204[3]);
    for(unsigned player=0;player<6;player++)for(unsigned slot=0;slot<8;slot++){
        assert(stock->player[player].x4[slot]->translate.x==player*10+slot);
        HSD_JObjRemoveAll(stock->player[player].x4[slot]);
    }
    memset(stock,0,sizeof(*stock));
}
