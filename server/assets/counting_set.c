/*
*NOTE: THIS SET IMPLEMENTATION IS NOT RESIZEABLE
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "counting_set.h"

unsigned long djb2_hash(char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

    return hash;
}


struct hash_node {
    char *val;
    int count;
    struct hash_node *next;
};

struct c_set {
    unsigned long size;
    unsigned long bucket_num;
    struct hash_node **buckets;
};


struct c_set *c_set_create(unsigned long bucket_num) {  // remember to make bucket_num 30% larger than number of expected elements
    struct c_set *s = calloc(1, sizeof(struct c_set));
    if (s == NULL) {
        fprintf(stderr, "calloc() failed for set in c_set_create()\n");
        return NULL;
    }

    s->buckets = calloc(bucket_num, sizeof(struct hash_node*));
    if (s->buckets == NULL) {
        fprintf(stderr, "calloc() failed for (%lu) buckets in c_set_create()\n", bucket_num);
        return NULL;
    }

    s->bucket_num = bucket_num;

    return s;
}


void c_set_insert(struct c_set *s, char *str) {
    unsigned long hash = djb2_hash(str);
    hash %= s->bucket_num;

    struct hash_node *hn = calloc(1, sizeof(struct hash_node));
    if (hn == NULL) {
        fprintf(stderr, "calloc() failed in set_insert()\n");
        return;
    }
    hn->val = strdup(str);
    if (hn->val == NULL) {
        fprintf(stderr, "strdup() failed in set_insert().\n");
        return;
    }
    hn->count = 1;

    if (s->buckets[hash] == NULL) {
        s->buckets[hash] = hn;
    } else {  // hash collision
        //  check if str already exists in the set
        struct hash_node *cur = s->buckets[hash];
        while (cur != NULL) {
            if (strcmp(cur->val, str) == 0) {  // str already exists!
                cur->count++;
                return;
            }
            cur = cur->next;
        }
        // At this point we know that str must not exist yet, so we add it to the front
        hn->next = s->buckets[hash];
        s->buckets[hash] = hn;
    }

    s->size++;
}

bool c_set_contains(struct c_set *s, char *str) {
    unsigned long hash = djb2_hash(str);
    hash %= s->bucket_num;

    struct hash_node *cur = s->buckets[hash];
    while (cur != NULL) {
        if (strcmp(cur->val, str) == 0) {
            return true;
        }
        cur = cur->next;
    }

    return false;
}

int c_set_get_count(struct c_set *s, char *str) {
    unsigned long hash = djb2_hash(str);
    hash %= s->bucket_num;

    struct hash_node *cur = s->buckets[hash];
    while(cur != NULL) {
        if (strcmp(cur->val, str) == 0) {
            return cur->count;
        }
        cur = cur->next;
    }
    return 0;
}

unsigned long c_set_get_size(struct c_set *s) {
    return s->size;
}
