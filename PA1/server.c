#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE	1000 	// same as packet size
#define SN_SIZE 2
#define CLIENT_MESSAGE_SIZE 3

int send_packet(int client_fd, FILE *fp, int *window_count) {
    char byte_data[BUFFER_SIZE];
    int read_bytes = fread(byte_data, 1, BUFFER_SIZE, fp);
    printf("Bytes sent: %d", read_bytes);
    fflush(stdout);
    write(client_fd, byte_data, read_bytes);
    (*window_count)++;
    if (read_bytes < BUFFER_SIZE)
        return 1;
    else
        return 0;
}

int main (int argc, char **argv) {
   
    char *filename[3] = {"./data/TransferMe10.mp4", "./data/TransferMe20.mp4", "./data/TransferMe30.mp4"};

	// Check arguments
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
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

    if (listen(socket_fd, SOMAXCONN) < 0) {
        printf("Listening to socket failed!\n");
        exit(1);
    }

    printf("Start waiting...\n");
    fflush(stdout);
    int window_count;
    FILE *fp = NULL;
    int is_read_finished = 1;
    int window_size;

    while(1) {
        fflush(stdout);
        // accept client fd
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &len);
        if (client_fd < 0) {
             switch (errno) {
                case EAGAIN:
                    printf("No connections to be accepted!\n");
                    break;
                case EBADF:
                    printf("Socket fd is not valid!\n");
                    break;
                case EINVAL:
                    printf("Socket is not accepting connections!\n");
                    break;
                case ENFILE:
                    printf("Maximum number of fd exceeded!\n");
                    break;
                case ENOTSOCK:
                    printf("The socket argument does not refere to a socket!\n");
                    break;
                case ENOBUFS:
                    printf("No buffer space available!\n");
                    break;
                default:
                    printf("Unknown error!\n");
            }
            printf("Accepting client request failed! accept Returned: %d\n", client_fd); 
            exit(1);
        }
        printf("Hi!\n");
        char client_message[CLIENT_MESSAGE_SIZE];
        int message_size = read(client_fd, client_message, CLIENT_MESSAGE_SIZE);
        printf("Message Received: %c\n", client_message[0]);
        fflush(stdout);
        if (message_size != CLIENT_MESSAGE_SIZE) {
            printf("Invalid client message size!\n");
            exit(1);
        }
        if (client_message[0] == 'A') {
            // ACK
            if (window_count == 0) {
                printf("Received ACK without sending message!\n");
                exit(1);
            }
            if (fp == NULL) {
                printf("Received ACK before fp is set!");
                exit(1);
            }
            while(window_count < window_size && !is_read_finished)
                is_read_finished = send_packet(client_fd, fp, &window_count);              
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
            window_size = client_message[2];
            is_read_finished = 0;
            window_count = 0;
            while(window_count < window_size && !is_read_finished)
                is_read_finished = send_packet(client_fd, fp, &window_count);
            printf("Done!\n");
            fflush(stdout);
        } else {
            printf("Invalid client request!\n");
            exit(1);
        }
        close(client_fd);
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
