#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE	1000 	// same as packet size

int main (int argc, char **argv) {
   
	// Check arguments
    if (argc != 3) {
        printf("Usage: %s <port> <window_size>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int window_size = atoi(argv[2]);
	
    // TODO: Create sockets for a server and a client
    //      bind, listen, and accept

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

    if (listen(socket_fd, 5) < 0) {
        printf("Listening to socket failed!");
        exit(1);
    }

    printf("Start waiting...");


	// TODO: Read a specified file and send it to a client.
    //      Send as many packets as window size (speified by
    //      client) allows and assume each packet contains
    //      1000 Bytes of data.
	//		When a client receives a packet, it will send back
	//		ACK packet.
    //      When a server receives an ACK packet, it will send
    //      next packet if available.

    // TODO: Print out events during the transmission.
	//		Refer the assignment PPT for the formats.

	// TODO: Close the sockets
    close(socket_fd);
    return 0;
}
