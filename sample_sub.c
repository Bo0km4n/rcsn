#include <stdio.h>
#include <stdlib.h>

int main(void) {
  uint8_t t1 = 0xfe;
  uint8_t t2 = 0xff;

  uint8_t res = t1 - t2;
  printf("[DEBUG] res = %02x\n", res);
  return 0;
}
