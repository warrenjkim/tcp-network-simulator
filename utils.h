#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdbool.h>

// MACROS
#define SERVER_IP "127.0.0.1"
#define LOCAL_HOST "127.0.0.1"
#define SERVER_PORT_TO 5002
#define CLIENT_PORT 6001
#define SERVER_PORT 6002
#define CLIENT_PORT_TO 5001
#define PAYLOAD_SIZE 1024
#define WINDOW_SIZE 10
#define TIMEOUT 2
#define MAX_SEQUENCE 1024

#define MSG_CONFIRM 0x800

#define SMSS 1036
#define IW 4


// #define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif


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
void build_packet(struct packet* pkt, unsigned short seqnum, unsigned short acknum, char last, char ack,unsigned int length, const char* payload);

// Utility function to print a packet
void print_recv(struct packet* pkt);

void print_send(struct packet* pkt, int resend);



typedef enum Color {
    RED,
    BLACK
} Color;


typedef struct Node {
    unsigned short id;
    Color color;
    struct Node *parent;
    struct Node *left;
    struct Node *right;
} Node;

Node *node_init(const struct packet *pkt);
void node_destroy(Node *node);

typedef struct Tree {
    Node *root;
    size_t size;
} Tree;


typedef struct Heap {
    struct packet *data;
    size_t size;
    size_t capacity;
} Heap;

Tree *rbt_init();
void rbt_destroy(Tree *tree);

Node *node_insert(Node *root, struct packet *pkt, Node **Z);
Node *node_delete(Node *root, Node *V);
Node *rbt_insert(Node *root, struct packet *pkt, size_t *size);
Node *rbt_delete(Node *root, const unsigned short id);
Node *rbt_successor(Node *node);

Node *rbt_restructure(Node *root);
Node *rbt_balance_insert(Node *root);
Node *rbt_double_black(Node *root, Node *V);
Node *rbt_ll_rotate(Node *X);
Node *rbt_rr_rotate(Node *X);
Node *rbt_lr_rotate(Node *X);
Node *rbt_rl_rotate(Node *X);

Node *rbt_grandparent(Node *node);
Node *rbt_uncle(Node *node);
Node *rbt_sibling(Node *node);
Node *rbt_recolor(Node *Z);


void rbt_inorder(Node *root);
void rbt_print_tree(Node *root, size_t space);

unsigned short rbt_next(Node *root, unsigned short *expected);

// red black tree properties
// 1. every node is either RED or BLACK
// 2. root and leaves is/are always BLACK
// 3. RED nodes cannot have RED children
// 4. every path from a node to any of its leaves have the same number of BLACK nodes



Heap *heap_init();
void heap_destroy(Node *root);


Heap *heap_push(Heap *heap, struct packet *pkt);
Heap *heap_pop(Heap *heap);

struct packet heap_top(Heap *heap);
Heap *heap_heapify(Heap *heap, size_t index);






typedef struct CQueue {
    struct packet *data;
    size_t size;
    size_t head;
    size_t tail;
    size_t max_size;
    size_t capacity;
} CQueue;

CQueue *cqueue_init();
void cqueue_destroy(CQueue *queue);

CQueue *cqueue_push(CQueue *queue, struct packet *pkt);
CQueue *cqueue_pop(CQueue *queue, const unsigned short seqnum);

bool cqueue_empty(CQueue *queue);
bool cqueue_full(CQueue *queue);

struct packet *cqueue_get(CQueue *queue, const unsigned short seqnum);
struct packet *cqueue_top(CQueue *queue);

void cqueue_resize(CQueue *queue);









#endif
