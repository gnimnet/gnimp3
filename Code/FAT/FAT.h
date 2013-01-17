/********************************************************
Write By Ming	2009-3
*********************************************************
ʹ��ע������:
1.�ڳ���ʼӦ���Ƚ��г�ʼ������(������һ��˳��):
	�ȳ�ʼ��SD�����ٳ�ʼ��FAT
2.ʹ��ReadFile���������Լ���֤����offset+count<=�ļ���С
********************************************************/
#ifndef _FAT_H_
#define _FAT_H_

#include "../MyType.h"
#include "../MMC_SD/MMC_SD.h"

//**********FAT�������**********//
//�ļ�ϵͳ������
#define FAT32 0
#define FAT16 1
#define FAT12 2

//�ļ����Ժ�����
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY|ATTR_HIDDEN|ATTR_SYSTEM|ATTR_VOLUME_ID)

//for LDIR_Ord Mark byte
#define LAST_LONG_ENTRY 0x40

//**********�ṹ������**********//
struct DIR{//�ļ���ڽṹ��
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

//**********�ӿں���**********//
BYTE InitFAT();
DWORD GetNextClus(DWORD N);//��ȡ��N����һ�غ�
DWORD GetTotalClus(DWORD fristClus);//��ȡ���ļ����ܴ���
BYTE GetDIRinfo(struct DIR * DIRadd,DWORD ClusNum,DWORD DirNum);//��ȡ�ļ���Ϣ
BYTE ReadFile(DWORD fristClus,DWORD offset,BYTE*buf,DWORD count);//��ȡ�ļ�����
DWORD GetRootClus();//��ȡ��Ŀ¼�Ĵغ�
DWORD GetClusSize();//��ȡһ���ص��ֽ���
BYTE GetSecPerClus();//��ȡÿ��������
DWORD GetClusSector(DWORD N);//��ȡһ���ص���ʼ������


#endif//end of fat.h
