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
    if (bind(listen_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
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

    if (!fp) {
        printf("error");
        return 1;
    }

    // TODO: Receive file from the client and save it as output.txt
    struct packet buffer;
    struct packet ack_pkt;
    int expected_seq_num = 0;
    unsigned short seq_num = 0;
    unsigned short ack_num = 0;
    unsigned char ack = 0;
    unsigned char last = 0;
    unsigned short last_num = -1;

    struct packet queue[WINDOW_SIZE];
    Heap *heap = heap_init();

    
    while (1) {
         recvfrom(listen_sockfd, &buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *) &client_addr_from, &addr_size);
         print_recv(&buffer);

         if (buffer.last)
             last_num = buffer.seqnum;

         seq_num = buffer.seqnum;

         heap = heap_push(heap, &buffer);
         struct packet min_pkt = heap_top(heap);
         printf("Min pkt: %d", min_pkt.seqnum);
        
         if (min_pkt.seqnum == expected_seq_num) {
             while (min_pkt.seqnum == expected_seq_num && 0 < heap->size) {
                 expected_seq_num++;
                 printf("writing %d\n", min_pkt.seqnum);
                 size_t bytes_written = fwrite(min_pkt.payload, sizeof(char), min_pkt.length, fp);
                 printf("bytes written: %ld\n", bytes_written);
                 heap = heap_pop(heap);
                 min_pkt = heap_top(heap);
             }
         }
         ack_num = expected_seq_num;
         
         if (expected_seq_num >= last_num)
             last = 1;

         build_packet(&ack_pkt, seq_num, ack_num, last, ack, 0, "");
         sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0, (const struct sockaddr *) &client_addr_to, addr_size);
         print_send(&ack_pkt, ack);
         printf("\n");

         if (last)
             break;

    }

    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}
