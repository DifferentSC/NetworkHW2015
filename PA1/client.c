#include <stdio.h>
#include <pwd.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

#define BUFFER_SIZE 1000    // same as packet size
#define CLIENT_MESSAGE_SIZE 3

void handler();
timer_t set_timer(long long);

int socket_fd, next_timer_index, last_alive_timer_index, timer_list_size;
int dead_timer_count, total_timer_count;
timer_t *timer_list;

int main (int argc, char **argv) {

	// Check arguments 
    if (argc != 7) {
        printf("Usage: %s <hostname> <port> [Arguments]\n", argv[0]);
        printf("\tMandatory Arguments:\n");
        printf("\t\t-w\t\tsize of the window used at server\n");
        printf("\t\t-d\t\tACK delay in msec");
        exit(1);
    }
    // Set Handler for timers 

    signal(SIGALRM, handler);

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
    // initialize timer list (circular queue)
    next_timer_index = 0;
    last_alive_timer_index = 0;
    dead_timer_count = 0;
    total_timer_count = 0;
    timer_list_size = window_size * 4;
    timer_list = (timer_t *)malloc(timer_list_size * sizeof(timer_t));
    while(1){
        char cmdline[30];
        memset(cmdline, 0, 30);
        fgets(cmdline, 20, stdin);
        cmdline[strlen(cmdline) - 1] = '\0';
        if (strlen(cmdline) == 0)
            continue;
        if (!strcmp("C", cmdline)) {
            if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                printf("Connection failure!\n");
                continue;
            }
            is_connected = 1;
            printf("Connection established\n");
        } else if (!strcmp("F", cmdline)) {
            char client_message[CLIENT_MESSAGE_SIZE];
            memset(client_message, 0, CLIENT_MESSAGE_SIZE);
            client_message[0] = 'F';
            // Send terminating message
            write(socket_fd, client_message, CLIENT_MESSAGE_SIZE);
            // close the socket
            close(socket_fd);
            printf("Connection terminated\n");
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
            if (file_choice < 1 || file_choice > 3) {
                printf("Invalid file_choice! Should be between 1 and 3!\n");
                continue;
            }
            char client_message[CLIENT_MESSAGE_SIZE];
            memset(client_message, 0, CLIENT_MESSAGE_SIZE);
            client_message[0] = 'R';
            client_message[1] = file_choice - 1;
            client_message[2] = window_size;
            write(socket_fd, client_message, CLIENT_MESSAGE_SIZE);
            prev_byte = 0;
            cur_byte = 0;
            struct timeval tval_start, tval_end;
            gettimeofday(&tval_start, NULL);
            while(1) {
                char packet[BUFFER_SIZE];
                memset(packet, 0, BUFFER_SIZE);
                int packet_size = 0;
                while(packet_size < BUFFER_SIZE) {
                    int incr_packet_size = read(socket_fd, packet + packet_size, BUFFER_SIZE - packet_size);
                    packet_size += incr_packet_size;
                }
                prev_byte = cur_byte;
                // 1 byte is used for indication of the last packet
                cur_byte += packet_size - 1;
                if (prev_byte / (1024 * 1024) != cur_byte / (1024 * 1024)) {
                    printf("%d MB transmitted\n", cur_byte / (1024 * 1024));
                }
                // Packet[0] represents whether the packet is the last or not
                if (packet[0] == 1) {
                    printf("File transfer finished\n");
                    break;
                }
                // Set timer and put the timer to the circular queue
                timer_list[next_timer_index] = set_timer(ack_delay);
                next_timer_index = (next_timer_index + 1) % timer_list_size;
            }
            gettimeofday(&tval_end, NULL); 
            float msec = (tval_end.tv_sec - tval_start.tv_sec) * 1000 + (float)(tval_end.tv_usec - tval_start.tv_usec) / (float) 1000;
            printf("Transfer time: %.2lfms\n", msec);
        }
    }
    free(timer_list); 
    return 0;
}

/*
 * handler()
 * Invoked by a timer.
 * Send ACK to the server
 */
void handler() {
    char client_message[CLIENT_MESSAGE_SIZE];
    memset(client_message, 0, CLIENT_MESSAGE_SIZE);
    client_message[0] = 'A';
    write(socket_fd, client_message, CLIENT_MESSAGE_SIZE);
    // increase the number of out-of-date timers
    dead_timer_count++;
}

/*
 * set_timer()
 * set timer in msec
 */
timer_t set_timer(long long time) {
    struct itimerspec time_spec = {.it_interval = {.tv_sec=0,.tv_nsec=0},
    				.it_value = {.tv_sec=0,.tv_nsec=0}};

	int sec = time / 1000;
	long n_sec = (time % 1000) * 1000000;
    time_spec.it_value.tv_sec = sec;
    time_spec.it_value.tv_nsec = n_sec;

    timer_t t_id;
    if (timer_create(CLOCK_MONOTONIC, NULL, &t_id))
        perror("timer_create");
    if (timer_settime(t_id, 0, &time_spec, NULL))
        perror("timer_settime");

    // delete dead timers in circular queue
    for(; dead_timer_count != 0; dead_timer_count--) {
        timer_delete(timer_list[last_alive_timer_index]);
        total_timer_count--;
        last_alive_timer_index = (last_alive_timer_index + 1) % timer_list_size;
    }
    total_timer_count++;
    return t_id;
}

