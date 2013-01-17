/*********************************************************
Write By Ming	2009-3-23
*********************************************************/
#include<avr/io.h>
#include<util/delay.h>
#include"VS1003B.h"

//**********VS1003B�Ĵ�����ַ����**********//
#define MODE 		0x00	//ģʽ����
#define STATUS		0x01	//VS1003״̬
#define BASS		0x02	//���õ���/������ǿ��
#define CLOCKF		0x03	//ʱ��Ƶ��+��Ƶ��
#define DECODE_TIME	0x04	//ÿ��������
#define AUDATA		0x05	//Misc.��Ƶ����
#define WRAM		0x06	//RAM д/��
#define WRAMADDR	0x07	//RAM д/����ַ
#define HDAT0		0x08	//��ͷ����0
#define HDAT1		0x09	//��ͷ����1
#define AIADDR		0x0A	//�û�������ʼ��ַ
#define VOL			0x0B	//��������
#define AICTRL0		0x0C	//Ӧ�ÿ��ƼĴ���0
#define AICTRL1		0x0D	//Ӧ�ÿ��ƼĴ���1
#define AICTRL2		0x0E	//Ӧ�ÿ��ƼĴ���2
#define AICTRL3		0x0F	//Ӧ�ÿ��ƼĴ���3

//**********VS1003B�Ĵ���ֵ����**********//
#define TREMBLE_VALUE	8		/* 0~15, 8 means off  */
#define TREMBLE_LOW_FS	8		/* 0~15, 0Hz-15KHz, lower frequency of tremble enhancement */
#define BASS_VALUE		0		/* 0~15, 0 means off  */
#define BASS_HIGH_FS	8		/* 2~15, up limit frequency of bass enhancement */
#define DEFAULT_BASS_TREMBLE ((TREMBLE_VALUE<<12)|(TREMBLE_LOW_FS<<8)|(BASS_VALUE<<4)|(BASS_HIGH_FS))

#define CLOCK_REG		0xE000	//PLL�趨ֵ��XTALI * 4.5
#define DEFAULT_VOLUME	0x2828	//Ĭ������ֵ
#define MUTE_VOLUME		0xFEFE	//����ֵ

//**********MCU��VS1003B�������趨**********//
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

//**********�ڲ�����**********//
void VS1003B_SPI_SetSpeedLow(){//�趨SPI����Ƶ��Ϊ1/128ʱ��Ƶ��(����)
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1)|(1<<SPR0);
	SPSR &= ~(1<<SPI2X);
}

void VS1003B_SPI_SetSpeedHigh(){//�趨SPI����Ƶ��Ϊ1/2ʱ��Ƶ��(����)
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR |= (1<<SPI2X);
}

uchar VS1003B_ByteRW(uchar data){//дһ�ֽ����ݣ����ѽ������ݶ���
	SPDR=data;
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

void VS1003B_Port_Init(){//��ʼ���˿ں���
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

//**********�ӿں���**********//
void VS1003B_WriteCMD(uchar addr,uint dat){//��VS1003Bд�Ĵ���
	XDCS_H();
	XCS_L();
	VS1003B_ByteRW(0x02);
	VS1003B_ByteRW(addr);
	VS1003B_ByteRW(dat>>8);
	VS1003B_ByteRW(dat);
	XCS_H();
}

uint VS1003B_ReadCMD(uchar addr){//��VS1003B�Ĵ���
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

void VS1003B_Fill2048Zero(){//��VS1003B����2048������0
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

void VS1003B_Write32B(uchar * buf){//��VS1003B����32�ֽ����ݣ�����Ϊ�����׵�ַ
	uchar n = 32;
	XDCS_L();
	while(n--){
		VS1003B_ByteRW(*buf++);
	}
	XDCS_H();
}

void VS1003B_SoftReset(){//��λ
	VS1003B_SPI_SetSpeedHigh();
	VS1003B_WriteCMD(MODE,0x0804);
	_delay_ms(20);
}

/*
uint VS1003B_ReadDecodeTime(){//��ÿ��������
	VS1003B_SPI_SetSpeedHigh();
	return VS1003B_ReadCMD(DECODE_TIME);
}
*/

uchar VS1003B_NeedData(){//���VS1003B�Ƿ���Ҫ���ݣ�����1--��Ҫ��0--����Ҫ
	if(DREQ_PIN & (1<<DREQ))
		return 1;
	else
		return 0;
}

void VS1003B_SetVolume(uint vol_val){//�趨����
	VS1003B_SPI_SetSpeedHigh();
	VS1003B_WriteCMD(VOL,vol_val);	
}

uchar VS1003B_Init(){//��ʼ��VS1003B,����0--�ɹ���1--ʧ��
	uchar retry;
	VS1003B_Port_Init();//��ʼ���˿�

	XRESET_L();//Ӳ��λ
	_delay_ms(20);
	XRESET_H();

	VS1003B_SPI_SetSpeedLow();//�趨SPI����
	_delay_ms(20);

	retry=0;
	while(VS1003B_ReadCMD(CLOCKF) != CLOCK_REG){//�趨PLL�Ĵ�����ѡ����Ƶ��
		VS1003B_WriteCMD(CLOCKF,CLOCK_REG);
		if(retry++ >10 )return 1;
	}
	_delay_ms(20);
	VS1003B_WriteCMD(AUDATA,0x000A);

	retry=0;
	while(VS1003B_ReadCMD(VOL) != MUTE_VOLUME){	//����
		VS1003B_WriteCMD(VOL,MUTE_VOLUME);
		if(retry++ >10 )return 1;
	}
	VS1003B_WriteCMD(AUDATA,0xAC45);//������

	retry=0;
	while(VS1003B_ReadCMD(VOL) != DEFAULT_VOLUME){//�趨Ĭ������
		VS1003B_WriteCMD(0x0b,DEFAULT_VOLUME);
		if(retry++ >10 )return 1;
	}

	retry=0;
	while(VS1003B_ReadCMD(MODE) != 0x0800){//�趨���ܼĴ���
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

	VS1003B_SPI_SetSpeedHigh();//�趨SPI����

	return 0;
}
