/*********************************************************
Write By Ming	2009-3-23
*********************************************************/
#include<avr/io.h>
#include<util/delay.h>
#include"VS1003B.h"

//**********VS1003B寄存器地址声明**********//
#define MODE 		0x00	//模式控制
#define STATUS		0x01	//VS1003状态
#define BASS		0x02	//内置低音/高音增强器
#define CLOCKF		0x03	//时钟频率+倍频数
#define DECODE_TIME	0x04	//每秒解码次数
#define AUDATA		0x05	//Misc.音频数据
#define WRAM		0x06	//RAM 写/读
#define WRAMADDR	0x07	//RAM 写/读基址
#define HDAT0		0x08	//流头数据0
#define HDAT1		0x09	//流头数据1
#define AIADDR		0x0A	//用户代码起始地址
#define VOL			0x0B	//音量控制
#define AICTRL0		0x0C	//应用控制寄存器0
#define AICTRL1		0x0D	//应用控制寄存器1
#define AICTRL2		0x0E	//应用控制寄存器2
#define AICTRL3		0x0F	//应用控制寄存器3

//**********VS1003B寄存器值声明**********//
#define TREMBLE_VALUE	8		/* 0~15, 8 means off  */
#define TREMBLE_LOW_FS	8		/* 0~15, 0Hz-15KHz, lower frequency of tremble enhancement */
#define BASS_VALUE		0		/* 0~15, 0 means off  */
#define BASS_HIGH_FS	8		/* 2~15, up limit frequency of bass enhancement */
#define DEFAULT_BASS_TREMBLE ((TREMBLE_VALUE<<12)|(TREMBLE_LOW_FS<<8)|(BASS_VALUE<<4)|(BASS_HIGH_FS))

#define CLOCK_REG		0xE000	//PLL设定值，XTALI * 4.5
#define DEFAULT_VOLUME	0x2828	//默认音量值
#define MUTE_VOLUME		0xFEFE	//静音值

//**********MCU对VS1003B的引脚设定**********//
#define XCS_DDR		DDRC
#define XCS_PORT	PORTC
#define XCS			PC1

#define XRESET_DDR  DDRC
#define XRESET_PORT PORTC
#define XRESET		PC4

#define DREQ_DDR	DDRC
#define DREQ_PORT	PORTC
#define DREQ_PIN	PINC
#define DREQ		PC3

#define XDCS_DDR	DDRC
#define XDCS_PORT	PORTC
#define XDCS		PC2

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

#define XCS_H()		XCS_PORT |= (1<<XCS)
#define XCS_L()		XCS_PORT &= ~(1<<XCS)

#define XRESET_H()	XRESET_PORT |= (1<<XRESET)
#define XRESET_L()	XRESET_PORT &= ~(1<<XRESET)

#define XDCS_H()    XDCS_PORT |=  (1<<XDCS)
#define XDCS_L()    XDCS_PORT &= ~(1<<XDCS)

//**********内部函数**********//
void VS1003B_SPI_SetSpeedLow(){//设定SPI工作频率为1/128时钟频率(低速)
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1)|(1<<SPR0);
	SPSR &= ~(1<<SPI2X);
}

void VS1003B_SPI_SetSpeedHigh(){//设定SPI工作频率为1/2时钟频率(高速)
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR |= (1<<SPI2X);
}

uchar VS1003B_ByteRW(uchar data){//写一字节数据，并把交换数据读出
	SPDR=data;
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

void VS1003B_Port_Init(){//初始化端口函数
	XCS_PORT |= (1<<XCS);
	XRESET_PORT |= (1<<XRESET);
	DREQ_PORT |= (1<<DREQ);
	XDCS_PORT |= (1<<XDCS);

	XCS_DDR |= (1<<XCS);
	XRESET_DDR |= (1<<XRESET);
	DREQ_DDR &= ~(1<<DREQ);
	XDCS_DDR |= (1<<XDCS);
	SCK_DDR |= (1<<SCK);
	MOSI_DDR |= (1<<MOSI);
	MISO_DDR &= ~(1<<MISO);
	SS_DDR |= 1<<SS;
}

//**********接口函数**********//
void VS1003B_WriteCMD(uchar addr,uint dat){//向VS1003B写寄存器
	XDCS_H();
	XCS_L();
	VS1003B_ByteRW(0x02);
	VS1003B_ByteRW(addr);
	VS1003B_ByteRW(dat>>8);
	VS1003B_ByteRW(dat);
	XCS_H();
}

uint VS1003B_ReadCMD(uchar addr){//读VS1003B寄存器
	uint rtn;
	XDCS_H();
	XCS_L();
	VS1003B_ByteRW(0x03);
	VS1003B_ByteRW(addr);
	rtn = VS1003B_ByteRW(0xFF);
	rtn <<= 8;
	rtn += VS1003B_ByteRW(0xFF);
	XCS_H();
	return rtn;
}

void VS1003B_Fill2048Zero(){//给VS1003B发送2048个数据0
	unsigned char i,j;
	for(i=0;i<64;i++){
		XDCS_L();
		while(VS1003B_NeedData()==0);
		for(j=0;j<32;j++){
			VS1003B_ByteRW(0x00);
		}
		XDCS_H();
	}
}

void VS1003B_Write32B(uchar * buf){//给VS1003B发送32字节数据，参数为缓冲首地址
	uchar n = 32;
	XDCS_L();
	while(n--){
		VS1003B_ByteRW(*buf++);
	}
	XDCS_H();
}

void VS1003B_SoftReset(){//软复位
	VS1003B_SPI_SetSpeedHigh();
	VS1003B_WriteCMD(MODE,0x0804);
	_delay_ms(20);
}

/*
uint VS1003B_ReadDecodeTime(){//读每秒解码次数
	VS1003B_SPI_SetSpeedHigh();
	return VS1003B_ReadCMD(DECODE_TIME);
}
*/

uchar VS1003B_NeedData(){//检查VS1003B是否需要数据，返回1--需要，0--不需要
	if(DREQ_PIN & (1<<DREQ))
		return 1;
	else
		return 0;
}

void VS1003B_SetVolume(uint vol_val){//设定音量
	VS1003B_SPI_SetSpeedHigh();
	VS1003B_WriteCMD(VOL,vol_val);	
}

uchar VS1003B_Init(){//初始化VS1003B,返回0--成功，1--失败
	uchar retry;
	VS1003B_Port_Init();//初始化端口

	XRESET_L();//硬复位
	_delay_ms(20);
	XRESET_H();

	VS1003B_SPI_SetSpeedLow();//设定SPI低速
	_delay_ms(20);

	retry=0;
	while(VS1003B_ReadCMD(CLOCKF) != CLOCK_REG){//设定PLL寄存器，选择工作频率
		VS1003B_WriteCMD(CLOCKF,CLOCK_REG);
		if(retry++ >10 )return 1;
	}
	_delay_ms(20);
	VS1003B_WriteCMD(AUDATA,0x000A);

	retry=0;
	while(VS1003B_ReadCMD(VOL) != MUTE_VOLUME){	//静音
		VS1003B_WriteCMD(VOL,MUTE_VOLUME);
		if(retry++ >10 )return 1;
	}
	VS1003B_WriteCMD(AUDATA,0xAC45);//软启动

	retry=0;
	while(VS1003B_ReadCMD(VOL) != DEFAULT_VOLUME){//设定默认音量
		VS1003B_WriteCMD(0x0b,DEFAULT_VOLUME);
		if(retry++ >10 )return 1;
	}

	retry=0;
	while(VS1003B_ReadCMD(MODE) != 0x0800){//设定功能寄存器
		VS1003B_WriteCMD(MODE,0x0800);
		if(retry++ >10 )return 1;
	}
	_delay_ms(1);

	retry=0;
	while(VS1003B_ReadCMD(BASS) != DEFAULT_BASS_TREMBLE)	/* set bass/tremble register */
	{
		VS1003B_WriteCMD(BASS,DEFAULT_BASS_TREMBLE);
		if(retry++ >10 )return 1;
	}
	_delay_ms(20);

	VS1003B_SoftReset();	/* A soft reset */
	_delay_ms(20);

	VS1003B_SPI_SetSpeedHigh();//设定SPI高速

	return 0;
}
