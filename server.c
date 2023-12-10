#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

int main() {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in server_addr, client_addr_from, client_addr_to;
    socklen_t addr_size = sizeof(client_addr_from);

    // Create a UDP socket for sending
    send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_sockfd < 0) {
        perror("Could not create send socket");
        return 1;
    }

    // Create a UDP socket for listening
    listen_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_sockfd < 0) {
        perror("Could not create listen socket");
        return 1;
    }

    // Configure the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the listen socket to the server address
    if (bind(listen_sockfd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(listen_sockfd);
        return 1;
    }

    // Configure the client address structure to which we will send ACKs
    memset(&client_addr_to, 0, sizeof(client_addr_to));
    client_addr_to.sin_family = AF_INET;
    client_addr_to.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    client_addr_to.sin_port = htons(CLIENT_PORT_TO);

    // Open the target file for writing (always write to output.txt)
    FILE *fp = fopen("output.txt", "wb");

    // TODO: Receive file from the client and save it as output.txt

    unsigned short expected_seq_num = 0;
    struct packet ack_pkt;
    struct packet buffer;

    unsigned short seq_num = 0;
    unsigned short ack_num = 0;
    char last = 0;
    char ack = 1;

    Heap *heap = heap_init();

    struct packet *min_pkt;

    while (1) {
        // read
        recvfrom(listen_sockfd, &buffer, sizeof(buffer), 0,
                 (struct sockaddr *)&client_addr_from, &addr_size);

        if (expected_seq_num <= buffer.seqnum) {
            heap = heap_push(heap, &buffer);
        }

        // process packets in order
        min_pkt = heap_top(heap);
        while (min_pkt && (expected_seq_num == min_pkt->seqnum) && min_pkt->length) {
            expected_seq_num++;

            fwrite(min_pkt->payload, sizeof(char), min_pkt->length, fp);

            if (min_pkt->last) {
                last = 1;
            }

            heap = heap_pop(heap);
            min_pkt = heap_top(heap);
        }

        build_packet(&ack_pkt, seq_num = buffer.seqnum,
                     ack_num = expected_seq_num, last, ack, 0, "");
        sendto(send_sockfd, &ack_pkt, sizeof(struct packet), 0,
               (const struct sockaddr *)&client_addr_to, addr_size);

        if (last) {
            break;
        }
    }

    build_packet(&ack_pkt, seq_num, ack_num, last, ack, 0, "");
    sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0,
           (const struct sockaddr *)&client_addr_to, addr_size);

    heap_destroy(heap);
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}
