CC = gcc
INC = -I.
FLAGS = -W -Wall -g
TARGETS = server client

all: $(TARGETS)

server: server.o
	$(CC) $^ -o $@ -lrt

client: client.o
	$(CC) $^ -o $@ -lrt

server.o: server.c
	$(CC) -c $(FLAGS) $(INC) $< -o $@ -lrt

client.o: client.c
	$(CC) -c $(FLAGS) $(INC) $< -o $@ -lrt

.PHONY: clean
clean:
	-rm -f *.o $(TARGETS)
