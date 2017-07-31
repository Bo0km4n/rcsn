#include<stdio.h>
#include<stdint.h>
#include<string.h>
#include<openssl/md5.h>
#include<openssl/aes.h>
#include<openssl/blowfish.h>

char *encpass = "hoge";

void md5sum(char *data, unsigned char *digest)
{
  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, data, strlen(data));
  MD5_Final(digest, &ctx);
}

void dump(unsigned char *data, int len)
{
  int i;
  printf("str: ");
  for(i=0;i<len;i++){
    if(data[i] < 32 || data[i] > 126){
      printf(".");
    }else{
      printf("%c", data[i]);
    }
  }
  printf("\n");
  printf("hex: ");
  for(i=0;i<len;i++){
    printf("%02x ",data[i]);
  }
  printf("\n");
}
void aes_demo()
{
  char *data = "1234567890123456";
  char encdata[1024];
  char decdata[1024];
  AES_KEY enckey;
  AES_KEY deckey;
  uint8_t md5[16];
  md5sum(encpass, md5);
  AES_set_encrypt_key(md5, 128, &enckey);
  AES_set_decrypt_key(md5, 128, &deckey);

  printf("===== AES =====\n");
  printf("[data]\n");
  dump(data, 16);

  printf("\n[encrypt]\n");
  AES_encrypt(data, encdata, &enckey);
  dump(encdata, 16);

  printf("\n[decrypt]\n");
  AES_decrypt(encdata, decdata, &deckey);
  dump(decdata, 16);
}

void blowfish_demo()
{
  BF_LONG data = 0x34333231;
  BF_KEY key;
  BF_set_key(&key, 4, "hoge");

  printf("===== blowfish =====\n");
  printf("[data]\n");
  dump((unsigned char *)&data, sizeof(data));

  printf("\n[encrypt]\n");
  BF_encrypt(&data, &key);
  dump((unsigned char *)&data, sizeof(data));

  printf("\n[decrypt]\n");
  BF_decrypt(&data, &key);
  dump((unsigned char *)&data, sizeof(data));
}

int main(int argc, char *argv[])
{
  aes_demo();
  printf("\n");
  blowfish_demo();
  return(0);
}
