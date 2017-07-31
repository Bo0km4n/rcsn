#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <string.h>

char *sha1_to_hex(const unsigned char *sha1) {
    static int bufno;
    static char hexbuffer[4][50];
    static const char hex[] = "0123456789abcdef";
    char *buffer = hexbuffer[3 & ++bufno], *buf = buffer;
    int i;

    for (i = 0; i < 20; i++) {
	unsigned int val = *sha1++;
	*buf++ = hex[val >> 4];
	*buf++ = hex[val & 0xf];
    }
    *buf = '\0';

    return buffer;
}

void calc_sha1(const char *body) {
    char *type = "blob";
    int hdrlen;
    char hdr[256];
    unsigned char sha1[41];
    unsigned long len;
    SHA_CTX c;

    len = strlen(body);

    sprintf(hdr, "%s %ld", type, len);
    hdrlen = strlen(hdr) + 1;

    SHA1_Init(&c);
    SHA1_Update(&c, hdr, hdrlen);
    SHA1_Update(&c, body, len);
    SHA1_Final(sha1, &c);

    printf("%s\n", sha1_to_hex(sha1));
}


int main(void) {
    char *body = "hello\n";
    calc_sha1(body);
    return 0;
}
