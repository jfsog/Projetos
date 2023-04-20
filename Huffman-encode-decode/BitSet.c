/*
 *  Como implementar BitSet em C
 *  https://stackoverflow.com/questions/40726269/how-to-implement-a-bitset-in-c
 * */
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
const u_int64_t M_ULONG = (CHAR_BIT * sizeof(u_int64_t));
typedef struct {
  u_int64_t size;
  u_int64_t idx;
  u_int64_t *bits;
} BitSet;
void ClearBitSet(BitSet *set) {
  if (!set)
    return;
  free(set->bits);
  free(set);
}
BitSet *newBitSet() {
  BitSet *set = calloc(sizeof(BitSet), 1);
  set->size = 1;
  set->bits = calloc(sizeof(u_int64_t), 1);
  return set;
}
void BitsetPut(BitSet *set, u_int64_t bit) {
  u_int64_t i = bit / M_ULONG;
  if (i >= set->size) {
    u_int64_t n = set->size;
    set->size = i + 1UL;
    set->bits = realloc(set->bits, sizeof(u_int64_t) * set->size);
    if (!set->bits) {
      printf("BitSet não expandido por falta de memória\n");
      exit(7);
    }
    while (n < set->size)
      set->bits[n++] = 0UL;
  }
  set->bits[i] |= 1UL << (bit % M_ULONG);
}
bool BitsetGet(BitSet *set, u_int64_t bit) {
  u_int64_t i = bit / M_ULONG;
  if (i >= set->size)
    return false;
  return (set->bits[i] >> (bit % M_ULONG)) & 1UL;
}
