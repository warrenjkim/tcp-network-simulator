#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#include "utils.h"


int main(int argc, char *argv[]) {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in client_addr, server_addr_to, server_addr_from;
    socklen_t addr_size = sizeof(server_addr_to);
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    struct packet pkt;
    struct packet ack_pkt;
    char buffer[PAYLOAD_SIZE];
    unsigned short seq_num = 0;
    unsigned short ack_num = 0;
    char last = 0;
    char ack = 0;

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
    struct packet cwnd[WINDOW_SIZE];
    ssize_t bytes_read = 0;
    int i = 0;
    while (0 < (bytes_read = fread(buffer, sizeof(char), sizeof(buffer) - 1, fp))) {
        if (i == WINDOW_SIZE) {
            printf("cwnd full!\n");
        }

        build_packet(&pkt, seq_num, ack_num, last, ack, sizeof(buffer) - 1, buffer);
        sendto(send_sockfd, &pkt, sizeof(pkt), 0, (const struct sockaddr *) &server_addr_to, addr_size);
        print_send(&pkt, 0);

        cwnd[(i++ % WINDOW_SIZE)] = pkt;

        ssize_t bytes_recv = recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_WAITALL, (struct sockaddr *) &server_addr_from, &addr_size);
        seq_num += bytes_recv + 1;
        ack_num = ack_pkt.acknum;
        print_recv(&ack_pkt);
        
        while (ack_pkt.last) {
            sendto(send_sockfd, &pkt, sizeof(pkt), 0, (const struct sockaddr *) &server_addr_to, addr_size);
            print_send(&pkt, 1);

            cwnd[(i++ % WINDOW_SIZE)] = pkt;

            bytes_recv = recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), MSG_WAITALL, (struct sockaddr *) &server_addr_from, &addr_size);
            seq_num += bytes_recv + 1;
            ack_num = ack_pkt.acknum;
            print_recv(&ack_pkt);
        }
    }

    sendto(send_sockfd, NULL, 0, 0, (const struct sockaddr *) &server_addr_to, addr_size);

 
    
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}

