/******************************************************************************
Write by Ming	2009-3-3
******************************************************************************/
#include <avr/io.h>
#include "MMC_SD.h"

//**********MCU��SD�������趨**********//
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

//**********�ڲ�����**********//
void SPI_SetSpeedLow(){//�趨SPI����Ƶ��Ϊ1/128ʱ��Ƶ��(����)
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1)|(1<<SPR0);
	SPSR &= ~(1<<SPI2X);
}

void SPI_SetSpeedHigh(){//�趨SPI����Ƶ��Ϊ1/2ʱ��Ƶ��(����)
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR |= (1<<SPI2X);
}

BYTE SPI_ByteRW(BYTE data){//��SPI��дһ���ֽ�����
	SPDR=data;
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}

BYTE SD_SendCommand(BYTE cmd,DWORD arg){//��SD����������
	BYTE rtn,cnt=0;
	
	SPI_ByteRW(0xFF);
	CS_LOW();
	
	SPI_ByteRW(cmd | 0x40);//send command
	SPI_ByteRW(arg>>24);
	SPI_ByteRW(arg>>16);
	SPI_ByteRW(arg>>8);
	SPI_ByteRW(arg);
	SPI_ByteRW(0x95);
	
	while((rtn=SPI_ByteRW(0xFF))==0xFF)//�ȴ�SDӦ��
		if((cnt++)==0xFF)
			break;//��ʱ

	CS_HIGH();
	SPI_ByteRW(0xFF);//����8��ʱ������

	return rtn;//����״ֵ̬
}

void Port_Init(){
	CS_PORT |= 1<<CS;
	CS_DDR |= 1<<CS;
	SCK_DDR |= 1<<SCK;
	MOSI_DDR |= 1<<MOSI;
	MISO_DDR &= ~(1<<MISO);
	SS_DDR |= 1<<SS;
}

BYTE ReadBegin(DWORD sector){//������ǰ�ĳ�ʼ��
	BYTE rtn;
	WORD cnt=0;
	SPI_SetSpeedHigh();//�趨SPI������ģʽ
	rtn = SD_SendCommand(17,sector<<9);//������
	if(rtn!=0x00)
		return 1;
	CS_LOW();
	//�����ݵĿ�ʼ
	while(SPI_ByteRW(0xFF)!=0xFE){
		if((cnt++)==0xFFFF){
			CS_HIGH();
			return 2;//��ʱ�˳�
		}
	}
	return 0;
}

void ReadEnd(){//��������ɺ�Ĳ���
	SPI_ByteRW(0xFF);//dummy crc
	SPI_ByteRW(0xFF);
	CS_HIGH();
	SPI_ByteRW(0xFF);//����8��ʱ������
}

//**********�ӿں���**********//
BYTE SD_Init(){//SD����ʼ���������ɹ�����0��ʧ���򷵻ط�0
	BYTE i;
	BYTE cnt = 0;
	BYTE rtn = 0;	
	Port_Init();//��ʼ������
	SPI_SetSpeedLow();//SPI��ʼ��,�趨Ϊ����
	do{
		for(i=0;i<10;i++)
			SPI_ByteRW(0xFF);
		rtn=SD_SendCommand(0,0);//��idle����
		if((cnt++)==0xFF)
			return 1;//��ʱ�˳�
	}while(rtn!=0x01);
	cnt=0;//�������Լ�����
	do{
		rtn=SD_SendCommand(1,0);//��active����
		if((cnt++)==0xFF)
			return 2;//��ʱ�˳�
	}while(rtn);
	SPI_SetSpeedHigh();//�趨SPI������ģʽ
	SD_SendCommand(59,0);//����CRC
	SD_SendCommand(16,512);//�趨������СΪ512

	return 0;//��ʼ���ɹ�
}

BYTE SD_ReadBytes(DWORD sector,DWORD offset,BYTE* buf,WORD num){//��ȡ�����ֽ�
	WORD SecReadCnt=0;
	while(offset>=512){//����ƫ������������
		offset-=512;
		sector++;
	}
	if(ReadBegin(sector++)!=0)
		return 1;//�����״̬ʧ��
	while(offset--){//����offset���ֽ�
		SecReadCnt++;
		SPI_ByteRW(0xFF);
	}
	while(num){//��������Ҫ��
		while(num>0 && SecReadCnt<512){//��������Ҫ�����ұ�����δ����
			*(buf++)=SPI_ByteRW(0xFF);
			SecReadCnt++;
			num--;
		}
		if(num==0){//�����
			break;
		}
		else{//����δ���꣬������������
			ReadEnd();
			if(ReadBegin(sector++)!=0)
				return 1;//������һ������״̬ʧ��
			SecReadCnt=0;//�������ֽڼ�������
		}
	}
	while((SecReadCnt++)<512){//���������������ֽ�
		SPI_ByteRW(0xFF);
	}
	ReadEnd();
	return 0;
}

BYTE SD_ReadOneSector(DWORD sector,BYTE* buf){//��һ������(bufΪ512�ֽ�)���ɹ�����0
	WORD i;
	if(ReadBegin(sector++)!=0)
		return 1;//�����״̬ʧ��
	for(i=0;i<512;i++){//��512������
		*(buf++)=SPI_ByteRW(0xFF);
	}
	ReadEnd();//�˳���״̬
	return 0;
}
