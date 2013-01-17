#include <avr/io.h>
#include <util/delay.h>
#include "MMC_SD/MMC_SD.h"
#include "FAT/FAT.h"
#include "VS1003B/VS1003B.h"

#define PLAY_FILE_BUF_SIZE	512	//必须是32的整数倍
#define SONG_CLUS_LIST_SIZE	70	//列表大小
#define MAX_SEARCH_DEPTH	5	//文件夹搜索最大深度
#define MODE_NUMS			3	//模式数量

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


DWORD SongClusList[SONG_CLUS_LIST_SIZE];//歌曲所在目录簇号
BYTE SongIndex[SONG_CLUS_LIST_SIZE];//歌曲对应目录项偏移
BYTE TotalNum;//总歌曲数目
uchar NowVol;//当前音量
uchar PlayMode;//播放模式 0--全部循环，1--单曲循环，2--随机播放

int main(void){
	BYTE NowPlay=0,tmp;
	MaskVS1003B();//屏蔽VS1003对SD初始化影响
	NowVol=0x28;
	LEDDDR|=(1<<LED1)|(1<<LED2)|(1<<LED3);//LED端口设为输出
	KEYDDR&=~((1<<KEY1)|(1<<KEY2)|(1<<KEY3)|(1<<KEY4)|(1<<KEY5));//按键端口设为输入
	TCCR0=0x01;//打开Timer0用作随机数种子
	LEDPORT|=(1<<LED1)|(1<<LED2)|(1<<LED3);
	//初始化SD卡信息
	while(SD_Init()!=0)
		;
	LEDPORT=~(1<<LED1);
	//初始化FAT信息
	if(InitFAT()>1){//>2则为FAT12或初始化出错
		while(1);//死循环等待复位
	}
	LEDPORT=~(1<<LED2);
	//初始化VS1003B
	while(VS1003B_Init()==1){
		MyDelay(100);//等待0.1秒重试
	}
	LEDPORT=~(1<<LED3);
	//查找歌曲
	SearchSongs();
	if(TotalNum==0){
		while(1);//没有歌曲
	}
	PlayMode=0;
	LEDPORT=(1<<PlayMode);
	//进入播放循环
	while(1){
		VS1003B_SoftReset();
		tmp=PlayFile(SongClusList[NowPlay],SongIndex[NowPlay]);//播放歌曲
		if(PlayMode==0 || PlayMode==1){
			if(tmp==0 && PlayMode==0){//歌曲播放完毕且为全部循环播放，切换下一首
				if(NowPlay<TotalNum-1)
					NowPlay++;
				else
					NowPlay=0;
			}
			else if(tmp==0xFF){//按下下一首按键
				if(NowPlay<TotalNum-1)
					NowPlay++;
				else
					NowPlay=0;
				MyDelay(10);
			}
			else if(tmp==0xFE){//按下上一首按键
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

BYTE GetFileType(DWORD DirFristClus,BYTE index){//获取文件类型函数
	WORD totalnum=GetClusSize()/32;
	struct DIR DIRinfo;
	while(index>=totalnum){
		index-=totalnum;
		DirFristClus=GetNextClus(DirFristClus);//文件位于下一簇
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

void SearchSongs(){//查找歌曲函数，用于初始化歌曲列表
	DWORD ClusPWD;//当前工作目录簇号
	BYTE IndexPWD[MAX_SEARCH_DEPTH];//当前工作目录簇号偏移
	BYTE NowLevel;//当前目录级数
	BYTE tmp;
	ClusPWD=GetRootClus();//从根目录开始
	IndexPWD[0]=0;//从0偏移开始找
	NowLevel=0;//当前目录为0级
	TotalNum=0;
	while(1){
		tmp=GetFileType(ClusPWD,IndexPWD[NowLevel]);
		if(tmp==0){//IO错误，重试
			continue;
		}
		else if(tmp==1){//当前目录搜索完毕
			if(NowLevel==0){
				return;//根目录则搜索结束
			}
			else{
				ChangePWD(&ClusPWD,1);//调回上级目录
				NowLevel--;
			}
		}
		else if(tmp==5){
			if(NowLevel<MAX_SEARCH_DEPTH){//未超过最大深度
				ChangePWD(&ClusPWD,IndexPWD[NowLevel]);
				NowLevel++;
				IndexPWD[NowLevel]=1;
			}
		}
		else if(tmp==7){//记录该文件
			SongClusList[TotalNum]=ClusPWD;
			SongIndex[TotalNum]=IndexPWD[NowLevel];
			TotalNum++;
			if(TotalNum>=SONG_CLUS_LIST_SIZE)
				return;//文件列表满
		}
		IndexPWD[NowLevel]++;//处理下一文件
	}
}

BYTE ChangePWD(DWORD * PWDClusAdd,WORD Offset){//切换工作目录函数
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


BYTE PlayFile(DWORD PWDClus,WORD Offset){//播放文件函数
	struct DIR DIRinfo;
	DWORD FileClus,FileSize,HaveSend,ClusSize,SecSend;
	DWORD NowClusSec;
	BYTE Buf[512];
	BYTE SecPerClus,SecCnt;
	uchar Key,Play=1;
	uint Skip=0;
	ClusSize=GetClusSize();//获取簇大小
	SecPerClus=GetSecPerClus();//获取每簇扇区数
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
			if(SecSend>=512){//当前扇区发送完毕
				SecCnt++;//下一扇区
				if(SD_ReadOneSector(NowClusSec+SecCnt,(BYTE*)Buf)!=0)
					return 1;
				if(SecCnt>=SecPerClus){//当前簇发送完毕
					SecCnt=0;
					FileClus=GetNextClus(FileClus);
					NowClusSec=GetClusSector(FileClus);
				}
				SecSend=0;
			}
		}
		else if((Key=GetKey())!=0 && Skip==0){
			if(Key==1){
				ChangeVol(1);//音量减
				Skip=0x1FFF;
			}
			else if(Key==2){
				ChangeVol(0);//音量加
				Skip=0x1FFF;
			}
			else if(Key==3){
				VS1003B_Fill2048Zero();
				return 0xFE;//上一首
			}
			else if(Key==4){
				VS1003B_Fill2048Zero();
				return 0xFF;//下一首
			}
			else if(Key==5){
				PlayMode=(PlayMode+1) % MODE_NUMS;//切换模式
				LEDPORT=(1<<PlayMode);
				Skip=0x5FFF;
			}
		}
		if(Skip!=0)
			Skip--;
	}
	return 0;
}

void ChangeVol(uchar dec){//调整音量函数
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
