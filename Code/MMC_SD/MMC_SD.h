/******************************************************************************
Write by Ming	2009-3-3
*******************************************************************************
使用注意事项:
1.在MMC_SD.c内设定所使用引脚
2.在程序开始应当先进行初始化工作(调用SD_Init)
******************************************************************************/
#ifndef _MMC_SD_h_
#define _MMC_SD_h_

#include "../mytype.h"

BYTE SD_Init();//SD卡初始化函数，成功返回0，失败则返回非0
BYTE SD_ReadOneSector(DWORD sector,BYTE* buf);//读一个扇区(buf为512字节)，成功返回0
BYTE SD_ReadBytes(DWORD sector,DWORD offset,BYTE* buf,WORD num);//读取若干字节

#endif //end of _MMC_SD_h_
