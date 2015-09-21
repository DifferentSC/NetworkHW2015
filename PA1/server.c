#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFFER_SIZE	1000 	// same as packet size

int main (int argc, char **argv) {

	// Check arguments
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }
	
    // TODO: Create sockets for a server and a client
    //      bind, listen, and accept

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
    return 0;
}
