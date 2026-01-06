compiler := gcc
public_objects = out/custom_protocol.o out/network_handler.o
server_objects = out/set.o out/word_validator.o out/server_main.o out/server_networking.o out/server_logic.o out/message_queue.o
client_objects = out/client.o

all: bin/server bin/client

bin/client: $(client_objects) $(public_objects)
	$(compiler) $^ -o $@ -Wall -g -Wall
$(client_objects): out/%.o: client/%.c public/*.h
	$(compiler) -c $(filter %.c,$^) -o $@ -g -Wall

bin/server: $(server_objects) $(public_objects)
	$(compiler) $^ -o $@ -Wall -g -Wall
$(server_objects): out/%.o: server/%.c public/*.h server/*.h
	$(compiler) -c $(filter %.c,$^) -o $@ -g -Wall



$(public_objects): out/%.o: public/%.c public/*.h
	$(compiler) -c $(filter %.c,$^) -o $@ -g -Wall


clean:
	rm -f bin/*
	rm -f out/*
