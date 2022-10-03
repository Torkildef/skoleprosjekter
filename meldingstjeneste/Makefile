flags = -g -Wall -Wextra -std=gnu11
compileInfo = gcc $(flags)

valgrindInfo = valgrind --track-origins=yes --malloc-fill=0x40 --free-fill=0x23 --leak-check=full --show-leak-kinds=all

BIN = upush_server upush_client

all: $(BIN)

runserver: upush_server
	./upush_server 2001 0

runclient: upush_client
	./upush_client Alice 127.0.0.1 2001 5 0

runclient2: upush_client
	./upush_client Bob 127.0.0.1 2001 5 0

vrs: upush_server
	$(valgrindInfo) ./upush_server 2001 5

vrc: upush_client
	$(valgrindInfo) ./upush_client Bob 127.0.0.1 2001 5 5

upush_client: send_packet.c client.o common.o common.h
	$(compileInfo) send_packet.c client.o common.o -o upush_client

upush_server: send_packet.c server.o common.o common.h
	$(compileInfo) send_packet.c server.o common.o -o upush_server 

client.o: client.c
	$(compileInfo) -c client.c

server.o: server.c
	$(compileInfo) -c server.c

common.o: common.c
	$(compileInfo) -c common.c

clean:
	rm -f $(BIN) *.o

