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

int main (int argc, char **argv) {
   
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
        printf("Opening socket failed!");
        exit(1);
    }

    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    // Binding
    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Binding failed! bind() returned");
        exit(1);
    }

    if (listen(socket_fd, 1) < 0) {
        printf("Listening to socket failed!");
        exit(1);
    }

    printf("Start waiting...");

    struct server_packet *window = (struct server_packet *)malloc(window_size * sizeof(struct server_packet));

    while(1) {
        // accept client fd
        int len;
        struct sockaddr_in client_addr;
        int client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &len);
        if (client_fd < 0) {
            printf("Accepting client request failed!\n");
            continue;
        }
        char client_message[CLIENT_MESSAGE_SIZE];
        int message_size = read(client_fd, client_message, CLIENT_MESSAGE_SIZE);
        if (message_size != CLIENT_MESSAGE_SIZE) {
            printf("Invalid client message size!");
            continue;
        }
        if (client_message[0] == 'A') {
            // ACK
        } else if (client_message[0] == 'R') {
            // Request
        } else {
            printf("Invalid client request!");
            continue;
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
