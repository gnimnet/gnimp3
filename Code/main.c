#include <avr/io.h>
#include <util/delay.h>
#include "MMC_SD/MMC_SD.h"
#include "FAT/FAT.h"
#include "VS1003B/VS1003B.h"

#define PLAY_FILE_BUF_SIZE	512	//������32��������
#define SONG_CLUS_LIST_SIZE	70	//�б��С
#define MAX_SEARCH_DEPTH	5	//�ļ�������������
#define MODE_NUMS			3	//ģʽ����

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

void PrintFileName(DWORD DirFristClus,BYTE index);

void SearchSongs();
BYTE ChangePWD(DWORD * PWDClusAdd,WORD Offset);
BYTE PlayFile(DWORD PWDClus,WORD Offset);
void ChangeVol(uchar dec);
uchar GetKey();
void MyDelay(uint ms);
void MaskVS1003B();


DWORD SongClusList[SONG_CLUS_LIST_SIZE];//��������Ŀ¼�غ�
BYTE SongIndex[SONG_CLUS_LIST_SIZE];//������ӦĿ¼��ƫ��
BYTE TotalNum;//�ܸ�����Ŀ
uchar NowVol;//��ǰ����
uchar PlayMode;//����ģʽ 0--ȫ��ѭ����1--����ѭ����2--�������

int main(void){
	BYTE NowPlay=0,tmp;
	MaskVS1003B();//����VS1003��SD��ʼ��Ӱ��
	NowVol=0x28;
	LEDDDR|=(1<<LED1)|(1<<LED2)|(1<<LED3);//LED�˿���Ϊ���
	KEYDDR&=~((1<<KEY1)|(1<<KEY2)|(1<<KEY3)|(1<<KEY4)|(1<<KEY5));//�����˿���Ϊ����
	TCCR0=0x01;//��Timer0�������������
	LEDPORT|=(1<<LED1)|(1<<LED2)|(1<<LED3);
	//��ʼ��SD����Ϣ
	while(SD_Init()!=0)
		;
	LEDPORT=~(1<<LED1);
	//��ʼ��FAT��Ϣ
	if(InitFAT()>1){//>2��ΪFAT12���ʼ������
		while(1);//��ѭ���ȴ���λ
	}
	LEDPORT=~(1<<LED2);
	//��ʼ��VS1003B
	while(VS1003B_Init()==1){
		MyDelay(100);//�ȴ�0.1������
	}
	LEDPORT=~(1<<LED3);
	//���Ҹ���
	SearchSongs();
	if(TotalNum==0){
		while(1);//û�и���
	}
	PlayMode=0;
	LEDPORT=(1<<PlayMode);
	//���벥��ѭ��
	while(1){
		VS1003B_SoftReset();
		tmp=PlayFile(SongClusList[NowPlay],SongIndex[NowPlay]);//���Ÿ���
		if(PlayMode==0 || PlayMode==1){
			if(tmp==0 && PlayMode==0){//�������������Ϊȫ��ѭ�����ţ��л���һ��
				if(NowPlay<TotalNum-1)
					NowPlay++;
				else
					NowPlay=0;
			}
			else if(tmp==0xFF){//������һ�װ���
				if(NowPlay<TotalNum-1)
					NowPlay++;
				else
					NowPlay=0;
				MyDelay(10);
			}
			else if(tmp==0xFE){//������һ�װ���
				if(NowPlay==0)
					NowPlay=TotalNum-1;
				else
					NowPlay--;
				MyDelay(10);
			}
		}
		else if(PlayMode==2){
			NowPlay=TCNT0;
			NowPlay%=TotalNum;
		}
	}
	return 0;
}

BYTE GetFileType(DWORD DirFristClus,BYTE index){//��ȡ�ļ����ͺ���
	WORD totalnum=GetClusSize()/32;
	struct DIR DIRinfo;
	while(index>=totalnum){
		index-=totalnum;
		DirFristClus=GetNextClus(DirFristClus);//�ļ�λ����һ��
	}
	if(GetDIRinfo(&DIRinfo,DirFristClus,index)!=0){//get file information
		return 0;//Read DIRinfo error!
	}
	if(DIRinfo.DIR_Name[0]==0x00)//End
		return 1;
	if(DIRinfo.DIR_Name[0]==0xE5)//Empty
		return 2;
	if(DIRinfo.DIR_Attr==ATTR_LONG_NAME)//Long File Name
		return 3;
	if(DIRinfo.DIR_Attr==ATTR_VOLUME_ID)
		return 4;
	else if(DIRinfo.DIR_Attr & ATTR_DIRECTORY)
		return 5;
	else{
		if(!(
		(DIRinfo.DIR_Name[8]=='M' && DIRinfo.DIR_Name[9]=='P' && DIRinfo.DIR_Name[10]=='3')||
		(DIRinfo.DIR_Name[8]=='W' && DIRinfo.DIR_Name[9]=='A' && DIRinfo.DIR_Name[10]=='V')||
		(DIRinfo.DIR_Name[8]=='M' && DIRinfo.DIR_Name[9]=='I' && DIRinfo.DIR_Name[10]=='D')||
		(DIRinfo.DIR_Name[8]=='W' && DIRinfo.DIR_Name[9]=='M' && DIRinfo.DIR_Name[10]=='A')
		)){
			return 6;//not a music file
		}
		return 7;//is a music file
	}
	return 8;//unknow file type
}

void SearchSongs(){//���Ҹ������������ڳ�ʼ�������б�
	DWORD ClusPWD;//��ǰ����Ŀ¼�غ�
	BYTE IndexPWD[MAX_SEARCH_DEPTH];//��ǰ����Ŀ¼�غ�ƫ��
	BYTE NowLevel;//��ǰĿ¼����
	BYTE tmp;
	ClusPWD=GetRootClus();//�Ӹ�Ŀ¼��ʼ
	IndexPWD[0]=0;//��0ƫ�ƿ�ʼ��
	NowLevel=0;//��ǰĿ¼Ϊ0��
	TotalNum=0;
	while(1){
		tmp=GetFileType(ClusPWD,IndexPWD[NowLevel]);
		if(tmp==0){//IO��������
			continue;
		}
		else if(tmp==1){//��ǰĿ¼�������
			if(NowLevel==0){
				return;//��Ŀ¼����������
			}
			else{
				ChangePWD(&ClusPWD,1);//�����ϼ�Ŀ¼
				NowLevel--;
			}
		}
		else if(tmp==5){
			if(NowLevel<MAX_SEARCH_DEPTH){//δ����������
				ChangePWD(&ClusPWD,IndexPWD[NowLevel]);
				NowLevel++;
				IndexPWD[NowLevel]=1;
			}
		}
		else if(tmp==7){//��¼���ļ�
			SongClusList[TotalNum]=ClusPWD;
			SongIndex[TotalNum]=IndexPWD[NowLevel];
			TotalNum++;
			if(TotalNum>=SONG_CLUS_LIST_SIZE)
				return;//�ļ��б���
		}
		IndexPWD[NowLevel]++;//������һ�ļ�
	}
}

BYTE ChangePWD(DWORD * PWDClusAdd,WORD Offset){//�л�����Ŀ¼����
	struct DIR DIRinfo;
	if(GetDIRinfo(&DIRinfo,*PWDClusAdd,Offset)!=0){//get file information
		return 1;
	}
	if(DIRinfo.DIR_Attr==ATTR_LONG_NAME || DIRinfo.DIR_Attr!=ATTR_DIRECTORY){
		return 2;//not a dictory
	}
	else{
		*PWDClusAdd=*((WORD*)&DIRinfo.DIR_FstClusHI);
		*PWDClusAdd=*PWDClusAdd<<16;
		*PWDClusAdd+=*((WORD*)&DIRinfo.DIR_FstClusLO);
		if(*PWDClusAdd==0){//Root dir
			*PWDClusAdd=GetRootClus();
		}
		return 0;
	}
}


BYTE PlayFile(DWORD PWDClus,WORD Offset){//�����ļ�����
	struct DIR DIRinfo;
	DWORD FileClus,FileSize,HaveSend,ClusSize,SecSend;
	DWORD NowClusSec;
	BYTE Buf[512];
	BYTE SecPerClus,SecCnt;
	uchar Key,Play=1;
	uint Skip=0;
	ClusSize=GetClusSize();//��ȡ�ش�С
	SecPerClus=GetSecPerClus();//��ȡÿ��������
	if(GetDIRinfo(&DIRinfo,PWDClus,Offset)!=0){//get file information
		return 1;
	}
	FileClus=*((WORD*)&DIRinfo.DIR_FstClusHI);
	FileClus=FileClus<<16;
	FileClus+=*((WORD*)&DIRinfo.DIR_FstClusLO);
	FileSize=*((DWORD*)DIRinfo.DIR_FileSize);
	HaveSend=0;
	NowClusSec=GetClusSector(FileClus);
	SecCnt=0;
	if(SD_ReadOneSector(NowClusSec,(BYTE*)Buf)!=0)
		return 1;
	SecSend=0;
	while(1){
		if(VS1003B_NeedData() && Play){
			VS1003B_Write32B(&Buf[SecSend]);
			SecSend+=32;
			HaveSend+=32;
			if(HaveSend>=FileSize)
				break;
			if(SecSend>=512){//��ǰ�����������
				SecCnt++;//��һ����
				if(SD_ReadOneSector(NowClusSec+SecCnt,(BYTE*)Buf)!=0)
					return 1;
				if(SecCnt>=SecPerClus){//��ǰ�ط������
					SecCnt=0;
					FileClus=GetNextClus(FileClus);
					NowClusSec=GetClusSector(FileClus);
				}
				SecSend=0;
			}
		}
		else if((Key=GetKey())!=0 && Skip==0){
			if(Key==1){
				ChangeVol(1);//������
				Skip=0x1FFF;
			}
			else if(Key==2){
				ChangeVol(0);//������
				Skip=0x1FFF;
			}
			else if(Key==3){
				VS1003B_Fill2048Zero();
				return 0xFE;//��һ��
			}
			else if(Key==4){
				VS1003B_Fill2048Zero();
				return 0xFF;//��һ��
			}
			else if(Key==5){
				PlayMode=(PlayMode+1) % MODE_NUMS;//�л�ģʽ
				LEDPORT=(1<<PlayMode);
				Skip=0x5FFF;
			}
		}
		if(Skip!=0)
			Skip--;
	}
	return 0;
}

void ChangeVol(uchar dec){//������������
	uint tmp;
	if(dec){
		if(NowVol<0xFE)
			NowVol+=2;
	}
	else{
		if(NowVol>0)
			NowVol-=2;
	}
	tmp=NowVol;
	tmp=tmp<<8;
	tmp+=NowVol;
	VS1003B_SetVolume(tmp);
}

uchar GetKey(){
	if( (KEYPIN & (1<<KEY1))==0){
		return 1;
	}
	else if( (KEYPIN & (1<<KEY2))==0){
		return 2;
	}
	else if( (KEYPIN & (1<<KEY3))==0){
		return 3;
	}
	else if( (KEYPIN & (1<<KEY4))==0){
		return 4;
	}
	else if( (KEYPIN & (1<<KEY5))==0){
		return 5;
	}
	return 0;
}

void MyDelay(uint ms){
	while(ms--)
		_delay_ms(ms);
}

void MaskVS1003B(){
	DDRC|=(1<<PC2);
	PORTC|=(1<<PC2);
}
