/*
*NOTE: THIS SET IMPLEMENTATION IS NOT RESIZEABLE
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

unsigned long djb2_hash(char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

    return hash;
}


struct hash_node {
    char *val;
    struct hash_node *next;
};

struct set {
    unsigned long size;
    unsigned long bucket_num;
    struct hash_node **buckets;
};


struct set *set_create(unsigned long bucket_num) {  // remember to make bucket_num 30% larger than number of expected elements
    struct set *s = calloc(1, sizeof(struct set));
    if (s == NULL) {
        fprintf(stderr, "calloc() failed for set int set_create()\n");
        return NULL;
    }

    s->buckets = calloc(bucket_num, sizeof(struct hash_node*));
    if (s->buckets == NULL) {
        fprintf(stderr, "calloc() failed for (%lu) buckets in set_create()\n", bucket_num);
        return NULL;
    }

    s->bucket_num = bucket_num;

    return s;
}


void set_insert(struct set *s, char *str) {
    unsigned long hash = djb2_hash(str);
    hash %= s->bucket_num;

    struct hash_node *hn = calloc(1, sizeof(struct hash_node));
    if (hn == NULL) {
        fprintf(stderr, "calloc() failed in set_insert()\n");
        return;
    }
    hn->val = strdup(str);

    if (s->buckets[hash] == NULL) {
        s->buckets[hash] = hn;
    } else {  // hash collision
        //  check if str already exists in the set
        struct hash_node *cur = s->buckets[hash];
        while (cur != NULL) {
            if (strcmp(cur->val, str) == 0) {  // str already exists!
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

bool set_contains(struct set *s, char *str) {
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

unsigned long set_get_size(struct set *s) {
    return s->size;
}
