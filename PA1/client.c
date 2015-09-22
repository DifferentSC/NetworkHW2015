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

struct timer_node {
    timer_t tid;
    struct timer_node *next;
    struct timer_node *prev;
};

struct timer_node *head, *tail;

void init_timer_list() {
    //initialize timer list
    head = (struct timer_node *)malloc(sizeof(struct timer_node));
    tail = (struct timer_node *)malloc(sizeof(struct timer_node));
    head->next = tail;
    head->prev = NULL;
    head->tid = NULL;
    tail->next = NULL;
    tail->prev = head;
    head->tid = NULL;
}

void add_new_timer(timer_t tid) {
    struct timer_node *new_timer_node;
    new_timer_node = (struct timer_node *)malloc(sizeof(struct timer_node));
    new_timer_node->next = tail;
    new_timer_node->prev = tail->prev;
    new_timer_node->tid = tid;
    
    tail->prev->next = new_timer_node;
    tail->prev = new_timer_node;
}

void destroy_oldest_timer() {
    struct timer_node *oldest_timer_node = head->next;
    if (head->next == tail)
        return;
    head->next = oldest_timer_node->next;
    oldest_timer_node->next->prev = head;
    timer_delete(oldest_timer_node->tid);
    free(oldest_timer_node);
}

void destroy_all_timer() {
    struct timer_node *timer_node;
    timer_node = head->next;
    while(timer_node != tail) {
        destroy_oldest_timer();
        timer_node = head->next;
    }
}

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
    init_timer_list();
    while(1){
        char cmdline[10];
        fgets(cmdline, 10, stdin);
        cmdline[strlen(cmdline) - 1] = '\0';
        if (strlen(cmdline) == 0)
            continue;
        if (!strcmp("C", cmdline)) {
            if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                printf("Connection failure!\n");
                continue;
            }
            is_connected = 1;
        } else if (!strcmp("F", cmdline)) {
            char client_message[CLIENT_MESSAGE_SIZE] = {'\0', };
            client_message[0] = 'F';
            send(socket_fd, client_message, CLIENT_MESSAGE_SIZE, 0);
            close(socket_fd);
            break;
        } else if (cmdline[0] == 'R') {
            if (!is_connected) {
                printf("%d\n", is_connected);
                printf("Request cannot be made before making connection!\n");
                continue;
            }
            int file_choice;
            if(sscanf(cmdline, "R %d", &file_choice) < 1) {
                printf("Cannot read file_choice from R command!\n");
                continue;
            }
            if (file_choice < 1 && file_choice > 3) {
                printf("Invalid file_choice! Should be between 1 and 3!\n");
                continue;
            }
            char client_message[CLIENT_MESSAGE_SIZE];
            client_message[0] = 'R';
            client_message[1] = file_choice - 1;
            client_message[2] = window_size;
            write(socket_fd, client_message, CLIENT_MESSAGE_SIZE);
            prev_byte = 0;
            cur_byte = 0;
            destroy_all_timer();
            while(1) {
                char packet[BUFFER_SIZE];
                int packet_size = recv(socket_fd, packet, BUFFER_SIZE, 0);
                if (packet_size <= 0)
                    continue;
                prev_byte = cur_byte;
                cur_byte += packet_size;
                if (prev_byte / (1024 * 1024) != cur_byte / (1024 * 1024)) {
                    printf("%d MB transmitted\n", cur_byte / (1024 * 1024));
                    fflush(stdout);
                }
                if (packet_size < BUFFER_SIZE) {
                    printf("File transfer finished\n");
                    fflush(stdout);
                    break;
                }
                add_new_timer(set_timer(ack_delay));
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
	// TODO: Send an ACK packet
    char client_message[CLIENT_MESSAGE_SIZE] = {'\0', };
    client_message[0] = 'A';
    send(socket_fd, client_message, CLIENT_MESSAGE_SIZE, 0);
    destroy_oldest_timer();
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
