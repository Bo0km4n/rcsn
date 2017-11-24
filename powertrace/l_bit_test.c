#include "l_bit.h"

void debugPrint(sha1_hash_t *t) {
  int i;
  for(i=DEFAULT_HASH_SIZE-1;i>=0;i--) {
    printf("%02x ", t->hash[i]);
  }
  printf("\n");
  return;
}

int main(void) {
  sha1_hash_t a;
  sha1_hash_t b;
  sha1_hash_t c;

  a.hash[0] = 0x78;
  a.hash[1] = 0x56;
  a.hash[2] = 0x34;
  a.hash[3] = 0x12;

  b.hash[0] = 0xF0;
  b.hash[1] = 0xDE;
  b.hash[2] = 0xBC;
  b.hash[3] = 0x9A;

  sha1Add(&a, &b, &c);
  debugPrint(&c);
  printf("=====================\n");

  a.hash[0] = 0xFF;
  a.hash[1] = 0xFF;
  a.hash[2] = 0xFF;
  a.hash[3] = 0xFF;

  b.hash[0] = 0x78;
  b.hash[1] = 0x56;
  b.hash[2] = 0x34;
  b.hash[3] = 0x12;

  sha1Sub(&a, &b, &c);
  debugPrint(&c);
  printf("=====================\n");

  a.hash[0] = 0x34;
  a.hash[1] = 0x12;
  a.hash[2] = 0x00;
  a.hash[3] = 0x00;

  b.hash[0] = 0x78;
  b.hash[1] = 0x56;
  b.hash[2] = 0x00;
  b.hash[3] = 0x00;

  c.hash[0] = 0x00;
  c.hash[1] = 0x00;
  c.hash[2] = 0x00;
  c.hash[3] = 0x00;

  sha1Mul(&a, &b, &c);
  debugPrint(&c);

  printf("=====================\n");

  a.hash[3] = 0x12;
  a.hash[2] = 0x34;
  a.hash[1] = 0x56;
  a.hash[0] = 0x78;

  b.hash[3] = 0x00;
  b.hash[2] = 0x00;
  b.hash[1] = 0x9A;
  b.hash[0] = 0xBC;

  c.hash[3] = 0x00;
  c.hash[2] = 0x00;
  c.hash[1] = 0x00;
  c.hash[0] = 0x00;

  sha1Div(&a, &b, &c);
  debugPrint(&c);


  return 0;
}
