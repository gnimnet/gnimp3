/*********************************************************
Write By Ming	2009-3
*********************************************************/
#include "FAT.h"

BYTE FATType;//文件系统类型,0--FAT32,1--FAT16,2--FAT12
DWORD DiskFristSec;//分区起始绝对扇区
DWORD FirstDataSector;//第一个数据扇区(数据起始扇区)
WORD RsvdSecCnt;//保留扇区数
WORD BytsPerSec;//每扇区字节数
BYTE SecPerClus;//每簇扇区数
DWORD RootClus;//根目录簇号
WORD RootDirSectors;//count of root directory sectors

//读写函数声明
#define READFUNC SD_ReadBytes
//函数参数应为:扇区/相对偏移/读缓冲区/读取字节数，成功则返回0
//BYTE READFUNC(DWORD sector,DWORD offset,BYTE* buf,WORD num);

BYTE InitFAT(){//FAT初始化函数
	BYTE BufBPB[90];
	DWORD TotSec;//the total sector num
	DWORD FATSz;//the FAT size
	DWORD DataSec;//data sector
	DWORD CountofClusters;//the count of data clusters
	//读取第一个分区的起始扇区
	if(READFUNC(0,0x01C6,(BYTE*)(&DiskFristSec),4)!=0)
		return 0xFF;//读错误
	//读取BPB信息
	if(READFUNC(DiskFristSec,0,BufBPB,90)!=0)
		return 0xFF;//读错误
	//保存部分重要信息
	RsvdSecCnt=*((WORD*)(&BufBPB[14]));//BPB_RsvdSecCnt
	BytsPerSec=*((WORD*)(&BufBPB[11]));//BPB_BytsPerSec
	SecPerClus=BufBPB[13];//BPB_SecPerClus
	//计算FAT类型(同时获取FATSz和TotSec)
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
	FirstDataSector+=DiskFristSec;//第一个数据扇区加上分区偏移，得到磁盘绝对偏移
	return FATType;
}

DWORD GetNextClus(DWORD N){//获取簇N的下一簇号
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
	if(FATType==FAT16 && N==0){//FAT16根目录特殊处理
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

DWORD GetRootClus(){//获取根目录的簇号
	return RootClus;
}

DWORD GetClusSize(){//get the Cluster Size
	return BytsPerSec * SecPerClus;
}

BYTE GetSecPerClus(){//get SecPerClus
	return SecPerClus;
}
