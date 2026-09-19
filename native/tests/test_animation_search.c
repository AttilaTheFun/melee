#include <melee/gr/granime.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <assert.h>
#include <stdio.h>

int main(void)
{
    HSD_AObj first = {0}, second = {0};
    HSD_JObj root = {0}, child = {0}, sibling = {0};
    root.child = &child;
    child.next = &sibling;
    child.aobj = &first; sibling.aobj = &second;
    assert(grAnime_FindFirstAObj(NULL, JOBJ_MASK) == NULL);
    assert(grAnime_FindFirstAObj(&root, 0) == NULL);
    assert(grAnime_FindFirstAObj(&root, JOBJ_MASK) == &first);
    child.aobj = NULL;
    assert(grAnime_FindFirstAObj(&root, JOBJ_MASK) == &second);
    sibling.aobj = NULL;
    assert(grAnime_FindFirstAObj(&root, JOBJ_MASK) == NULL);
    root.aobj = &first;
    assert(grAnime_FindFirstAObj(&root, JOBJ_MASK) == &first);
    /* Finding the root must abort before descending further. */
    root.child = &root;
    assert(grAnime_FindFirstAObj(&root, JOBJ_MASK) == &first);
    root.child = NULL;
    for (int i = 0; i < 100; ++i) {
        root.aobj = i % 2 ? &second : NULL;
        assert(grAnime_FindFirstAObj(&root, JOBJ_MASK) == root.aobj);
    }
    puts("Original HSD traversal finds full-width animation pointers with native early exit.");
    return 0;
}
