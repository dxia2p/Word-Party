#ifndef SERVER_LOGIC_H
#define SERVER_LOGIC_H

#include "message_queue.h"

void game_init(struct message_queue_group *queue_group);
void game_update(double deltatime);

#endif
