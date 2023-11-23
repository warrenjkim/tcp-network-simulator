#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#include "utils.h"

#define MSG_CONFIRM 0x800

int main(int argc, char *argv[]) {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in client_addr, server_addr_to, server_addr_from;
    socklen_t addr_size = sizeof(server_addr_to);
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;


    // read filename from command line argument
    if (argc != 2) {
        printf("Usage: ./client <filename>\n");
        return 1;
    }
    char *filename = argv[1];

    // Create a UDP socket for listening
    listen_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_sockfd < 0) {
        perror("Could not create listen socket");
        return 1;
    }

   if (setsockopt(listen_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
       perror("couldn't set receive timeout");
       return 1;
   }

    // Create a UDP socket for sending
    send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_sockfd < 0) {
        perror("Could not create send socket");
        return 1;
    }

   if (setsockopt(listen_sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
       perror("couldn't set send timeout");
       return 1;
   }

    // Configure the server address structure to which we will send data
    memset(&server_addr_to, 0, sizeof(server_addr_to));
    server_addr_to.sin_family = AF_INET;
    server_addr_to.sin_port = htons(SERVER_PORT_TO); // 5002
    server_addr_to.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Configure the client address structure
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PORT); // 6001
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the listen socket to the client address
    if (bind(listen_sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed");
        close(listen_sockfd);
        return 1;
    }

    // Open file for reading
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        close(listen_sockfd);
        close(send_sockfd);
        return 1;
    }

    // TODO: Read from file, and initiate reliable data transfer to the server
    


    struct packet pkt;
    struct packet ack_pkt;
    char buffer[PAYLOAD_SIZE];
    unsigned short seq_num = 1;
    unsigned short ack_num = 1;
    char last = 0;
    char ack = 0;

    ssize_t bytes_read = 0;

    ack_pkt.seqnum = -1;
    ack_pkt.acknum = -1;
    ack_pkt.last = 0;
    int count = 0;
    unsigned short old = 0;

    unsigned int ssthresh = (unsigned int)(-1);
    CQueue *cwnd = cqueue_init();
    

    while (1) {
        printf("head %ld ", cwnd->head);
        printf("tail %ld\n", cwnd->tail);
        for (size_t i = 0; i < cwnd->size; i++) {
            printf("%d ", cwnd->data[(cwnd->head + i) % cwnd->max_size].seqnum);
        }
        printf("\n");
        DEBUG_PRINT("cwnd->size %ld\n", cwnd->size);
        DEBUG_PRINT("cwnd->max_size %ld\n", cwnd->max_size);
        DEBUG_PRINT("cwnd->capacity %ld\n", cwnd->capacity);

        count = (old == ack_pkt.acknum) ? (count + 1) : 0;
        old = ack_pkt.acknum;

        if (ack_pkt.last)
            break;

        if (ack_pkt.acknum != (unsigned short)(-1))
            cwnd = cqueue_pop(cwnd, ack_pkt.acknum);

        while (!cqueue_full(cwnd) && !last) {
            bytes_read = fread(buffer, sizeof(char), PAYLOAD_SIZE, fp);

            if (bytes_read < PAYLOAD_SIZE)
                last = 1;

            DEBUG_PRINT("bytes read: %ld\n", bytes_read);
            build_packet(&pkt, seq_num++, ack_num++, last, ack, bytes_read, buffer);
            sendto(send_sockfd, &pkt, sizeof(pkt), MSG_CONFIRM, (const struct sockaddr *) &server_addr_to, addr_size);
            cwnd = cqueue_push(cwnd, &pkt);
            print_send(&pkt, ack);

            printf("\n");
        }

        if (count == 3) {
            old = cqueue_top(cwnd)->seqnum;
            count = 0;
            // size_t new_max = cwnd->max_size / 2;
            // cwnd->max_size = new_max < 2 ? 2 : new_max;
            struct packet *resend = cqueue_get(cwnd, ack_pkt.acknum);
            if (!resend)
                continue;
            sendto(send_sockfd, resend, sizeof(*resend), MSG_CONFIRM, (const struct sockaddr *) &server_addr_to, addr_size);
            print_send(resend, 1);
        }

        ssize_t timeout = recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_WAITALL, (struct sockaddr *) &server_addr_from, &addr_size);
        print_recv(&ack_pkt);
        DEBUG_PRINT("\n");

        if (timeout < 0) {
            // size_t new_max = cwnd->max_size / 2;
            // cwnd->max_size = new_max < 2 ? 2 : new_max;
            struct packet *resend = cqueue_top(cwnd);
            if (!resend)
                continue;
            sendto(send_sockfd, resend, sizeof(*resend), MSG_CONFIRM, (const struct sockaddr *) &server_addr_to, addr_size);
            print_send(resend, last);
        }
    }


    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}

