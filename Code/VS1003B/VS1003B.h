/******************************************************************************
Write by Ming	2009-3-23
*******************************************************************************
使用注意事项:
1.在VS1003B.c内设定所使用引脚
2.在程序开始应当先进行初始化工作
******************************************************************************/
#ifndef _VS1003B_H_
#define _VS1003B_H_

#include "../MyType.h"

void VS1003B_WriteCMD(uchar addr,uint dat);//向VS1003B写寄存器
uint VS1003B_ReadCMD(uchar addr);//读VS1003B寄存器
void VS1003B_Fill2048Zero();//给VS1003B发送2048个数据0
void VS1003B_Write32B(uchar * buf);//给VS1003B发送32字节数据，参数为缓冲首地址
void VS1003B_SoftReset();//软复位
uchar VS1003B_NeedData();//检查VS1003B是否需要数据，返回1--需要，0--不需要
void VS1003B_SetVolume(uint vol_val);//设定音量
uchar VS1003B_Init();//初始化VS1003B,返回0--成功，1--失败

#endif //end of _VS1003B_H_
