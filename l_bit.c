#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "l_bit.h"

// TODO sha1_hash_tはuint8_tの配列に戻して関数ないでunsigned shortに変換して計算する

void init_max_hash(sha1_hash_t *hash) {
  int i;
  for(i=0;i<DEFAULT_HASH_SIZE;i++){
    hash->hash[i] = 0xff;
  }
}

void init_min_hash(sha1_hash_t *hash) {
  int i;
  for(i=0;i<DEFAULT_HASH_SIZE;i++){
    hash->hash[i] = 0x00;
  }
}

/*
 * c = a + b
 */
void sha1Add(sha1_hash_t *a, sha1_hash_t *b, sha1_hash_t *c) {
  unsigned short aHash[DEFAULT_HASH_SIZE], bHash[DEFAULT_HASH_SIZE], cHash[DEFAULT_HASH_SIZE];
  int i;
  
  for(i=0;i<DEFAULT_HASH_SIZE;i++) {
    aHash[i] = a->hash[i];
    bHash[i] = b->hash[i];
    cHash[i] = c->hash[i];
  }
 
  cHash[0] = (aHash[0] & 0xFF)+(bHash[0] & 0xFF);

	for(i=1;i<DEFAULT_HASH_SIZE;i++) {
		cHash[i]=(aHash[i] & 0xFF)+(bHash[i] & 0xFF)+(cHash[i-1]>>8);
  }

  for(i=0;i<DEFAULT_HASH_SIZE;i++) {
    c->hash[i] = (uint8_t)(cHash[i] & 0xFF);
  }
}

/*
 * c = a - b
 */
void sha1Sub(sha1_hash_t *a, sha1_hash_t *b, sha1_hash_t *c) {
  //2の補数をとる
  unsigned short aHash[DEFAULT_HASH_SIZE], bHash[DEFAULT_HASH_SIZE], cHash[DEFAULT_HASH_SIZE];
  unsigned short buf[DEFAULT_HASH_SIZE];
  sha1_hash_t Hbuf;
  int i;
  for(i=0;i<DEFAULT_HASH_SIZE;i++) {
    aHash[i] = a->hash[i];
    bHash[i] = b->hash[i];
    cHash[i] = c->hash[i];
  }

	buf[0] = ((~bHash[0]) & 0xFF) + 0x01;
	for(i=1;i<DEFAULT_HASH_SIZE;i++){
		buf[i]=((~bHash[i]) & 0xFF) + (buf[i-1] >> 8);
	}
	//足し算をする
  for(i=0;i<DEFAULT_HASH_SIZE;i++) {
    a->hash[i] = (uint8_t)(aHash[i] & 0xFF);
    Hbuf.hash[i] = (uint8_t)(buf[i] & 0xFF);
    c->hash[i] = (uint8_t)(cHash[i] & 0xFF);
  }
	sha1Add(a, &Hbuf, c);
}

/*
 * dst <= src copy
 */
void sha1Copy(unsigned short *in, unsigned short *out) {
  int i;
  if(in) {
		for(i=0;i<DEFAULT_HASH_SIZE;i++) {
			out[i] = in[i];
		}
	}else{
		for(i=0;i<DEFAULT_HASH_SIZE;i++){
			out[i] = 0;
		}
	}
}

/*
 * 左シフト(1bit)
 */
void sha1LShift(unsigned short *dstHash) {
  int i;

  dstHash[0] = dstHash[0] << 1;
  for(i=1;i<DEFAULT_HASH_SIZE;i++){
		dstHash[i] = (dstHash[i] << 1) | (dstHash[i-1] >> 8 & 0x01);
	}
}

/*
 * 右シフト(1bit)
 */
void sha1RShift(unsigned short *dstHash) {
  int i;

  for(i=0;i<DEFAULT_HASH_SIZE-1;i++) {
		dstHash[i] = (dstHash[i] >> 1) | ((dstHash[i+1] << 7) & 0xFF);
	}
	dstHash[DEFAULT_HASH_SIZE-1] = dstHash[DEFAULT_HASH_SIZE-1] >> 1;
}

/*
 * 比較
 * a >= b return 0
 * a < b return 1
 */
int sha1Comp(sha1_hash_t *a, sha1_hash_t *b) {
  int i;
  unsigned short aHash[DEFAULT_HASH_SIZE], bHash[DEFAULT_HASH_SIZE];
  for(i=DEFAULT_HASH_SIZE-1;i>=0;i--) {
    aHash[i] = a->hash[i];
    bHash[i] = b->hash[i];
  }
  for(i=DEFAULT_HASH_SIZE;i>=0;i--){
		if((aHash[i] & 0xFF) < (bHash[i] & 0xFF)) return 1;
		if((aHash[i] & 0xFF) > (bHash[i] & 0xFF)) return 0;
	}
	return 0;
}

/*
 * c = a * b
 */
void sha1Mul(sha1_hash_t *a, sha1_hash_t *b, sha1_hash_t *c) {
  int i;
  int j;
  unsigned short aHash[DEFAULT_HASH_SIZE], bHash[DEFAULT_HASH_SIZE];
  
  for(i=0;i<DEFAULT_HASH_SIZE;i++) {
    aHash[i] = a->hash[i];
    bHash[i] = b->hash[i];
  }

  for(i=0;i<DEFAULT_HASH_SIZE*8;i++) {
    if(aHash[0] & 0x01) {
      sha1Add(b, c, c);
		}
    sha1RShift(aHash);
		sha1LShift(bHash);
    for(j=0;j<DEFAULT_HASH_SIZE;j++) {
      b->hash[j] = bHash[j];
    }
  }

}

void divAdd(unsigned short*a,unsigned short*b,unsigned short*c) {
  int i;
  c[0]=(a[0] & 0xFF)+(b[0] & 0xFF);
	for(i=1;i<DEFAULT_HASH_SIZE;i++){
		c[i]=(a[i] & 0xFF)+(b[i] & 0xFF)+(c[i-1]>>8);
	}

}

void divSub(unsigned short *a, unsigned short *b, unsigned short *c, unsigned short *buf) {
  int i;

  buf[0] = ((~b[0]) & 0xFF) + 0x01;
	for(i=1;i<DEFAULT_HASH_SIZE;i++) {
		buf[i] = ((~b[i]) & 0xFF) + (buf[i-1] >> 8);
	}
	
  divAdd(a, buf, c);
}

int divComp(unsigned short *a, unsigned short *b) {
  int i;
  for(i=DEFAULT_HASH_SIZE-1;i>=0;i--){
		if((a[i] & 0xFF) < (b[i] & 0xFF)) return 1;
		if((a[i] & 0xFF) > (b[i] & 0xFF)) return 0;
	}
	return 0;
}

/*
 * c = a / b
 * a = 余り
 */
void sha1Div(sha1_hash_t *a, sha1_hash_t *b, sha1_hash_t *c) {
  int i;
  unsigned short aHash[DEFAULT_HASH_SIZE], bHash[DEFAULT_HASH_SIZE], cHash[DEFAULT_HASH_SIZE];
  unsigned short buf1[DEFAULT_HASH_SIZE], buf2[DEFAULT_HASH_SIZE];
  
  for(i=0;i<DEFAULT_HASH_SIZE;i++) {
    aHash[i] = a->hash[i];
    bHash[i] = b->hash[i];
    buf1[i] = bHash[i];
    cHash[i] = c->hash[i];
  } 

  int shift = 1;

	for(i=0;i<DEFAULT_HASH_SIZE*8;i++) {
		sha1LShift(buf1);
		shift++;
		if(buf1[DEFAULT_HASH_SIZE-1] & 0x80) break;
	}
  while(shift--) {
		sha1LShift(cHash);
		if(!divComp(aHash, buf1)) {
			divSub(aHash, buf1, aHash, buf2);
      cHash[0] = cHash[0] | 0x01;
		}
		sha1RShift(buf1);
	}

  for(i=0;i<DEFAULT_HASH_SIZE;i++) {
    c->hash[i] = (uint8_t)(cHash[i] & 0xFF);
  }
}


