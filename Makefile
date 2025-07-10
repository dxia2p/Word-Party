compiler := gcc
public_objects = out/custom_protocol.o out/network_handler.o
server_objects = out/server.o out/set.o out/word_validator.o
client_objects = out/client.o

all: bin/server bin/client

bin/client: $(client_objects) $(public_objects)
	$(compiler) $^ -o $@ -Wall -g
$(client_objects): out/%.o: client/%.c
	$(compiler) -c $^ -o $@ -g

bin/server: $(server_objects) $(public_objects)
	$(compiler) $^ -o $@ -Wall -g
$(server_objects): out/%.o: server/%.c
	$(compiler) -c $^ -o $@ -g



$(public_objects): out/%.o: public/%.c
	$(compiler) -c $^ -o $@ -g


clean:
	rm -f bin/*
	rm -f out/*
