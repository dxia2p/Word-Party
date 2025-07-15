#include "server_logic.h"
#include "server_networking.h"
#include "message_queue.h"
#include "thread_protocol_codes.h"
#include "../public/custom_protocol.h"

#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

int main() {
    struct message_queue *network_to_logic_queue = create_message_queue();
    struct message_queue *logic_to_network_queue = create_message_queue();
    struct message_queue_group networking_queue_group;
    struct message_queue_group logic_queue_group;

    networking_queue_group.read_q = logic_to_network_queue;
    networking_queue_group.write_q = network_to_logic_queue;
    logic_queue_group.read_q = network_to_logic_queue;
    logic_queue_group.write_q = logic_to_network_queue;

    bool game_running = true;
    game_init(&logic_queue_group);

    pthread_t networking_thread;
    if (pthread_create(&networking_thread, NULL, &networking_main_routine, &networking_queue_group) != 0) {
        fprintf(stderr, "networking thread could not be created.\n");
        return 1;
    }

    while (game_running) {
        // Do stuff
        game_update();
    }
    
    if (pthread_join(networking_thread, NULL) != 0) {
        fprintf(stderr, "networking thread could not be joined.\n");
        return 1;
    }

    // Clean up
    return 0;
}




