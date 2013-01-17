/*********************************************************
Write By Ming	2009-3
*********************************************************/
#include "FAT.h"

BYTE FATType;//�ļ�ϵͳ����,0--FAT32,1--FAT16,2--FAT12
DWORD DiskFristSec;//������ʼ��������
DWORD FirstDataSector;//��һ����������(������ʼ����)
WORD RsvdSecCnt;//����������
WORD BytsPerSec;//ÿ�����ֽ���
BYTE SecPerClus;//ÿ��������
DWORD RootClus;//��Ŀ¼�غ�
WORD RootDirSectors;//count of root directory sectors

//��д��������
#define READFUNC SD_ReadBytes
//��������ӦΪ:����/���ƫ��/��������/��ȡ�ֽ������ɹ��򷵻�0
//BYTE READFUNC(DWORD sector,DWORD offset,BYTE* buf,WORD num);

BYTE InitFAT(){//FAT��ʼ������
	BYTE BufBPB[90];
	DWORD TotSec;//the total sector num
	DWORD FATSz;//the FAT size
	DWORD DataSec;//data sector
	DWORD CountofClusters;//the count of data clusters
	//��ȡ��һ����������ʼ����
	if(READFUNC(0,0x01C6,(BYTE*)(&DiskFristSec),4)!=0)
		return 0xFF;//������
	//��ȡBPB��Ϣ
	if(READFUNC(DiskFristSec,0,BufBPB,90)!=0)
		return 0xFF;//������
	//���沿����Ҫ��Ϣ
	RsvdSecCnt=*((WORD*)(&BufBPB[14]));//BPB_RsvdSecCnt
	BytsPerSec=*((WORD*)(&BufBPB[11]));//BPB_BytsPerSec
	SecPerClus=BufBPB[13];//BPB_SecPerClus
	//����FAT����(ͬʱ��ȡFATSz��TotSec)
	if(*((WORD*)(&BufBPB[22]))!=0){//BPB_FATSz16!=0
		FATSz=*((WORD*)(&BufBPB[22]));//FATSz=BPB_FATSz16
		if((TotSec=*((WORD*)(&BufBPB[19])))==0){//(TotSec=BPB_TotSec16)==0
			TotSec=*((DWORD*)(&BufBPB[32]));//TotSec=BPB_TotSec32
		}
	}
	else{
		FATSz=*((DWORD*)(&BufBPB[36]));//FATSz=BPB_FATSz32
		TotSec=*((DWORD*)(&BufBPB[32]));//TotSec=BPB_FATSz32
	}
	//RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytsPerSec - 1)) / BPB_BytsPerSec
	RootDirSectors=( *((WORD*)(&BufBPB[17])) * 32 + (BytsPerSec-1) ) / BytsPerSec;
	//FirstDataSector = BPB_ResvdSecCnt + (BPB_NumFATs * FATSz) + RootDirSectors
	FirstDataSector=*((WORD*)(&BufBPB[14])) + (BufBPB[16]*FATSz) + RootDirSectors;
	//DataSec = TotSec - (BPB_ResvdSecCnt + (BPB_NumFATs * FATSz) + RootDirSectors)
	DataSec=TotSec-FirstDataSector;
	//CountofClusters = DataSec / BPB_SecPerClus;
	CountofClusters=DataSec / SecPerClus;
	if(CountofClusters<4085){
		FATType=FAT12;//Volume is FAT12
	}
	else if(CountofClusters<65525){
		FATType=FAT16;//Volume is FAT16
		RootClus=0;
	}
	else{
		FATType=FAT32;//Volume is FAT32
		RootClus=*((DWORD*)(&BufBPB[44]));
	}
	FirstDataSector+=DiskFristSec;//��һ�������������Ϸ���ƫ�ƣ��õ����̾���ƫ��
	return FATType;
}

DWORD GetNextClus(DWORD N){//��ȡ��N����һ�غ�
	DWORD FATOffset=0;
	DWORD ThisFATSecNum;
	DWORD ThisFATEntOffset;
	BYTE readbuf[4];
	if(FATType==FAT32){
		FATOffset=N*4;
	}
	else if(FATType==FAT16){
		FATOffset=N*2;
	}
	ThisFATSecNum=DiskFristSec + RsvdSecCnt + ( FATOffset / BytsPerSec );
	ThisFATEntOffset=FATOffset % BytsPerSec;
	if(FATType==FAT32){
		READFUNC(ThisFATSecNum,ThisFATEntOffset,(BYTE*)readbuf,4);
		return *((DWORD*)readbuf);
	}
	else if(FATType==FAT16){
		READFUNC(ThisFATSecNum,ThisFATEntOffset,(BYTE*)readbuf,2);
		return *((WORD*)readbuf);
	}
	return 0xFFFFFFFF;
}

DWORD GetClusSector(DWORD N){//Get the sector num of Cluster N
	if(FATType==FAT16 && N==0){//FAT16��Ŀ¼���⴦��
		return FirstDataSector - RootDirSectors;
	}
	return FirstDataSector + ((N-2)*SecPerClus);
}

BYTE GetDIRinfo(struct DIR * DIRadd,DWORD ClusNum,DWORD DirNum){//get directory information
	return READFUNC(GetClusSector(ClusNum),DirNum*32,(BYTE*)DIRadd,32);//0--Done,1--Failed
}

DWORD GetTotalClus(DWORD fristClus){//Get the total cluster num
	DWORD count=1;
	DWORD EndClus=0;
	if(FATType==FAT32){
		EndClus=0x0FFFFFF8;
	}
	else if(FATType==FAT16){
		EndClus=0xFFF8;
	}
	while((fristClus=GetNextClus(fristClus))<EndClus){
		count++;
	}
	return count;
}

DWORD GetRootClus(){//��ȡ��Ŀ¼�Ĵغ�
	return RootClus;
}

DWORD GetClusSize(){//get the Cluster Size
	return BytsPerSec * SecPerClus;
}

BYTE GetSecPerClus(){//get SecPerClus
	return SecPerClus;
}
