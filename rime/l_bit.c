#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "l_bit.h"



void init_max_hash(sha1_hash_t *hash) {
  int i;
  for(i=0;i<20;i++){
    hash->hash[i] = 0xff;
  }
}

void init_min_hash(sha1_hash_t *hash) {
  int i;
  for(i=0;i<20;i++){
    hash->hash[i] = hash->hash[i] & 0x00;
  }
}

/*
 * c = a + b
 */
void sha1Add(sha1_hash_t *a, sha1_hash_t *b, sha1_hash_t *c) {
  c->hash[0]=(a->hash[0] & 0xFF)+(b->hash[0] & 0xFF);
  int i;
	  for(i=1;i<DEFAULT_HASH_SIZE;i++){
		  c->hash[i]=(a->hash[i] & 0xFF)+(b->hash[i] & 0xFF)+(c->hash[i-1]>>8);
    }
}

/*
 * c = a - b
 */
void sha1Sub(sha1_hash_t *a, sha1_hash_t *b, sha1_hash_t *c) {
  //2の補数をとる
  sha1_hash_t buf;
	buf.hash[0]=((~b->hash[0]) & 0xFF)+0x01;
  int i;
	for(i=1;i<DEFAULT_HASH_SIZE;i++){
		buf.hash[i]=((~b->hash[i]) & 0xFF)+(buf.hash[i-1]>>8);
	}
	//足し算をする
	sha1Add(a, &buf, c);
}

/*
 * dst <= src copy
 */
void sha1Copy(sha1_hash_t *dst, sha1_hash_t *src) {
  int i;
  if(dst){
    for(i=0;i<DEFAULT_HASH_SIZE;i++){
      dst->hash[i]=src->hash[i];
    }
  }else{
		for(i=0;i<DEFAULT_HASH_SIZE;i++){
			dst->hash[i]=0;
		}
	}
}

/*
 * 左シフト(1bit)
 */
void sha1LShift(sha1_hash_t *dst) {
  int i;
  dst->hash[0]=dst->hash[0]<<1;
  for(i=1;i<DEFAULT_HASH_SIZE;i++){
		dst->hash[i]=(dst->hash[i]<<1)|(dst->hash[i-1]>>8 & 0x01);
	}
}

/*
 * 右シフト(1bit)
 */
void sha1RShift(sha1_hash_t *dst) {
  int i;
  for(i=0;i<DEFAULT_HASH_SIZE-1;i++){
		dst->hash[i]=(dst->hash[i]>>1)|((dst->hash[i+1]<<7) & 0xFF);
	}
	dst->hash[DEFAULT_HASH_SIZE-1]=dst->hash[DEFAULT_HASH_SIZE-1]>>1;
}

/*
 * 比較
 * a >= b return 0
 * a < b return 1
 */
int sha1Comp(sha1_hash_t *a, sha1_hash_t *b) {
  int i;
  for(i=DEFAULT_HASH_SIZE;i>=0;i--){
		if((a->hash[i]&0xFF)<(b->hash[i]&0xFF)) return 1;
		if((a->hash[i]&0xFF)>(b->hash[i]&0xFF)) return 0;
	}
	return 0;
}

/*
 * c = a * b
 */
void sha1Mul(sha1_hash_t *a, sha1_hash_t *b, sha1_hash_t *c) {
  int i;
  for(i=0;i<DEFAULT_HASH_SIZE*8;i++){
    if(a->hash[0] & 0x01){
			add(b,c,c);
		}
    sha1RShift(a);
		sha1LShift(b);
  }
}

/*
 * c = a / b
 * a = 余り
 */
void sha1Div(sha1_hash_t *a, sha1_hash_t *b, sha1_hash_t *c) {
  //配列を複製する
  sha1_hash_t buf1;
  sha1_hash_t buf2;
	copy(&buf1, b);
	//法を左端に移動する
	int shift=1;
  int i;
	for(i=0;i<DEFAULT_HASH_SIZE*8;i++){
		sha1LShift(&buf1);
		shift++;
		if(buf1.hash[DEFAULT_HASH_SIZE-1] & 0x80) break;
	}
	//被除数から法を引いてゆく処理と計算結果を作成する処理
	while(shift--){
		sha1LShift(c);
		if(!sha1Comp(a, &buf1)){
			sha1Sub(a, &buf1, a);
			c->hash[0]=c->hash[0] | 0x01;
		}
		sha1RShift(&buf1);
	}

}
