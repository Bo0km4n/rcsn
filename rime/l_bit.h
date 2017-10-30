#ifndef _L_BIT_
#define _L_BIT_

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_HASH_SIZE 20

typedef struct sha1_hash{
  unsigned short hash[DEFAULT_HASH_SIZE];
} sha1_hash_t;

/*
 * Function prototypes
 */
void increment(sha1_hash_t *);
void debug_print(sha1_hash_t *);
void sha1Add(sha1_hash_t *, sha1_hash_t *, sha1_hash_t *);
void sha1Sub(sha1_hash_t *, sha1_hash_t *, sha1_hash_t *);
void sha1Mul(sha1_hash_t *, sha1_hash_t *, sha1_hash_t *);
void sha1Div(sha1_hash_t *, sha1_hash_t *, sha1_hash_t *);
void sha1LShift(sha1_hash_t *);
void sha1RShift(sha1_hash_t *);
void sha1Copy(sha1_hash_t *, sha1_hash_t *);
int sha1Comp(sha1_hash_t *, sha1_hash_t *);

void init_max_hash(sha1_hash_t *);
void init_min_hash(sha1_hash_t *);
int compare_hash(sha1_hash_t *, sha1_hash_t *);
#endif
