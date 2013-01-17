/********************************************************
Write By Ming	2009-3
*********************************************************
使用注意事项:
1.在程序开始应当先进行初始化工作(并按照一定顺序):
	先初始化SD卡，再初始化FAT
2.使用ReadFile函数必须自己保证参数offset+count<=文件大小
********************************************************/
#ifndef _FAT_H_
#define _FAT_H_

#include "../MyType.h"
#include "../MMC_SD/MMC_SD.h"

//**********FAT相关声明**********//
//文件系统宏声明
#define FAT32 0
#define FAT16 1
#define FAT12 2

//文件属性宏声明
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY|ATTR_HIDDEN|ATTR_SYSTEM|ATTR_VOLUME_ID)

//for LDIR_Ord Mark byte
#define LAST_LONG_ENTRY 0x40

//**********结构体声明**********//
struct DIR{//文件入口结构体
	BYTE DIR_Name[11];//file name
	BYTE DIR_Attr;//attribute byte
	BYTE DIR_NTRes;//Reserved for use by Windows NT
	BYTE DIR_CrtTimeTenth;//Millisecond stamp at file creation time
	BYTE DIR_CrtTime[2];//Time file was created
	BYTE DIR_CrtDate[2];//Date file was created
	BYTE DIR_LstAccDate[2];//Last access date
	BYTE DIR_FstClusHI[2];//High word of this entry first cluster number(FAT32)
	BYTE DIR_WrtTime[2];//Time of last write
	BYTE DIR_WrtDate[2];//Date of last write
	BYTE DIR_FstClusLO[2];//Low word of this entry first cluster number
	BYTE DIR_FileSize[4];//32-bit DWORD holding this file size in bytes
};

//**********接口函数**********//
BYTE InitFAT();
DWORD GetNextClus(DWORD N);//获取簇N的下一簇号
DWORD GetTotalClus(DWORD fristClus);//获取该文件的总簇数
BYTE GetDIRinfo(struct DIR * DIRadd,DWORD ClusNum,DWORD DirNum);//获取文件信息
BYTE ReadFile(DWORD fristClus,DWORD offset,BYTE*buf,DWORD count);//读取文件数据
DWORD GetRootClus();//获取根目录的簇号
DWORD GetClusSize();//获取一个簇的字节数
BYTE GetSecPerClus();//获取每簇扇区数
DWORD GetClusSector(DWORD N);//获取一个簇的起始扇区号


#endif//end of fat.h
