#include "utils.h"

int main() {
    Heap *heap = heap_init();
    
    struct packet pkt;

    int arr[] = {6, 7, 12, 10, 1, 2, 8, 11, 30, 21};
    int arr2[] = {42,77,53,62,2,23,64,32,25,65};
    int dup[] = {3, 2, 2, 2, 9, 2, 6, 7, 3, 9};
    int del[] = { 7, 3, 8, 1, 5 };


    printf("=========\n");
    printf("HEAP TEST\n");
    printf("=========\n");
    printf("test push");

    for (int i = 0; i < 10; i++) {
        build_packet(&pkt, dup[i], 0, 0, 0, 0, "");
        heap = heap_push(heap, &pkt);

        printf("\n\nmin: %d\n", heap->data[0].seqnum);
        printf("size: %ld\n", heap->size);
        printf("heap: ");
        for (int j = 0; j < heap->size; j++) {
            printf("%d ", heap->data[j].seqnum);
        }
    }

    printf("\n\ntest pop");

    while (heap->size) {
        printf("\n\nmin: %d\n", heap_top(heap).seqnum);
        heap = heap_pop(heap);
        printf("new min: %d\n", heap_top(heap).seqnum == 65535 ? -1 : heap_top(heap).seqnum);
        printf("size: %ld\n", heap->size);
        printf("heap: ");

        for (int j = 0; j < heap->size; j++)
            printf("%d ", heap->data[j].seqnum);
    }

    printf("\n\n=========\n");
    printf("RED-BLACK TREE TEST\n");
    printf("=========\n");

    Tree *tree = rbt_init();

    for (size_t i = 0; i < 5; i++) {
        build_packet(&pkt, del[i], del[i], 0, 0, 0, "");
        tree->root = rbt_insert(tree->root, &pkt, &(tree->size));

        printf("\n\nroot: %d\n", tree->root->id);
        rbt_print_tree(tree->root, 0);
    }
    rbt_delete(tree->root, 1);
    printf("\n\nroot: %d\n", tree->root->id);
    rbt_print_tree(tree->root, 0);


    return 0;
}
