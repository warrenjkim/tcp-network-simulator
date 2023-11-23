#include "utils.h"

CQueue *cqueue_init() {
    CQueue *queue = (CQueue *)(malloc(sizeof(CQueue)));

    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->max_size = IW;
    queue->capacity = WINDOW_SIZE;
    queue->data = (struct packet *)(malloc(WINDOW_SIZE * sizeof(struct packet)));

    return queue; 
}


void cqueue_resize(CQueue *queue) {
    queue->max_size = queue->capacity;
    queue->capacity *= 1.5;

    struct packet *temp = (struct packet *)(malloc(queue->capacity * sizeof(struct packet)));
    
    if (!temp) {
        perror("cqueue: realloc failed");
        exit(1);
    }

    for (size_t i = 0; i < queue->size; i++)
        temp[i] = queue->data[(queue->head + i) % queue->max_size];
    

    queue->head = 0;
    queue->tail = queue->size - 1;
    queue->data = temp;
}


void cqueue_destroy(CQueue *queue) {
    free(queue);
}


CQueue *cqueue_push(CQueue *queue, struct packet *pkt) {
    if (queue->max_size == queue->capacity)
        cqueue_resize(queue);

    queue->data[queue->tail] = *pkt;
    queue->tail = (queue->tail + 1) % queue->max_size;
    queue->size++;


    return queue;
}


CQueue *cqueue_pop(CQueue *queue, const unsigned short seqnum) {
    if (cqueue_empty(queue))
        return queue;

    while (0 < queue->size && queue->data[queue->head].seqnum < seqnum) {
        if (queue->data[queue->head].seqnum == 0)
            break;

        queue->head = (queue->head + 1) % queue->max_size;
        queue->size--;

        queue->max_size += cqueue_full(queue) ? 0 : 1;
        printf("popped %d\n", queue->data[queue->head].seqnum);
    }

    return queue;
}


bool cqueue_empty(CQueue *queue) {
    return queue->size == 0;
}


bool cqueue_full(CQueue *queue) {
    return queue->max_size <= queue->size;
}


struct packet *cqueue_get(CQueue *queue, const unsigned short seqnum) {
    for (size_t i = 0; i < queue->size; i++) {
        size_t index = (queue->head + i) % queue->max_size;
        if (queue->data[index].seqnum == seqnum)
            return &queue->data[index];
    }
    return NULL;
}

struct packet *cqueue_top(CQueue *queue) {
    return &queue->data[queue->head];
}
