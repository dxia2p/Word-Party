#ifndef SET_H
#define SET_H
#include <stdbool.h>

unsigned long djb2_hash(char *str);

struct hash_node;
struct set;
struct set *set_create(unsigned long bucket_num);
void set_insert(struct set *s, char *str);
bool set_contains(struct set *s, char *str);
unsigned long set_get_size(struct set *s);

#endif
