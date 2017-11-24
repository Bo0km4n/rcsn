#include <stdio.h>

//足し算 c=a+b
void add(unsigned short*a,unsigned short*b,unsigned short*c,int size){
	c[0]=(a[0] & 0xFF)+(b[0] & 0xFF);
	for(int i=1;i<size;i++){
		c[i]=(a[i] & 0xFF)+(b[i] & 0xFF)+(c[i-1]>>8);
	}
}

//引き算 c=a-b  bufは計算処理に必要なバッファ
void sub(unsigned short*a,unsigned short*b,unsigned short*c,int size
		 ,unsigned short*buf){
  for(i=size-1;i>=0;i--) {
    printf("%02x ", (unsigned short)(a[i] & 0xFF));
  }
  printf("|| ");
  for(i=size-1;i>=0;i--) {
    printf("%02x ", (unsigned short)(b[i] & 0xFF));
  }
  printf("|| ");
  for(i=size-1;i>=0;i--) {
    printf("%02x ", (unsigned short)(c[i] & 0xFF));
  }
  printf("\n");
	//2の補数をとる
	buf[0]=((~b[0]) & 0xFF)+0x01;
	for(int i=1;i<size;i++){
		buf[i]=((~b[i]) & 0xFF)+(buf[i-1]>>8);
	}
	//足し算をする
	add(a,buf,c,size);
}

//配列のコピーもしくは　in がNULLの場合クリア
void copy(unsigned short*in,unsigned short*out,int size){
	if(in){
		for(int i=0;i<size;i++){
			out[i]=in[i];
		}
	}else{
		for(int i=0;i<size;i++){
			out[i]=0;
		}
	}
}

//左シフト
void left_shift(unsigned short*a,int size){
	a[0]=a[0]<<1;
	for(int i=1;i<size;i++){
		a[i]=(a[i]<<1)|(a[i-1]>>8 & 0x01);
	}
}

//右シフト
void right_shift(unsigned short*a,int size){
	for(int i=0;i<size-1;i++){
		a[i]=(a[i]>>1)|((a[i+1]<<7) & 0xFF);
	}
	a[size-1]=a[size-1]>>1;
}

//掛け算 c=a*b  a,b の値は計算により書き換えられます
void mul(unsigned short*a,unsigned short*b,unsigned short*c,int size){
	for(int i=0;i<size*8;i++){
		if(a[0] & 0x01){
			add(b,c,c,size);
		}
		right_shift(a,size);
		left_shift(b,size);
	}
}

//比較 a<b と同等
int cmp(unsigned short*a,unsigned short*b,int size){
	for(int i=size-1;i>=0;i--){
		if((a[i]&0xFF)<(b[i]&0xFF)) return 1;
		if((a[i]&0xFF)>(b[i]&0xFF)) return 0;
	}
	return 0;
}

//割り算 a:被除数 b:法 c:計算結果 a:余り size:配列のサイズ buf1,buf2:バッファ 
//a の値は計算により書き換えられます
void div(unsigned short*a,unsigned short*b,unsigned short*c,int size
		 ,unsigned short*buf1,unsigned short*buf2){
	//配列を複製する
  int j;
	copy(b,buf1,size);
	//法を左端に移動する
	int shift=1;
	for(int i=0;i<size*8;i++){
		left_shift(buf1,size);
		shift++;
		if(buf1[size-1] & 0x80) break;
	}
	//被除数から法を引いてゆく処理と計算結果を作成する処理
	while(shift--){
		left_shift(c,size);
		if(!cmp(a,buf1,size)){
			sub(a,buf1,a,size,buf2);
      for(j=size-1;j>=0;j--) {
        printf("%02x ", (unsigned char)(a[j] & 0xFF));
      }
      printf("\n");
			c[0]=c[0] | 0x01;
		}
		right_shift(buf1,size);
	}
}

int main(){
	//変数
  int i;
	unsigned short a[4],b[4],c[4];
	//計算に必要なバッファ
	unsigned short buf1[4],buf2[4];

	//初期値の設定
	a[3]=0x12;a[2]=0x34;a[1]=0x56;a[0]=0x78;
	b[3]=0x00;b[2]=0x00;b[1]=0x9A;b[0]=0xBC;
	//配列 c の初期化
	copy(NULL,c,(sizeof(a)/sizeof(a[0])));
	
	//割り算の実行
	div(a,b,c,(sizeof(a)/sizeof(a[0])),buf1,buf2);

	//表示
	for(i=(sizeof(a)/sizeof(a[0]))-1;i>=0;i--){
		printf("%02x ",(unsigned char)(c[i] & 0xFF));
	}
	printf("\n");
	for(i=(sizeof(a)/sizeof(a[0]))-1;i>=0;i--){
		printf("%02x ",(unsigned char)(a[i] & 0xFF));
	}
	printf("\n");
	return 0;
}
