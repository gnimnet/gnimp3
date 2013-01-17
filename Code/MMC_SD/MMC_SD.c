/******************************************************************************
Write by Ming	2009-3-3
******************************************************************************/
#include <avr/io.h>
#include "MMC_SD.h"

//**********MCU对SD的引脚设定**********//
#define CS_DDR		DDRC
#define CS_PORT		PORTC
#define CS			PC0

#define SCK_DDR		DDRB
#define SCK_PORT	PORTB
#define SCK			PB5

#define MOSI_DDR	DDRB
#define MOSI_PORT	PORTB
#define MOSI		PB3

#define MISO_DDR	DDRB
#define MISO_PORT	PORTB
#define MISO		PB4

#define SS_DDR		DDRB
#define SS			PB2

#define CS_LOW()	CS_PORT &= ~(1<<CS)	//Card Select
#define CS_HIGH()	CS_PORT |= 1<<CS	//Card Deselect

//**********内部函数**********//
void SPI_SetSpeedLow(){//设定SPI工作频率为1/128时钟频率(低速)
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1)|(1<<SPR0);
	SPSR &= ~(1<<SPI2X);
}

void SPI_SetSpeedHigh(){//设定SPI工作频率为1/2时钟频率(高速)
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR |= (1<<SPI2X);
}

BYTE SPI_ByteRW(BYTE data){//向SPI读写一个字节数据
	SPDR=data;
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

BYTE SD_SendCommand(BYTE cmd,DWORD arg){//向SD卡发送命令
	BYTE rtn,cnt=0;
	
	SPI_ByteRW(0xFF);
	CS_LOW();
	
	SPI_ByteRW(cmd | 0x40);//send command
	SPI_ByteRW(arg>>24);
	SPI_ByteRW(arg>>16);
	SPI_ByteRW(arg>>8);
	SPI_ByteRW(arg);
	SPI_ByteRW(0x95);
	
	while((rtn=SPI_ByteRW(0xFF))==0xFF)//等待SD应答
		if((cnt++)==0xFF)
			break;//超时

	CS_HIGH();
	SPI_ByteRW(0xFF);//额外8个时钟周期

	return rtn;//返回状态值
}

void Port_Init(){
	CS_PORT |= 1<<CS;
	CS_DDR |= 1<<CS;
	SCK_DDR |= 1<<SCK;
	MOSI_DDR |= 1<<MOSI;
	MISO_DDR &= ~(1<<MISO);
	SS_DDR |= 1<<SS;
}

BYTE ReadBegin(DWORD sector){//读数据前的初始化
	BYTE rtn;
	WORD cnt=0;
	SPI_SetSpeedHigh();//设定SPI到高速模式
	rtn = SD_SendCommand(17,sector<<9);//读命令
	if(rtn!=0x00)
		return 1;
	CS_LOW();
	//等数据的开始
	while(SPI_ByteRW(0xFF)!=0xFE){
		if((cnt++)==0xFFFF){
			CS_HIGH();
			return 2;//超时退出
		}
	}
	return 0;
}

void ReadEnd(){//读数据完成后的操作
	SPI_ByteRW(0xFF);//dummy crc
	SPI_ByteRW(0xFF);
	CS_HIGH();
	SPI_ByteRW(0xFF);//额外8个时钟周期
}

//**********接口函数**********//
BYTE SD_Init(){//SD卡初始化函数，成功返回0，失败则返回非0
	BYTE i;
	BYTE cnt = 0;
	BYTE rtn = 0;	
	Port_Init();//初始化引脚
	SPI_SetSpeedLow();//SPI初始化,设定为低速
	do{
		for(i=0;i<10;i++)
			SPI_ByteRW(0xFF);
		rtn=SD_SendCommand(0,0);//发idle命令
		if((cnt++)==0xFF)
			return 1;//超时退出
	}while(rtn!=0x01);
	cnt=0;//清零重试计数器
	do{
		rtn=SD_SendCommand(1,0);//发active命令
		if((cnt++)==0xFF)
			return 2;//超时退出
	}while(rtn);
	SPI_SetSpeedHigh();//设定SPI到高速模式
	SD_SendCommand(59,0);//禁用CRC
	SD_SendCommand(16,512);//设定扇区大小为512

	return 0;//初始化成功
}

BYTE SD_ReadBytes(DWORD sector,DWORD offset,BYTE* buf,WORD num){//读取若干字节
	WORD SecReadCnt=0;
	while(offset>=512){//计算偏移跳过的扇区
		offset-=512;
		sector++;
	}
	if(ReadBegin(sector++)!=0)
		return 1;//进入读状态失败
	while(offset--){//跳过offset个字节
		SecReadCnt++;
		SPI_ByteRW(0xFF);
	}
	while(num){//还有数据要读
		while(num>0 && SecReadCnt<512){//还有数据要读并且本扇区未读完
			*(buf++)=SPI_ByteRW(0xFF);
			SecReadCnt++;
			num--;
		}
		if(num==0){//读完毕
			break;
		}
		else{//数据未读完，本扇区读结束
			ReadEnd();
			if(ReadBegin(sector++)!=0)
				return 1;//进入下一扇区读状态失败
			SecReadCnt=0;//本扇区字节计数清零
		}
	}
	while((SecReadCnt++)<512){//跳过本扇区最后的字节
		SPI_ByteRW(0xFF);
	}
	ReadEnd();
	return 0;
}

BYTE SD_ReadOneSector(DWORD sector,BYTE* buf){//读一个扇区(buf为512字节)，成功返回0
	WORD i;
	if(ReadBegin(sector++)!=0)
		return 1;//进入读状态失败
	for(i=0;i<512;i++){//读512个数据
		*(buf++)=SPI_ByteRW(0xFF);
	}
	ReadEnd();//退出读状态
	return 0;
}
