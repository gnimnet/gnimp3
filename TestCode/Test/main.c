#include <avr/io.h>
#include <util/delay.h>

#define uchar unsigned char
#define uint unsigned int

#define LEDPORT	PORTD
#define LEDDDR	DDRD
#define LED1	PD0
#define LED2	PD1
#define LED3	PD2

#define KEYPIN	PIND
#define KEYDDR	DDRD
#define KEY1	PD3
#define KEY2	PD4
#define KEY3	PD5
#define KEY4	PD6
#define KEY5	PD7

uchar GetKey();

void MyDelay(uint ms){
	while(ms--)
		_delay_ms(1);
}

int main(){
	uchar tmp;
	LEDDDR|=(1<<LED1)|(1<<LED2)|(1<<LED3);//LED端口设为输出
	KEYDDR&=~((1<<KEY1)|(1<<KEY2)|(1<<KEY3)|(1<<KEY4)|(1<<KEY5));//按键端口设为输入
	while(1){
		LEDPORT|=(1<<LED1);
		LEDPORT|=(1<<LED2);
		LEDPORT|=(1<<LED3);
		MyDelay(500);
		LEDPORT&=~(1<<LED1);
		LEDPORT&=~(1<<LED2);
		LEDPORT&=~(1<<LED3);
		MyDelay(500);
		tmp=GetKey();
		if(tmp!=0){
			LEDPORT=tmp;
		}
	}
	return 0;
}

uchar GetKey(){
	if( (KEYPIN&(1<<KEY1))==0 ){
		return 1;
	}
	if( (KEYPIN&(1<<KEY2))==0 ){
		return 2;
	}
	if( (KEYPIN&(1<<KEY3))==0 ){
		return 3;
	}
	if( (KEYPIN&(1<<KEY4))==0 ){
		return 4;
	}
	if( (KEYPIN&(1<<KEY4))==0 ){
		return 5;
	}
	return 0;
}
