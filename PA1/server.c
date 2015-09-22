#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE	1000 	// same as packet size
#define SN_SIZE 2
#define CLIENT_MESSAGE_SIZE 3

struct server_packet {
    short sn;
    char file_data[BUFFER_SIZE - 2];
};

char* server_packet_to_bytes(struct server_packet packet) {
    char *return_byte = (char *)malloc(BUFFER_SIZE * sizeof(char));
    
    return_byte[0] = packet.sn / 256;
    return_byte[1] = packet.sn % 256;
    memcpy(return_byte+2, packet.file_data, (BUFFER_SIZE - 2) * sizeof(char));

    return return_byte;
}

void delete_and_shift(struct server_packet *window, int *window_count) {
    int i;
    for(i = 1; i < *window_count; i ++) {
        window[i-1] = window[i];
    }
    *window_count--;
}

int read_file(FILE *fp, struct server_packet *packet, short *sn) {
    int i;
    
    packet->sn = *sn;
    *sn++;
    for (i = 0; i < BUFFER_SIZE - 2; i ++) {
        char temp;
        if ((temp = fgetc(fp)) == EOF) {
            return 1;
        } else {
            packet->file_data[i] = temp;
        }
    }
    return 0;
}

int main (int argc, char **argv) {
   
    char *filename[3] = {"./data/TransferMe10.mp4", "./data/TransferMe20.mp4", "./data/TransferMe30.mp4"};

	// Check arguments
    if (argc != 3) {
        printf("Usage: %s <port> <window_size>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int window_size = atoi(argv[2]);
    int socket_fd;
    // Opening socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Opening socket failed!\n");
        exit(1);
    }

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    // Binding
    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Binding failed! bind() returned\n");
        exit(1);
    }

    if (listen(socket_fd, 1) < 0) {
        printf("Listening to socket failed!\n");
        exit(1);
    }

    printf("Start waiting...");

    struct server_packet *window = (struct server_packet *)malloc(window_size * sizeof(struct server_packet));
    int window_count;
    FILE *fp = NULL;
    int is_read_finished = 1;
    short sn;

    while(1) {
        // accept client fd
        int len;
        struct sockaddr_in client_addr;
        int client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &len);
        if (client_fd < 0) {
            printf("Accepting client request failed!\n");
            exit(1);
        }
        char client_message[CLIENT_MESSAGE_SIZE];
        int message_size = read(client_fd, client_message, CLIENT_MESSAGE_SIZE);
        if (message_size != CLIENT_MESSAGE_SIZE) {
            printf("Invalid client message size!\n");
            exit(1);
        }
        if (client_message[0] == 'A') {
            short client_sn = client_message[1] * 256 + client_message[2];
            // ACK
            if (window_count == 0) {
                printf("Received ACK without sending message!\n");
                exit(1);
            }
            if (fp == NULL) {
                printf("Received ACK before fp is set!");
                exit(1);
            }
            if (window[0].sn != client_sn) {
                printf("ACK or message is missing!\n");
                exit(1);
            }
            delete_and_shift(window, &window_count);
            if (!is_read_finished) {
                while(window_count < window_size && !is_read_finished) {
                    is_read_finished = read_file(fp, &window[window_count], &sn);
                    window_count++;
                }
            }
        } else if (client_message[0] == 'R') {
            // Request
            int file_choice = client_message[1];
            if (file_choice < 0 || file_choice > 2) {
                printf("Invalid file number!\n");
                exit(1);
            }
            if (!is_read_finished) {
                printf("Get another request before the previous request is finished!\n");
                exit(1);
            }
            fp = fopen(filename[file_choice], "r");
            is_read_finished = 0;
            window_count = 0;
            sn = 0;
            while(window_count < window_size && !is_read_finished) {
                is_read_finished = read_file(fp, &window[window_count], &sn);
                window_count++;
            }
        } else {
            printf("Invalid client request!\n");
            exit(1);
        }
    }

	// TODO: Read a specified file and send it to a client.
    //      Send as many packets as window size (speified by
    //      client) allows and assume each packet contains
    //      1000 Bytes of data.
	//		When a client receives a packet, it will send back
	//		ACK packet.
    //      When a server receives an ACK packet, it will send
    //      next packet if available.
    close(socket_fd);
    return 0;
}
