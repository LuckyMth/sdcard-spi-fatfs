
#include "sd_spi.h"

#define SD_MOSI(x)  gpio_set (SD_MOSI_PIN,x)
#define SD_MISO     gpio_get (SD_MISO_PIN)
#define SD_SCK(x)   gpio_set (SD_SCK_PIN,x)
#define SD_CS(x)    gpio_set (SD_CS_PIN,x)

static uint8_t card_version=0;       //1代表Ver1.x，2代表Ver2.00 or later
static uint8_t card_capacity=0;      //1代表sdsc，2代表sdhc
uint16_t sector_size;

static uint8_t delay_us=0;

//初始化引脚
void SPI_Init()
{
    gpio_init(SD_MOSI_PIN,gpo,1);
    gpio_init(SD_MISO_PIN,gpi,0);
    gpio_init(SD_SCK_PIN,gpo,0);
    gpio_init(SD_CS_PIN,gpo,1);
    
    //开漏输出
//    port_opendrain(SD_MOSI_PIN);
//    port_opendrain(SD_MISO_PIN);
//    port_opendrain(SD_SCK_PIN);
//    port_opendrain(SD_CS_PIN);
    
}

//sd初始化函数
void SD_Init()
{
    uint8_t r1,r7[4],r3[4];
//初始化模拟spi引脚
    SPI_Init();
//sd卡复位
    SD_MOSI(0);
    pit_delay_ms(pit0,1);
    SPI_Speed(10);
    //拉高片选线
    SD_CS(1);
    //发送至少74个时钟脉冲（下面发送了80个）
    for(uint8_t i=0;i<10;i++)
    {
        SPI_ReadWriteByte(0xff);
    }
    //发送cmd0，应该响应0x01
    while(SD_SendCommand(SD_CMD0,0,0x95)!=0x01);
    
    /*-----------------------------------------------------
    //发送cmd59，应该响应0x01
    //spi模式默认关闭crc功能
    while(SD_SendCommand(SD_CMD59,0,0)!=0x01);
    -----------------------------------------------------*/
    
//识别sd卡版本
    //发送cmd8，相应类型是R7，拆分成两个变量r1和r7保存
    while((r1=SD_SendCommand(SD_CMD8,0x1aa,0x87))!=0x01);
    for(uint8_t i=0;i<4;i++)
    {
        r7[3-i]=SPI_ReadWriteByte(0xff);
    }
    if((r7[1]&0x0f)==0x01)
    {
        card_version=2;
        sector_size = 512;
    }
    
//sd卡初始化，仅支持2.0或更高版本的卡
    if(card_version==2)
    {
        do{
        //循环发送cmd55+acmd41，先发送cmd55，r1应为0x01
        while(SD_SendCommand(SD_CMD55,0,0x01)!=0x01);
        //再发送acmd41（spi模式下，crc功能默认关闭），响应0x01表示还在初始化，0x00表示初始化完成
        r1=SD_SendCommand(SD_ACMD41,0x40000000,0x01);
        while(r1!=0x01&&r1!=0x00);
        }while(r1==0x01);
    }
    
//鉴别sd卡版本：sdsc or sdhc
    //发送cmd58，响应为R3，分r1，r3保存
    SD_SendCommand(SD_CMD58,0,0x01);
    while((r1=SPI_ReadWriteByte(0xff))!=0x00);
    for(uint8_t i=0;i<4;i++)
    {
        r3[3-i]=SPI_ReadWriteByte(0xff);
    }
    if(r3[3]&0x40)
    {
        //sdhc
        card_capacity=2;
    }
    else
    {
        //sdsc
        card_capacity=1;
    }
    SPI_Speed(2);
}

//sd发送命令函数
uint8_t SD_SendCommand(uint8_t cmd,uint32_t arg,uint8_t crc)
{
    uint8_t r1;
    //由于不能连续发送命令，至少等待8倍的时钟周期，故拉高片选线，等待8个时钟，再拉低片选线，发送8个时钟周期
    SD_CS(1);
    SPI_ReadWriteByte(0xff);
    SD_CS(0);
    SPI_ReadWriteByte(0xff);
    //发送起始位（0）+传输位（1）+命令标号
    SPI_ReadWriteByte(cmd|0x40);
    //发送命令参数
    SPI_ReadWriteByte((uint8_t)(arg>>24));
    SPI_ReadWriteByte((uint8_t)(arg>>16));
    SPI_ReadWriteByte((uint8_t)(arg>>8));
    SPI_ReadWriteByte((uint8_t)(arg>>0));
    //发送crc7+停止位
    SPI_ReadWriteByte(crc);
    //接收r1，r1最高位是0
    do{
        r1 = SPI_ReadWriteByte(0xff);
    }while(r1&0x80);
    return r1;
}

//spi接收单字节
uint8_t SPI_ReceiveByte()
{
    uint8_t byte=0;
    SD_SCK(0);
    for(uint8_t i=8;i>0;i--)
    {
        SD_SCK(1);
        pit_delay_us(pit0,delay_us);
        if(SD_MISO)
        {
            byte|=1;
        }
        else
        {
            byte|=0;
        }
        byte<<=1;
        SD_SCK(0);
        pit_delay_us(pit0,delay_us);
    }
    return byte;
}

//spi发送单字节
void SPI_SendByte(uint8_t byte)
{
    SD_SCK(0);
    for(uint8_t i=8;i>0;i--)
    {
        if(byte&0x80)
        {
            SD_MOSI(1);
        }
        else
        {
            SD_MOSI(0);
        }
        SD_SCK(1);
        pit_delay_us(pit0,delay_us);        
        SD_SCK(0);
        byte<<=1;
        pit_delay_us(pit0,delay_us);        
    }
}

//SPI读写数据
uint8_t SPI_ReadWriteByte(uint8_t byte)
{
    SD_SCK(0);
    for(uint8_t i=8;i>0;i--)
    {
        SD_SCK(0);
        if(byte&0x80)
        {
            SD_MOSI(1);
        }
        else
        {
            SD_MOSI(0);
        }
        pit_delay_us(pit0,delay_us);        
        byte<<=1;
        SD_SCK(1);
        pit_delay_us(pit0,delay_us);        
        byte|=SD_MISO;
    }
    SD_SCK(0);
    return byte;
}

//设置模拟spi延时参数
void SPI_Speed(uint8_t delay)
{
    delay_us=delay;
}

//读单块扇区
//sector：扇区号
//buff：数据指针
void SD_ReadBlock(uint32_t sector,uint8_t *buff)
{
    //发送cmd17，应该响应0x00
    while(SD_SendCommand(SD_CMD17,sector,0x01)!=0x00);
    //接收start block token
    while(SPI_ReadWriteByte(0xff)!=0xfe);
    //接受512字节的用户数据
    for(uint16_t i=0;i<511;i++)
    {
        *buff = SPI_ReadWriteByte(0xff);
        buff++;
    }
    *buff = SPI_ReadWriteByte(0xff);
    //2个字节的crc16
    for(uint8_t i=0;i<2;i++)
    {
        SPI_ReadWriteByte(0xff);
    }
}

//写单块扇区
//sector：扇区号
//buff：数据指针
void SD_WriteBlock(uint32_t sector,uint8_t *buff)
{
    //发送cmd24，应该响应0x00
    while(SD_SendCommand(SD_CMD24,sector,0x01)!=0x00);
    //根据fatfs官网提供的时序图，先发一个空字节？
    SPI_ReadWriteByte(0xff);    
    //发start token
    SPI_ReadWriteByte(0xfe);
    //发送512字节的用户数据
    for(uint16_t i=0;i<511;i++)
    {
        SPI_ReadWriteByte(*buff++);
    }
    SPI_ReadWriteByte(*buff);
    //发送2个字节的crc16
    SPI_ReadWriteByte(0xff);
    SPI_ReadWriteByte(0xff);
    //sd卡返回data_response
    while((SPI_ReadWriteByte(0xff)&0x1f)!=0x05);
    //等待sd卡释放miso线
    while(SPI_ReadWriteByte(0xff)!=0x00);
}

//读多块扇区
//sector：扇区号
//num：扇区数
//buff：数据指针
void SD_Read(uint32_t sector,uint16_t num,uint8_t *buff)
{
    //发送cmd18，应该响应0x00
    while(SD_SendCommand(SD_CMD18,sector,0x01)!=0x00);
    //一次循环接受一个data block
    for(uint16_t i=0;i<num;i++)
    {
        //接收start block token
        while(SPI_ReadWriteByte(0xff)!=0xfe);
        //512字节的用户数据
        for(uint16_t j=0;j<511;j++)
        {
            *buff++ = SPI_ReadWriteByte(0xff);
        }
        if(i == num-1)
        {
            *buff = SPI_ReadWriteByte(0xff);
        }
        else
        {
            *buff++ = SPI_ReadWriteByte(0xff);
        }
        //2个字节的crc16
        for(uint8_t j=0;j<2;j++)
        {
            SPI_ReadWriteByte(0xff);
        }
    }
    //发送cmd12，响应0x00表示忙碌，需要等待返回任何不为0的数值
    while(SD_SendCommand(SD_CMD12,0,0x01)==0x00);
    //r1b为0x00表示忙碌，需等待
    while(SPI_ReadWriteByte(0xff)==0x00);

}

//写多块扇区
//sector：扇区号
//num：扇区数
//buff：数据指针
void SD_Write(uint32_t sector,uint16_t num,uint8_t const *buff)
{
    //发送cmd25，应该响应0x00
    while(SD_SendCommand(SD_CMD25,sector,0x01)!=0x00);
    //根据fatfs官网提供的时序图，先发一个空字节？
    SPI_ReadWriteByte(0xff);    
    //循环一次写一个扇区
    for(uint16_t i=0;i<num;i++)
    {
        //发start token
        SPI_ReadWriteByte(0xfc);
        //发送512字节的用户数据
        for(uint16_t i=0;i<511;i++)
        {
            SPI_ReadWriteByte(*buff++);
        }
        if(i == num-1)
        {
            SPI_ReadWriteByte(*buff);
        }
        else
        {
            SPI_ReadWriteByte(*buff++);
        }
        //发送2个字节的crc16
        SPI_ReadWriteByte(0xff);
        SPI_ReadWriteByte(0xff);
        //sd卡返回data_response
        while((SPI_ReadWriteByte(0xff)&0x1f)!=0x05);
//        等待sd卡释放miso线
        while(SPI_ReadWriteByte(0xff)==0x00);
    }
    //发送stop tran token
    SPI_ReadWriteByte(0xfd);
    //根据fatfs官网提供的时序图，先发一个空字节？
    SPI_ReadWriteByte(0xff);    
    //等待sd卡释放miso线
    while(SPI_ReadWriteByte(0xff)==0x00);
}

//读CSD寄存器
//传入参数：csd应为大小为16字节的数组指针，数组用于保存读取到的csd的内容
void SD_ReadCSD(uint8_t *csd)
{
    //发送cmd9，返回r1，0x00
    while(SD_SendCommand(SD_CMD9,0,0x01)!=0x00);
    //接受start token
    while(SPI_ReadWriteByte(0xff)!=0xfe);
    //接受16个字节，128位的csd寄存器内容
    for(uint8_t i=0;i<16;i++)
    {
        csd[15-i] = SPI_ReadWriteByte(0xff);
    }
    //2个字节的crc16
    for(uint8_t i=0;i<2;i++)
    {
        SPI_ReadWriteByte(0xff);
    }
}

//读CID寄存器
//传入参数：cid应为大小为16字节的数组指针，数组用于保存读取到的cid的内容
void SD_ReadCID(uint8_t *cid)
{
    //发送cmd10，返回r1，0x00
    while(SD_SendCommand(SD_CMD10,0,0x01)!=0x00);
    //接受start token
    while(SPI_ReadWriteByte(0xff)!=0xfe);
    //接受16个字节，128位的cid寄存器内容
    for(uint8_t i=0;i<16;i++)
    {
        cid[15-i] = SPI_ReadWriteByte(0xff);
    }
    //2个字节的crc16
    for(uint8_t i=0;i<2;i++)
    {
        SPI_ReadWriteByte(0xff);
    }
}

//计算sd卡扇区数，计算结果是card_sector
uint32_t SD_Sector()
{
    uint8_t csd[16];
    uint16_t Csize;
    uint32_t card_sector;
    //读CSD寄存器
    SD_ReadCSD(csd);
    //card_capacity为2，表示sdhc
    if(card_capacity == 2)
    {
        Csize = (((uint16_t)csd[7]<<8) + csd[6]);
        card_sector = (Csize + 1) << 10;
    }
    return card_sector;
}


