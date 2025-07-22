#ifndef SET_H
#define SET_H
#include <stdbool.h>

unsigned long djb2_hash(char *str);

//struct hash_node;
struct c_set;
struct c_set *c_set_create(unsigned long bucket_num);
void c_set_insert(struct c_set *s, char *str);
bool c_set_contains(struct c_set *s, char *str);
int c_set_get_count(struct c_set *s, char *str);
unsigned long c_set_get_size(struct c_set *s);

#endif
