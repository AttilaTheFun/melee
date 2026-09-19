#include <sysdolphin/baselib/psdisp.h>
#include <sysdolphin/baselib/psstructs.h>
#include <assert.h>
#include <stdio.h>

extern HSD_Particle* hsd_804D0908[16];

static u32 kind(unsigned bucket)
{ return ((bucket & 7u) << 25) | (bucket < 8 ? 8u : 0); }

int main(void)
{
    HSD_Particle nodes[6] = {0};
    unsigned keys[] = {10, 1, 10, 8, 1, 15};
    unsigned expected[] = {1, 4, 3, 0, 2, 5};
    for (int i = 0; i < 6; ++i) {
        nodes[i].kind = kind(keys[i]);
        nodes[i].next = i < 5 ? &nodes[i+1] : NULL;
    }
    hsd_804D0908[3] = &nodes[0];
    HSD_Particle* head;
    HSD_Particle* back;
    assert(particleSort(3, 1, &head, &back) == &nodes[1]);
    assert(head == &nodes[1] && back == &nodes[3]);
    HSD_Particle* p = head;
    for (unsigned i = 0; i < 6; ++i) { assert(p == &nodes[expected[i]]); p = p->next; }
    assert(p == NULL && hsd_804D0908[3] == head);
    assert(particleSort(3, 1, &head, &back) == &nodes[1]); /* cached same frame */
    hsd_804D0908[3] = NULL;
    assert(particleSort(3, 2, &head, &back) == NULL && head == NULL && back == NULL);
    nodes[0].next = NULL; nodes[0].kind = kind(0);
    hsd_804D0908[3] = &nodes[0];
    assert(particleSort(3, 3, &head, &back) == &nodes[0] && back == NULL);
    hsd_804D0908[3] = NULL;
    puts("Original particle sorting preserves bucket order and full-width linked-list pointers.");
    return 0;
}
