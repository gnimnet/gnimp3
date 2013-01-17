#include<avr/io.h>

#define DefaultVolume   0x2828

#define VS1003B_PORT    PORTC
#define VS1003B_DDR     DDRC
#define VS1003B_PIN     PINC

#define VS1003B_XCS     PC1
#define VS1003B_XDCS    PC2
#define VS1003B_DREQ    PC3
#define VS1003B_XRESET  PC4

#define DDR_SPI		DDRB
#define DD_MOSI		PB3
#define DD_SCK		PB5
#define DD_SS		PB2

#define PORT_INI()        VS1003B_DDR |= _BV(VS1003B_XCS)|_BV(VS1003B_XRESET)|_BV(VS1003B_XDCS)

#define VS1003B_XCS_H()    VS1003B_PORT |=  _BV(VS1003B_XCS)
#define VS1003B_XCS_L()    VS1003B_PORT &= ~_BV(VS1003B_XCS)

#define VS1003B_XRESET_H()    VS1003B_PORT |=  _BV(VS1003B_XRESET)
#define VS1003B_XRESET_L()    VS1003B_PORT &= ~_BV(VS1003B_XRESET)

#define VS1003B_XDCS_H()    VS1003B_PORT |=  _BV(VS1003B_XDCS)
#define VS1003B_XDCS_L()    VS1003B_PORT &= ~_BV(VS1003B_XDCS)

#define uchar unsigned char
#define uint  unsigned int

void SPI_MasterInit(){
	/* ����MOSI ��SCK��SS Ϊ���������Ϊ���� */
	DDR_SPI = (1<<DD_MOSI)|(1<<DD_SCK)|(1<<DD_SS);
	/* ʹ��SPI ����ģʽ������ʱ������Ϊfck/16 */
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
	SPSR &= ~(1<<SPI2X);
}

//��VS1003дһ�ֽ�		//write one byte to vs1003
uchar VS1003B_WriteByte(uchar cData){
	SPDR = cData;
	while(!(SPSR & (1<<SPIF)))
		;
	return SPDR;
}

//������ٶ�����	//low speed
void VS1003B_SPI_Low(void){
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR1)|(1<<SPR0);
	SPSR &= ~(1<<SPI2X);
}

//��FCK/4�ٶ�����	//full speed
void VS1003B_SPI_High(void){
	SPCR = (1<<SPE)|(1<<MSTR);
	SPSR |= (1<<SPI2X);
}

//��ʱ		//delay
void VS1003B_Delay(uint n){
	while(n--)
		;
}

//д�Ĵ�������������ַ������	//config register
void VS1003B_WriteCMD(uchar addr,uint dat){
	VS1003B_XDCS_H();
	VS1003B_XCS_L();
	VS1003B_WriteByte(0x02);
	VS1003B_WriteByte(addr);
	VS1003B_WriteByte(dat>>8);
	VS1003B_WriteByte(dat);
	VS1003B_XCS_H();
}

//���Ĵ��������� ��ַ ��������	//read register
uint VS1003B_ReadCMD(uchar addr){
	uint temp;
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

//д���ݣ���������		//write data (music data)
void VS1003B_WriteDAT(uchar dat){
	VS1003B_XDCS_L();
	VS1003B_WriteByte(dat);
	VS1003B_XDCS_H();
	VS1003B_XCS_H();
}

uchar VS1003_SineTest()
{
	uchar retry;
	PORT_INI();
	VS1003B_DDR &=~_BV(VS1003B_DREQ);
	VS1003B_XCS_H();
	VS1003B_XCS_H();
	VS1003B_XRESET_L();
	VS1003B_Delay(0xffff);
	VS1003B_XRESET_H();//ʹ��оƬ	//chip select
	VS1003B_SPI_Low();//���Ե�Ƶ����	//low speed
	VS1003B_Delay(0xffff);//��ʱ	//delay
	VS1003B_Delay(0xffff);

	retry=0;

	while(VS1003B_ReadCMD(0x03) != 0xc000){//дʱ�ӼĴ���	//set PLL register
		VS1003B_WriteCMD(0x03,0xc000);
		if(retry++ > 10)
			return 1;
	}

	VS1003B_Delay(0xffff);

	VS1003B_WriteCMD(0x05,0x000a);
	
	retry=0;
	while(VS1003B_ReadCMD(0x0b) != 0xfefe){//������	//set Volume
		VS1003B_WriteCMD(0x0b,0xfefe);
		if(retry++ > 10)
			return 1;
	}


	VS1003B_WriteCMD(0x05,0xac45);

	retry=0;
	while(VS1003B_ReadCMD(0x0b) != 0x2828){//������	//set Volume
		VS1003B_WriteCMD(0x0b,0x2828);
		if(retry++ > 10)
			return 1;
	}

	retry=0;
	while(VS1003B_ReadCMD(0x00) != 0x0820){//дmode�Ĵ���	//set mode register
		VS1003B_WriteCMD(0x00,0x0820);
		if(retry++ > 10)
			return 1;
	}

	VS1003B_Delay(0xffff);

	VS1003B_SPI_High();//����ٶȣ�ȫ������		//turn to high speed
	

	VS1003B_WriteDAT(0x53);
	VS1003B_WriteDAT(0xEF);
	VS1003B_WriteDAT(0x6E);
	VS1003B_WriteDAT(0x7E);
	VS1003B_WriteDAT(0x00);
	VS1003B_WriteDAT(0x00);
	VS1003B_WriteDAT(0x00);
	VS1003B_WriteDAT(0x00);
	return 0;
}

unsigned char VS1003_SineTest1(){
	unsigned char retry;
	PORT_INI();
	VS1003B_DDR &=~_BV(VS1003B_DREQ);
	VS1003B_XCS_H();
	VS1003B_XCS_H();
	VS1003B_XRESET_L();
	VS1003B_Delay(0xffff);
	VS1003B_XRESET_H();//ʹ��оƬ	//chip select
	VS1003B_SPI_Low();//���Ե�Ƶ����	//low speed
	VS1003B_Delay(0xffff);//��ʱ	//delay
	VS1003B_Delay(0xffff);
//	WaitDRQ(10000);


	retry=0;
	while(VS1003B_ReadCMD(0x00) != 0x0820)//дmode�Ĵ���	//set mode register
	{
		VS1003B_WriteCMD(0x00,0x0820);
		if(retry++ >10 )return 1;//
	}	

	VS1003B_Delay(0xffff);
	VS1003B_Delay(0xffff);	

	retry=0;
	while(VS1003B_ReadCMD(0x03) != 0xc000)//дʱ�ӼĴ���	//set PLL register
	{
		VS1003B_WriteCMD(0x03,0xc000);
		if(retry++ >10 )return 1;
	}

	VS1003B_Delay(0xffff);
	VS1003B_Delay(0xffff);
	

	retry=0;
	while(VS1003B_ReadCMD(0x0b) != DefaultVolume)//������	//set Volume
	{
		VS1003B_WriteCMD(0x0b,DefaultVolume);
		if(retry++ >10 )return 1;
	}


	VS1003B_Delay(0xffff);
	VS1003B_Delay(0xffff);

	VS1003B_SPI_High();//����ٶȣ�ȫ������		//turn to high speed
	

	VS1003B_WriteDAT(0x53);
	VS1003B_WriteDAT(0xEF);
	VS1003B_WriteDAT(0x6E);
	VS1003B_WriteDAT(0x7E);
	VS1003B_WriteDAT(0x00);
	VS1003B_WriteDAT(0x00);
	VS1003B_WriteDAT(0x00);
	VS1003B_WriteDAT(0x00);
	return 0;
}


//VS1003��ʼ����0�ɹ� 1ʧ��	//initialize VS1003
unsigned char VS1003B_Init(){
	unsigned char retry;
	PORT_INI();
	VS1003B_DDR &=~_BV(VS1003B_DREQ);
	VS1003B_XCS_H();
	VS1003B_XCS_H();
	VS1003B_XRESET_L();
	VS1003B_Delay(0xffff);
	VS1003B_XRESET_H();//ʹ��оƬ	//chip select
	VS1003B_SPI_Low();//���Ե�Ƶ����	//low speed
	VS1003B_Delay(0xffff);//��ʱ	//delay
	retry=0;
	while(VS1003B_ReadCMD(0x00) != 0x0800)//дmode�Ĵ���	//set mode register
	{
		VS1003B_WriteCMD(0x00,0x0800);
		if(retry++ >10 )return 1;//
	}
	retry=0;
	while(VS1003B_ReadCMD(0x03) != 0xc000)//дʱ�ӼĴ���	//set PLL register
	{
		VS1003B_WriteCMD(0x03,0xc000);
		if(retry++ >10 )return 1;
	}
	retry=0;
	while(VS1003B_ReadCMD(0x0b) != DefaultVolume)//������	//set Volume
	{
		VS1003B_WriteCMD(0x0b,DefaultVolume);
		if(retry++ >10 )return 1;
	}
	VS1003B_SPI_High();//����ٶȣ�ȫ������		//turn to high speed
	return 0;
}

//VS1003�����λ	//VS1003 soft reset
void VS1003B_SoftReset(){
	VS1003B_WriteCMD(0x00,0x0804);//д��λ		//reset
	VS1003B_Delay(0xffff);//��ʱ������1.35ms //wait at least 1.35ms
}



int main(){
	SPI_MasterInit();
	VS1003_SineTest();
	while(1)
	{
		VS1003B_Delay(0xffff);
	}
}

