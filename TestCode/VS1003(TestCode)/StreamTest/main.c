#include<avr/io.h>
#include"VS1003B.h"
#include"data.h"

#define DDR_SPI		DDRB
#define DD_MOSI		PB3
#define DD_SCK		PB5
#define DD_SS		PB2

#define uchar unsigned char
#define uint  unsigned int

void SPI_MasterInit(){
	/* 设置MOSI 和SCK、SS 为输出，其他为输入 */
	DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK)|(1<<DD_SS);
	/* 使能SPI 主机模式，设置时钟速率为fck/16 */
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
	SPSR &= ~(1<<SPI2X);
}

void VS1003B_SPI_Low(void){
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1)|(1<<SPR0);
	SPSR &= ~(1<<SPI2X);
}

void VS1003B_SPI_High(void){
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR |= (1<<SPI2X);
}

void VS1003B_Delay(uint n){
	while(n--);
}

uchar VS1003B_WriteByte(uchar cData){   
	SPDR = cData;
	while(!(SPSR & (1<<SPIF)))
		;
	return SPDR;
}

void VS1003B_WriteCMD(uchar addr,uint dat){
	VS1003B_XDCS_H();
	VS1003B_XCS_L();
	VS1003B_WriteByte(0x02);
	VS1003B_WriteByte(addr);
	VS1003B_WriteByte(dat>>8);
	VS1003B_WriteByte(dat);
	VS1003B_XCS_H();
}


uint VS1003B_ReadCMD(uchar addr){
	unsigned int temp;
	VS1003B_XDCS_H();
	VS1003B_XCS_L();
	VS1003B_WriteByte(0x03);
	VS1003B_WriteByte(addr);
	temp = VS1003B_WriteByte(0xFF);
	temp <<= 8;
	temp += VS1003B_WriteByte(0xFF);
	VS1003B_XCS_H();
	return temp;
}

void VS1003B_WriteDAT(uchar dat){
	VS1003B_XDCS_L();
	VS1003B_Delay(1);
	VS1003B_WriteByte(dat);
	VS1003B_XDCS_H();
	VS1003B_XCS_H();
}


void VS1003B_Init()
{
	uint temp;
	PORT_INI();
	DDRB |= _BV(0)|_BV(1)|_BV(2);
	VS1003B_Delay(50000);VS1003B_Delay(50000);VS1003B_Delay(50000);
	VS1003B_XCS_H();
	VS1003B_XDCS_H();
	VS1003B_XRESET_H();
	VS1003B_SPI_Low();
	VS1003B_Delay(50000);
	VS1003B_Delay(50000);
	VS1003B_WriteCMD(0x00,0x0800);
	temp=VS1003B_ReadCMD(0x00);
	VS1003B_WriteCMD(0x03,0x0800);
	temp=VS1003B_ReadCMD(0x03);
	VS1003B_WriteCMD(0x0b,0x2020);
	temp=VS1003B_ReadCMD(0x0b);
	VS1003B_SPI_High();
}


int main(){
	uint cnt;
	uchar i;
	SPI_MasterInit();
	VS1003B_Init();
	while(1){
		while(1){
			cnt=0;
			while(cnt<4800){
				if(VS1003B_PIN & _BV(VS1003B_DREQ)){
					for(i=0;i<32;i++){
						VS1003B_WriteDAT(pgm_read_byte(&vsBeepMP3[cnt]));
						cnt++;
					}
					if(cnt==4799)
						break;
				}
			}
		}
	}	
}
