#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <signal.h>

#define BUFFER_SIZE 1000    // same as packet size
#define CLIENT_MESSAGE_SIZE 3

void handler();
timer_t set_timer(long long);

int socket_fd;

int main (int argc, char **argv) {

	// Check arguments 
    if (argc != 7) {
        printf("Usage: %s <hostname> <port> [Arguments]\n", argv[0]);
        printf("\tMandatory Arguments:\n");
        printf("\t\t-w\t\tsize of the window used at server\n");
        printf("\t\t-d\t\tACK delay in msec");
        exit(1);
    }

    char server_hostname[100];
    strcpy(server_hostname, argv[1]);
    int server_port = atoi(argv[2]);
    int window_size, ack_delay;
    if (!strcmp("-w", argv[3]) && !strcmp("-d", argv[5])) {
        window_size = atoi(argv[4]);
        ack_delay = atoi(argv[6]);
    } else if (!strcmp("-d", argv[3]) && !strcmp("-w", argv[5])) {
        window_size = atoi(argv[6]);
        ack_delay = atoi(argv[4]);
    } else {
        printf("Invalid argument option!\n");
        exit(1);
    }

    // Set Handler for timers
    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGALRM);
    sigact.sa_handler = &handler;
    sigaction(SIGALRM, &sigact, NULL);

    if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Opening socket failed!\n");
        exit(1);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_hostname);
    server_addr.sin_port = htons(server_port);

    int is_connected = 0;
    int prev_byte, cur_byte;
    while(1){
        char cmdline[10];
        scanf("%s", cmdline);
        if (strlen(cmdline) == 0)
            continue;
        if (!strcmp("C", cmdline)) {
            if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                printf("Connection failure!\n");
                continue;
            }
        } else if (!strcmp("F", cmdline)) {
            char client_message[CLIENT_MESSAGE_SIZE] = {'\0', };
            client_message[0] = 'F';
            write(socket_fd, client_message, CLIENT_MESSAGE_SIZE);
            close(socket_fd);
            break;
        } else if (cmdline[0] == 'R') {
            if (!is_connected) {
                printf("Request cannot be made before making connection!\n");
                continue;
            }
            int file_choice;
            if(sscanf(cmdline, "R %d", &file_choice) < 1) {
                printf("Cannot read file_choice from R command!\n");
                continue;
            }
            if (file_choice < 0 && file_choice > 2) {
                printf("Invalid file_choice! Should be between 0 and 2!\n");
                continue;
            }
            char client_message[CLIENT_MESSAGE_SIZE];
            client_message[0] = 'R';
            client_message[1] = file_choice;
            client_message[2] = window_size;
            write(socket_fd, client_message, CLIENT_MESSAGE_SIZE);
            prev_byte = 0;
            cur_byte = 0;
            while(1) {
                char packet[BUFFER_SIZE];
                read(socket_fd, packet, BUFFER_SIZE);
                int data_size = packet[0] * 256 + packet[1];
                prev_byte = cur_byte;
                cur_byte += data_size;
                if (prev_byte % (1024 * 1024) != cur_byte % (1024 * 1024)) {
                    printf("%d MB transmitted\n", cur_byte % (1024 * 1024));
                }
                if (data_size < BUFFER_SIZE - 2) {
                    printf("File transfer finished\n");
                    break;
                }
                set_timer(ack_delay);
            }
        }
    }

    // TODO: Create a socket for a client
    //      connect to a server
    //      set ACK delay
    //      set server window size
    //      specify a file to receive
    //      finish the connection

    // TODO: Receive a packet from server
    //      set timer for ACK delay
    //      send ACK packet back to server (usisng handler())
    //      You may use "ack_packet"

    // TODO: Close the socket
    return 0;
}

/*
 * handler()
 * Invoked by a timer.
 * Send ACK to the server
 */
void handler() {
    printf("Hi\n");
	// TODO: Send an ACK packet
    char client_message[CLIENT_MESSAGE_SIZE] = {'\0', };
    client_message[0] = 'A';
    write(socket_fd, client_message, 3);
}

/*
 * set_timer()
 * set timer in msec
 */
timer_t set_timer(long long time) {
    struct itimerspec time_spec = {.it_interval = {.tv_sec=0,.tv_nsec=0},
    				.it_value = {.tv_sec=0,.tv_nsec=0}};

	int sec = time / 1000;
	long n_sec = (time % 1000) * 1000;
    time_spec.it_value.tv_sec = sec;
    time_spec.it_value.tv_nsec = n_sec;

    timer_t t_id;
    if (timer_create(CLOCK_MONOTONIC, NULL, &t_id))
        perror("timer_create");
    if (timer_settime(t_id, 0, &time_spec, NULL))
        perror("timer_settime");

    return t_id;
}
