#include "utils.h"


Heap *heap_init() {
    Heap *heap = (Heap *)(malloc(sizeof(Heap *)));
    heap->size = 0;

    return heap;
}



Heap *heap_push(Heap *heap, struct packet *pkt) {
    if (heap->size == 0) {
        heap->data[heap->size++] = *pkt;
        return heap;
    }

    for (size_t i = 0; i < heap->size; i++)
        if (heap->data[i].seqnum == pkt->seqnum)
            return heap;

    size_t pos = heap->size++;
    
    while ((pkt->seqnum < heap->data[(pos - 1) / 2].seqnum) && 0 < pos) {
        heap->data[pos] = heap->data[(pos - 1) / 2];
        pos = (pos - 1) / 2;
    }

    heap->data[pos] = *pkt;

    return heap;

    // size_t pos = heap->size++;
    // 
    // while ((pkt->seqnum < heap->data[(pos - 1) / 2].seqnum) && 0 < pos) {
    //     heap->data[pos] = heap->data[(pos - 1) / 2];
    //     pos = (pos - 1) / 2;
    // }

    // heap->data[pos] = *pkt;

    // return heap;
}


Heap *heap_pop(Heap *heap) {
    struct packet old = heap_top(heap);
        
    heap->data[0] = heap->data[--heap->size];
   
    heap = heap_heapify(heap, 0);

    if (heap_top(heap).seqnum == old.seqnum && 0 < heap->size)
        heap = heap_pop(heap);

    return heap;
}

Heap *heap_heapify(Heap *heap, size_t index) {
    if (heap->size < (index * 2) + 1)
        return heap;

    struct packet left = heap->data[(index * 2) + 1];
    struct packet right = heap->data[(index * 2) + 2];
    const unsigned short min = left.seqnum < right.seqnum ? left.seqnum : right.seqnum;
    size_t new_index = 0;

    if (heap->data[index].seqnum < min)
        return heap;

    if (min == left.seqnum || heap->size < (index * 2) + 2)
        new_index = (index * 2) + 1;
    else
        new_index = (index * 2) + 2;

    struct packet temp = heap->data[new_index];
    heap->data[new_index] = heap->data[index];
    heap->data[index] = temp;



    return heap_heapify(heap, new_index);
}


struct packet heap_top(Heap *heap) {
    if (heap->size == 0) {
        struct packet pkt;
        build_packet(&pkt, -1, 0, 0, 0, 0, "");
        return pkt;
    }
    return heap->data[0];
}
