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

void handler();
timer_t set_timer(long long);

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
    struct sigaction sigact;
    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGALRM);
    sigact.sa_handler = &handler;
    sigaction(SIGALRM, &sigact, NULL);

    // Timer example
    set_timer(1000);
    set_timer(2000);
    set_timer(3000);
    set_timer(4000);

    while(1){}

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
