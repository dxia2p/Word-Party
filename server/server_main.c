#include "server_logic.h"
#include "server_networking.h"
#include "message_queue.h"

#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

int main( int argc, char *argv[]) {
    struct message_queue *network_to_logic_queue = create_message_queue();
    struct message_queue *logic_to_network_queue = create_message_queue();
    struct message_queue_group networking_queue_group;
    struct message_queue_group logic_queue_group;

    networking_queue_group.read_q = logic_to_network_queue;
    networking_queue_group.write_q = network_to_logic_queue;
    logic_queue_group.read_q = network_to_logic_queue;
    logic_queue_group.write_q = logic_to_network_queue;

    bool game_running = true;

    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        game_init(&logic_queue_group, true);
    } else {
        game_init(&logic_queue_group, false);
    }

    pthread_t networking_thread;
    if (pthread_create(&networking_thread, NULL, &networking_main_routine, &networking_queue_group) != 0) {
        fprintf(stderr, "networking thread could not be created.\n");
        return 1;
    }

    struct timespec prev_time;
    clock_gettime(CLOCK_MONOTONIC, &prev_time);
    double deltatime = 0;

    printf("Type \"start\" to start the game once 2 or more players have joined.\n");

    while (game_running) {
        struct timespec frame_begin_time;
        clock_gettime(CLOCK_MONOTONIC, &frame_begin_time);

        // Do stuff
        game_update(deltatime);


        struct timespec frame_end_time;
        clock_gettime(CLOCK_MONOTONIC, &frame_end_time);
        struct timespec wait_time;
        wait_time.tv_sec = frame_end_time.tv_sec - frame_begin_time.tv_sec;
        wait_time.tv_nsec = frame_end_time.tv_nsec - frame_begin_time.tv_nsec;
        if (wait_time.tv_sec > 0 && wait_time.tv_nsec < 0) {
            wait_time.tv_nsec += 1e9;
            wait_time.tv_sec--;
        }
        double target_frametime = 1.0/30.0;
        struct timespec remaining, requested = {.tv_sec = 0, .tv_nsec = target_frametime * 1e9 - wait_time.tv_nsec };
        nanosleep(&requested, &remaining);
        
        struct timespec cur_time;
        clock_gettime(CLOCK_MONOTONIC, &cur_time);
        struct timespec frame_time;
        frame_time.tv_sec = cur_time.tv_sec - prev_time.tv_sec;
        frame_time.tv_nsec = cur_time.tv_nsec - prev_time.tv_nsec;
        if (frame_time.tv_sec > 0 && frame_time.tv_nsec < 0) {
            frame_time.tv_nsec += 1e9;
            frame_time.tv_sec--;
        }
        deltatime = frame_time.tv_sec;
        deltatime += ((double)frame_time.tv_nsec) / 1e9;
        prev_time = cur_time;
        // printf("frame_time.tv_sec: %ld, frame_time.tv_nsec: %ld, deltatime %f\n", frame_time.tv_sec, frame_time.tv_nsec, deltatime);
    }
    
    if (pthread_join(networking_thread, NULL) != 0) {
        fprintf(stderr, "networking thread could not be joined.\n");
        return 1;
    }

    // Clean up
    return 0;
}




