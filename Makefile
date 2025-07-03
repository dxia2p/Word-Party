compiler := gcc

all: bin/server bin/client

bin/client: out/client.o out/custom_protocol.o
	$(compiler) out/client.o out/custom_protocol.o -o bin/client

bin/server: out/server.o out/custom_protocol.o
	$(compiler) out/server.o out/custom_protocol.o -o bin/server

out/server.o: server/server.c
	$(compiler) server/server.c -c -o out/server.o

out/client.o: client/client.c
	$(compiler) client/client.c -c -o out/client.o

out/custom_protocol.o: include/custom_protocol.c
	$(compiler) include/custom_protocol.c -c -o out/custom_protocol.o









clean:
	rm -f bin/*
	rm -f out/*
