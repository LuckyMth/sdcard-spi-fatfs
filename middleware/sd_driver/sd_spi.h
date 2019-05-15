
#ifndef _SD_SPI_H_
#define _SD_SPI_H_

#include "headfile.h"


//----宏定义SD模块引脚----	 
#define  SD_MOSI_PIN        PTE1
#define  SD_MISO_PIN        PTE3
#define  SD_SCK_PIN         PTE2
#define  SD_CS_PIN          PTE0

//----宏定义sd卡命令----
#define  SD_CMD0        0       //卡复位
#define  SD_CMD8        8       //检测卡版本和电压
#define  SD_CMD9        9       //读CSD
#define  SD_CMD10       10      //读CID
#define  SD_CMD12       12      //stop tran
#define  SD_CMD17       17      //读单块数据
#define  SD_CMD18       18      //读多块数据
#define  SD_CMD24       24      //写单块数据
#define  SD_CMD25       25      //写多块数据
#define  SD_CMD55       55      //发扩展命令前要发
#define  SD_CMD58       58      //get ccs，鉴别卡的容量
#define  SD_CMD59       59      //开启/关闭crc功能，实际上从sd模式进入spi模式后，crc默认关闭


#define  SD_ACMD41      41      //初始化sd卡


extern uint16_t sector_size;


extern void SPI_Init();
extern void SD_Init();
extern uint8_t SD_SendCommand(uint8_t cmd,uint32_t arg,uint8_t crc7);
extern uint8_t SPI_ReceiveByte();
extern void SPI_SendByte(uint8_t Data);
extern uint8_t SPI_ReadWriteByte(uint8_t byte);
extern void SPI_Speed(uint8_t delay);
extern void SD_ReadBlock(uint32_t sector,uint8_t *buff);
extern void SD_Read(uint32_t sector,uint16_t num,uint8_t *buff);
extern void SD_WriteBlock(uint32_t sector,uint8_t *buff);
extern void SD_Write(uint32_t sector,uint16_t num,uint8_t const *buff);
extern void SD_ReadCSD(uint8_t *csd);
extern void SD_ReadCID(uint8_t *cid);
extern uint32_t SD_Sector();



#endif
