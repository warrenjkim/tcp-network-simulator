#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

// MACROS
#define SERVER_IP "127.0.0.1"
#define LOCAL_HOST "127.0.0.1"
#define SERVER_PORT_TO 5002
#define CLIENT_PORT 6001
#define SERVER_PORT 6002
#define CLIENT_PORT_TO 5001
#define PAYLOAD_SIZE 1024
#define WINDOW_SIZE 5
#define TIMEOUT 2
#define MAX_SEQUENCE 1024



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
void build_packet(struct packet* pkt, unsigned short seqnum, unsigned short acknum, char last, char ack,unsigned int length, const char* payload) {
    pkt->seqnum = seqnum;
    pkt->acknum = acknum;
    pkt->ack = ack;
    pkt->last = last;
    pkt->length = length;
    memcpy(pkt->payload, payload, length);
}

// Utility function to print a packet
void print_recv(struct packet* pkt) {
    printf("RECV seqnum: %d acknum: %d%s%s\n", pkt->seqnum, pkt->acknum, pkt->last ? " LAST": "", (pkt->ack) ? " ACK": "");
}

void print_send(struct packet* pkt, int resend) {
    if (resend)
        printf("RESEND seqnum: %d acknum: %d%s%s\n", pkt->seqnum, pkt->acknum, pkt->last ? " LAST": "", pkt->ack ? " ACK": "");
    else
        printf("SEND seqnum: %d acknum: %d%s%s\n", pkt->seqnum, pkt->acknum, pkt->last ? " LAST": "", pkt->ack ? " ACK": "");
}




typedef enum Color {
    RED,
    BLACK
} Color;


typedef struct Node {
    unsigned short id; // pkt.acknum
    struct packet pkt;
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

Tree *rbt_init();
Node *rbt_insert(Node *root, struct packet *pkt);
Node *rbt_delete(Node *root, const unsigned short id);
Node *rbt_successor(Node *node);
Node *rbt_balance(Node *root);
void rbt_ll_rotate(Node *root);
Node *rbt_lr_rotate(Node *root);
Node *rbt_rl_rotate(Node *root);
Node *rbt_rr_rotate(Node *root);
void rbt_inorder(Node *root);
Node *grandparent(Node *node);
Node *uncle(Node* node);
Node *recolor();





// red black tree properties
// 1. every node is either RED or BLACK
// 2. root and leaves is/are always BLACK
// 3. RED nodes cannot have RED children
// 4. every path from a node to any of its leaves have the same number of BLACK nodes

#endif
