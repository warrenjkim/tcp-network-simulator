#ifndef UTILS_H
#define UTILS_H
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// MACROS
#define SERVER_IP "127.0.0.1"
#define LOCAL_HOST "127.0.0.1"
#define SERVER_PORT_TO 5002
#define CLIENT_PORT 6001
#define SERVER_PORT 6002
#define CLIENT_PORT_TO 5001
#define PAYLOAD_SIZE 1190
#define WINDOW_SIZE 100
#define TIMEOUT 2.2

#define MSS 1

// Packet Layout
// You may change this if you want to
struct packet {
    unsigned short seqnum;
    unsigned short acknum;
    char ack;
    char last;
    unsigned short length;
    char payload[PAYLOAD_SIZE];
};

// Utility function to build a packet
void build_packet(struct packet *pkt, unsigned short seqnum,
                  unsigned short acknum, char last, char ack,
                  unsigned short length, const char *payload) {
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
    if (!heap) {
        perror("heap: malloc failed");
        exit(1);
    }

    heap->data = (struct packet *)(malloc(WINDOW_SIZE * sizeof(struct packet)));
    if (!heap->data) {
        perror("heap->data: malloc failed");
        exit(1);
    }

    heap->size = 0;
    heap->capacity = WINDOW_SIZE;

    return heap;
}

void heap_destroy(Heap *heap) {
    if (!heap) {
        return;
    }

    free(heap->data);
    free(heap);
}

Heap *heap_grow(Heap *heap) {
    if (!heap) {
        return heap_init();
    }

    struct packet *temp = (struct packet *)(malloc(
        (size_t)(2 * heap->capacity * sizeof(struct packet))));
    if (!temp) {
        perror("heap_grow(): malloc failed");
        exit(1);
    }

    memcpy(temp, heap->data, heap->size * sizeof(struct packet));
    heap->capacity *= 2;
    free(heap->data);
    heap->data = temp;

    return heap;
}

Heap *heap_heapify(Heap *heap, size_t index) {
    if (!heap) {
        return heap_init();
    }

    while (true) {
        size_t left = (index * 2) + 1;
        size_t right = (index * 2) + 2;
        size_t min = index;

        if (left < heap->size &&
            heap->data[left].seqnum < heap->data[min].seqnum) {
            min = left;
        }

        if (right < heap->size &&
            heap->data[right].seqnum < heap->data[min].seqnum) {
            min = right;
        }

        if (min != index) {
            struct packet temp = heap->data[index];
            heap->data[index] = heap->data[min];
            heap->data[min] = temp;

            index = min;
        } else {
            break;
        }
    }

    return heap;
}

bool heap_empty(Heap *heap) {
    if (!heap) {
        perror("heap_empty(): heap is NULL");
        exit(1);
    }

    return heap->size == 0;
}

bool heap_full(Heap *heap) {
    if (!heap) {
        perror("heap_full(): heap is NULL");
        exit(1);
    }

    return heap->size == heap->capacity;
}

struct packet *heap_top(Heap *heap) {
    if (!heap) {
        perror("heap_top(): heap is NULL");
        exit(1);
    }

    if (heap_empty(heap)) {
        return NULL;
    }

    return &heap->data[0];
}

Heap *heap_push(Heap *heap, struct packet *pkt) {
    if (!heap) {
        return heap_push(heap_init(), pkt);
    }

    if (heap_empty(heap)) {
        heap->data[heap->size++] = *pkt;
        return heap;
    }

    if (heap_full(heap)) {
        heap = heap_grow(heap);
    }

    size_t pos = heap->size++;

    while ((pkt->seqnum < heap->data[(pos - 1) / 2].seqnum) && 0 < pos) {
        heap->data[pos] = heap->data[(pos - 1) / 2];
        pos = (pos - 1) / 2;
    }

    heap->data[pos] = *pkt;

    return heap;
}

Heap *heap_pop(Heap *heap) {
    if (!heap) {
        perror("heap_pop(): heap is NULL");
        exit(1);
    }

    heap->size--;
    heap->data[0] = heap->data[heap->size];

    heap = heap_heapify(heap, 0);

    if (heap_empty(heap)) {
        return heap_init();
    }

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
    if (!node) {
        perror("node: malloc failed");
        exit(1);
    }

    for (size_t i = 0; i < WINDOW_SIZE; i++) {
        build_packet(&node->data[i], 0, 0, 0, 0, 0, "");
    }
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
    if (!queue) {
        perror("queue: malloc failed");
        exit(1);
    }

    queue->size = 0;
    queue->max_size = 1;
    queue->capacity = WINDOW_SIZE;

    queue->front = node_init();
    queue->back = queue->front;

    return queue;
}

void queue_destroy(Queue *queue) {
    if (!queue) {
        return;
    }

    while (queue->front) {
        Node *next = queue->front->next;
        node_destroy(queue->front);
        queue->front = next;
    }

    queue->front = NULL;
    queue->back = NULL;

    free(queue);
}

Queue *queue_shrink(Queue *queue) {
    if (!queue) {
        return queue_init();
    }
    Node *temp = queue->front;
    queue->front = temp->next;
    node_destroy(temp);

    return queue;
}

Queue *queue_grow(Queue *queue) {
    if (!queue) {
        return queue_init();
    }
    Node *temp = queue->back;
    temp->next = node_init();
    queue->back = temp->next;

    return queue;
}

bool queue_empty(Queue *queue) {
    if (!queue) {
        perror("queue_empty(): queue is NULL");
        exit(1);
    }

    return queue->size == 0;
}

bool queue_full(Queue *queue) {
    if (!queue) {
        perror("queue_full(): queue is NULL");
        exit(1);
    }

    return queue->size >= (size_t)(queue->max_size);
}

struct packet *queue_get(Queue *queue, const unsigned short seqnum) {
    if (!queue) {
        perror("queue_get(): queue is NULL");
        exit(1);
    }

    Node *front = queue->front;

    while (front) {
        for (size_t i = front->head; i < queue->capacity; i++) {
            if (front->data[i].seqnum == seqnum) {
                return &front->data[i];
            }
        }

        front = front->next;
    }

    return NULL;
}

struct packet *queue_top(Queue *queue) {
    if (!queue) {
        perror("queue_top(): queue is NULL");
        exit(1);
    }

    return &queue->front->data[queue->front->head];
}

Queue *queue_push(Queue *queue, struct packet *pkt) {
    if (!queue) {
        perror("queue_push(): queue is NULL");
        exit(1);
    }

    Node *back = queue->back;
    if (!back) {
        perror("queue_push(): queue->back is NULL");
        exit(1);
    }

    back->data[back->tail] = *pkt;
    back->tail++;
    queue->size++;

    if (back->tail == queue->capacity) {
        queue = queue_grow(queue);
    }

    return queue;
}

size_t queue_pop_cum(Queue *queue, unsigned short acknum) {
    if (!queue) {
        perror("queue_pop_cum(): queue is NULL");
        exit(1);
    }

    if (queue_empty(queue)) {
        return 0;
    }

    size_t popped = 0;
    Node *front = queue->front;
    while (front && !queue_empty(queue) &&
           front->data[front->head].seqnum < acknum) {
        if (front->data[front->head].ack != 0) {
            popped++;
            queue->size--;
        }

        front->data[front->head].ack = 0;
        front->head++;

        if (front->head == queue->capacity) {
            queue = queue_shrink(queue);
        }

        front = queue->front;
    }

    return popped;
}

size_t queue_pop(Queue *queue, unsigned short seqnum) {
    if (!queue) {
        perror("queue_pop(): queue is NULL");
        exit(1);
    }

    Node *back = queue->back;
    for (size_t i = back->head; i < queue->capacity; i++) {
        if (back->data[i].seqnum == seqnum) {
            if (back->data[i].ack != 0) {
                back->data[i].ack = 0;
                queue->size--;
                return 1;
            }

            return 0;
        }
    }

    return 0;
}

typedef enum State { SLOW_START, CONGESTION_AVOIDANCE, FAST_RECOVERY } State;
#endif
