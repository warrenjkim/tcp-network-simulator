#include "utils.h"

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

