#ifndef UTILS_H
#define UTILS_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

// MACROS
#define SERVER_IP "127.0.0.1"
#define LOCAL_HOST "127.0.0.1"
#define SERVER_PORT_TO 5002
#define CLIENT_PORT 6001
#define SERVER_PORT 6002
#define CLIENT_PORT_TO 5001
#define PAYLOAD_SIZE 1024
#define WINDOW_SIZE 100
#define TIMEOUT 1.5
#define MAX_SEQUENCE 1024

#define SCALE 10

#ifndef MSG_CONFIRM
#define MSG_CONFIRM 0x800
#endif

#define MSS 1
// Packet Layout
// You may change this if you want to
struct packet {
    unsigned short seqnum;
    unsigned short acknum;
    char ack;
    char last;
    unsigned int length;
    char payload[PAYLOAD_SIZE];
};

// Utility function to build a packet
void build_packet(struct packet *pkt, unsigned short seqnum,
                  unsigned short acknum, char last, char ack,
                  unsigned int length, const char *payload) {
    pkt->seqnum = seqnum;
    pkt->acknum = acknum;
    pkt->ack = ack;
    pkt->last = last;
    pkt->length = length;
    memcpy(pkt->payload, payload, length);
}

// Utility function to print a packet
void print_recv(struct packet *pkt) {
    printf("RECV %d %d%s%s\n", pkt->seqnum, pkt->acknum,
           pkt->last ? " LAST" : "", (pkt->ack) ? " ACK" : "");
}

void print_send(struct packet *pkt, int resend) {
    if (resend)
        printf("RESEND %d %d%s%s\n", pkt->seqnum, pkt->acknum,
               pkt->last ? " LAST" : "", pkt->ack ? " ACK" : "");
    else
        printf("SEND %d %d%s%s\n", pkt->seqnum, pkt->acknum,
               pkt->last ? " LAST" : "", pkt->ack ? " ACK" : "");
}

typedef struct Heap {
    struct packet *data;
    size_t size;
    size_t capacity;
} Heap;

Heap *heap_init() {
    Heap *heap = (Heap *)(malloc(sizeof(Heap)));
    heap->data = (struct packet *)(malloc(WINDOW_SIZE * sizeof(struct packet)));
    heap->size = 0;
    heap->capacity = WINDOW_SIZE;

    return heap;
}

void heap_destroy(Heap *heap) {
    free(heap->data);
    free(heap);
}

Heap *heap_shrink(Heap *heap) {
    struct packet *temp =
        (struct packet *)(malloc(WINDOW_SIZE * sizeof(struct packet)));

    free(heap->data);
    heap->data = temp;
    heap->capacity = WINDOW_SIZE;

    return heap;
}

Heap *heap_grow(Heap *heap) {
    struct packet *temp = (struct packet *)(malloc(
        (size_t)(1.5 * heap->capacity * sizeof(struct packet))));
    memcpy(temp, heap->data, heap->capacity * sizeof(struct packet));

    heap->capacity *= 1.5;
    free(heap->data);
    heap->data = temp;

    return heap;
}

struct packet heap_top(Heap *heap) {
    if (heap->size == 0) {
        struct packet pkt;
        build_packet(&pkt, -1, -1, 0, 0, 0, "");
        return pkt;
    }

    return heap->data[0];
}

bool heap_empty(Heap *heap) { return heap->size == 0; }

bool heap_full(Heap *heap) { return heap->size == heap->capacity; }

Heap *heap_push(Heap *heap, struct packet *pkt) {
    if (heap_empty(heap)) {
        heap->data[heap->size++] = *pkt;
        return heap;
    }

    for (size_t i = 0; i < heap->size; i++)
        if (heap->data[i].seqnum == pkt->seqnum)
            return heap;

    if (heap_full(heap))
        heap = heap_grow(heap);

    size_t pos = heap->size++;

    while ((pkt->seqnum < heap->data[(pos - 1) / 2].seqnum) && 0 < pos) {
        heap->data[pos] = heap->data[(pos - 1) / 2];
        pos = (pos - 1) / 2;
    }

    heap->data[pos] = *pkt;

    return heap;
}

Heap *heap_heapify(Heap *heap, size_t index) {
    size_t left = (index * 2) + 1;
    size_t right = (index * 2) + 2;
    size_t min = index;

    if (heap->size < left || heap->size < right)
        return heap;

    if (heap->data[left].seqnum < heap->data[min].seqnum)
        min = left;

    if (heap->data[right].seqnum < heap->data[min].seqnum)
        min = right;

    if (min != index) {
        struct packet temp = heap->data[index];
        heap->data[index] = heap->data[min];
        heap->data[min] = temp;
        heap_heapify(heap, min);
    }

    return heap;
}

Heap *heap_pop(Heap *heap) {
    heap->size--;
    heap->data[0] = heap->data[heap->size];

    heap = heap_heapify(heap, 0);

    // if (heap_empty(heap) && WINDOW_SIZE < heap->capacity)
    //   heap = heap_shrink(heap);

    return heap;
}

typedef struct Node {
    struct packet data[WINDOW_SIZE];
    size_t head;
    size_t tail;
    struct Node *next;
} Node;

Node *node_init() {
    Node *node = (Node *)(malloc(sizeof(Node)));

    node->next = NULL;

    node->head = 0;
    node->tail = 0;

    return node;
}

void node_destroy(Node *node) {
    node->next = NULL;
    free(node);
}

typedef struct Queue {
    size_t size;
    double max_size;
    size_t capacity;

    Node *front;
    Node *back;
} Queue;

Queue *queue_init() {
    Queue *queue = (Queue *)(malloc(sizeof(Queue)));

    queue->size = 0;
    queue->max_size = 1;
    queue->capacity = WINDOW_SIZE;

    queue->front = node_init();
    queue->back = queue->front;

    return queue;
}

void queue_destroy(Queue *queue) {
    if (queue->front)
        node_destroy(queue->front);
    if (queue->back)
        node_destroy(queue->back);

    free(queue);
}

Queue *queue_shrink(Queue *queue) {
    Node *temp = queue->front;
    queue->front = temp->next;
    node_destroy(temp);

    return queue;
}

Queue *queue_grow(Queue *queue) {
    Node *temp = queue->back;
    temp->next = node_init();
    queue->back = temp->next;

    return queue;
}

struct packet *queue_get(Queue *queue, const unsigned short seqnum) {
    Node *front = queue->front;

    while (front) {
        for (size_t i = 0; (i + front->head) < queue->capacity; i++) {
            if (front->data[i + front->head].seqnum == seqnum)
                return &front->data[i + front->head];
        }
        front = front->next;
    }

    return NULL;
}

Queue *queue_push(Queue *queue, struct packet *pkt) {
    Node *back = queue->back;
    back->data[back->tail] = *pkt;
    back->tail++;
    queue->size++;

    if (back->tail == queue->capacity)
        queue = queue_grow(queue);

    return queue;
}

bool queue_empty(Queue *queue) { return queue->size == 0; }

size_t queue_pop(Queue *queue, unsigned short seqnum) {
    if (queue_empty(queue))
        return 0;

    size_t popped = 0;
    Node *front = queue->front;
    while (front && 0 < queue->size &&
           front->data[front->head].seqnum < seqnum) {
        front->head++;
        queue->size--;
        popped++;

        if (front->head == WINDOW_SIZE)
            queue = queue_shrink(queue);

        front = queue->front;
    }

    return popped;
}

struct packet *queue_top(Queue *queue) {
    return &queue->front->data[queue->front->head];
}

bool queue_full(Queue *queue) {
    return queue->size >= (size_t)(queue->max_size);
}

typedef enum { SLOW_START, CONGESTION_AVOIDANCE, FAST_RECOVERY } State;
#endif
