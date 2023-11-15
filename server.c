#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

int main() {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in server_addr, client_addr_from, client_addr_to;
    struct packet buffer;
    socklen_t addr_size = sizeof(client_addr_from);
    int expected_seq_num = 0;
    int recv_len;
    struct packet ack_pkt;

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

     while (1) {
        recv_len = recvfrom(listen_sockfd, &buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *) &client_addr_from, &addr_size);
        print_recv(&buffer);
        unsigned short ack_num = buffer.acknum + recv_len + 1;
        unsigned char last = 0;
        unsigned char ack = 0;

        if (recv_len <= 0) {
            break;
        }

        if (buffer.seqnum == expected_seq_num) {
            ack = 1;
            expected_seq_num += recv_len + 1;
            fprintf(fp, "%s", buffer.payload);
        }
        else if (buffer.seqnum < expected_seq_num) {
            ack = 1;
        }
        else {
            last = 1;
            printf("\nbuffer: %d\n", buffer.seqnum);
            printf("expected: %d\n\n", expected_seq_num);
        }

        build_packet(&ack_pkt, expected_seq_num, ack_num, last, ack, 0, "");
        sendto(send_sockfd, &ack_pkt, sizeof(ack_pkt), 0, (const struct sockaddr *) &client_addr_to, addr_size);
        print_send(&ack_pkt, last);
     }





    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}
