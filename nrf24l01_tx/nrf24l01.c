
#include "bsp/config.h"



//定义SPI的操作IO

#define NRF24L01_CE0	P11 = 0	
#define NRF24L01_CE1	P11 = 1
#define SPI_SCK0		P10 = 0
#define SPI_SCK1		P10 = 1
#define SPI_MISO		P37
#define SPI_CS0			P36 = 0
#define SPI_CS1			P36 = 1
#define SPI_MOSI0		P33 = 0
#define SPI_MOSI1		P33 = 1


//SPI最小延时，决定了spi的性能
#define SPI_DELAY		delay_us(10)
//定义SPI的序号，如果多次调用该文件，需要保证序号不重复
#define SPI()				SPI ## 1
#include "io_spi/io_spi.h"


u8 const TX_ADDRESS[TX_ADR_WIDTH] = {0x34, 0x43, 0x10, 0x10, 0x01};	//本地地址
u8 const RX_ADDRESS[RX_ADR_WIDTH] = {0x34, 0x43, 0x10, 0x10, 0x01};	//接收地址


u8 NRF24L01_Read(u8 addr)
{
    u8 val;

    SPI()_CS(0);                // CSN low, initialize SPI communication...
    DELAY_US(12);
    SPI()_Byte(addr);            // Select register to read from..
    DELAY_US(12);
    val = SPI()_Byte(0);    	// ..then read registervalue
    DELAY_US(12);
    SPI()_CS(1);                // CSN high, terminate SPI communication

    return(val);        // return register value
}

/*
NRF24L01初始化
*/
void nRF24L01_Init(void)
{
	u8 i;
	for(i=0;i<10;i++)DELAY_US(100);

	NRF24L01_CE0;
	SPI()_Init();
    
    NRF24L01_Wbuf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);    // 写本地地址
    NRF24L01_Wbuf(WRITE_REG + RX_ADDR_P0, RX_ADDRESS, RX_ADR_WIDTH); // 写接收端地址
    NRF24L01_Wreg(WRITE_REG + EN_AA, 0x01);      //  频道0自动	ACK应答允许
    NRF24L01_Wreg(WRITE_REG + EN_RXADDR, 0x01);  //  允许接收地址只有频道0，如果需要多频道可以参考Page21
    NRF24L01_Wreg(WRITE_REG + RF_CH, 0x40);        //   设置信道工作为2.4GHZ，收发必须一致
    NRF24L01_Wreg(WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH); //设置接收数据长度，本次设置为32字节
    NRF24L01_Wreg(WRITE_REG + RF_SETUP, 0x27);   		//设置发射速率为1MB/S，发射功率为最大值+7dB，由于有X2401L功放，实际+21dbm输出
}

/****************************************************************************************************/
/*函数：void SetRX_Mode(void)
/*功能：数据接收配置
/****************************************************************************************************/
void nRF24L01_SetMode(u8 tx)
{
	u8 i;

    NRF24L01_CE0;
	if(tx)
		NRF24L01_Wreg(WRITE_REG + CONFIG, 0x5e);
	else
    	NRF24L01_Wreg(WRITE_REG + CONFIG, 0x0f);
    	
	NRF24L01_CE1;//使能
	for(i=0;i<20;i++)DELAY_US(100);
}

/*
功能：NRF24L01读写寄存器函数
*/
u8 NRF24L01_Wreg(u8 addr, u8 value)
{
    u8 status;

    SPI()_CS(0);                   // CSN low, init SPI transaction
    status = SPI()_Byte(addr);      // select register
    SPI()_Byte(value);             // ..and write value to it..
    SPI()_CS(1);                   // CSN high again

    return(status);            // return nRF24L01 status uchar
}
/*
功能: 用于读数据，
addr：为寄存器地址，pBuf：为待读出数据地址，len：读出数据的个数
*/
u8 NRF24L01_Rbuf(u8 addr, u8 *pBuf, u8 len)
{
    u8 status, i;

    SPI()_CS(0);                    		// Set CSN low, init SPI tranaction
    status = SPI()_Byte(addr);       		// Select register to write to and read status uchar

    for(i = 0; i < len; i++)
        pBuf[i] = SPI()_Byte(0);    //

    SPI()_CS(1);

    return(status);                    // return nRF24L01 status uchar
}
/*
功能: 用于写数据：为寄存器地址，pBuf：为待写入数据地址，uchars：写入数据的个数
*/
u8 NRF24L01_Wbuf(u8 addr, u8 *pBuf, u8 len)
{
    u8 status, i;

    SPI()_CS(0);             //SPI使能
    status = SPI()_Byte(addr);
    for(i = 0; i < len; i++) //
        SPI()_Byte(pBuf[i]);
    SPI()_CS(1);           //关闭SPI
    return(status);    //
}

u8 sta;
/******************************************************************************************************/
/*函数：unsigned char nRF24L01_RxPacket(unsigned char* rx_buf)
/*功能：数据读取后放如rx_buf接收缓冲区中
/******************************************************************************************************/
u8 nRF24L01_RxPacket(u8 *rx_buf)
{
    u8 rx_ok = 0;
	
	//
    sta = NRF24L01_Wreg(STATUS,0);	// 读取状态寄存其来判断数据接收状况
    if(sta & ST_RX_DR)				// 判断是否接收到数据
    {
		//NRF24L01_CE0;//关闭接收
        NRF24L01_Rbuf(RD_RX_PLOAD, rx_buf, TX_PLOAD_WIDTH); 
        rx_ok = 1;			//读取数据完成标志
		//P05 = ~P05;
    }
	//接收到数据后RX_DR,TX_DS,MAX_PT都置高为1，通过写1来清
    NRF24L01_Wreg(WRITE_REG + STATUS, sta);
    //NRF24L01_CE1;
    return rx_ok;
}
/***********************************************************************************************************
/*函数：void nRF24L01_TxPacket(unsigned char * tx_buf)
/*功能：发送 tx_buf中数据
/**********************************************************************************************************/
void nRF24L01_TxPacket(u8 *tx_buf)
{
	NRF24L01_CE0;
    NRF24L01_Wreg(WRITE_REG + STATUS, 0xff);
    NRF24L01_Wreg(0xE1, 0xff);
	
    NRF24L01_Wbuf(WR_TX_PLOAD, tx_buf, TX_PLOAD_WIDTH);
    //NRF24L01_Wreg(WRITE_REG + CONFIG, 0x0e);
    NRF24L01_CE1;
    DELAY_US(20);   //CE高电平大于10us才能进入发射模式


}

void RX_Mode(void)
{
	NRF24L01_CE0;
  	NRF24L01_Wbuf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH);  // 接收设备接收通道0使用和发送设备相同的发送地址
  	NRF24L01_Wreg(WRITE_REG + EN_AA, 0x01);               // 使能接收通道0自动应答
  	NRF24L01_Wreg(WRITE_REG + EN_RXADDR, 0x01);           // 使能接收通道0
  	NRF24L01_Wreg(WRITE_REG + RF_CH, 40);                 // 选择射频通道0x40
  	NRF24L01_Wreg(WRITE_REG + RX_PW_P0, TX_PLOAD_WIDTH);  // 接收通道0选择和发送通道相同有效数据宽度
  	NRF24L01_Wreg(WRITE_REG + RF_SETUP, 0x07);            // 数据传输率1Mbps，发射功率0dBm，低噪声放大器增益
  	NRF24L01_Wreg(WRITE_REG + CONFIG, 0x0f);              // CRC使能，16位CRC校验，上电，接收模式
  	NRF24L01_CE1;                                            // 拉高CE启动接收设备
}


void TX_Mode(u8 * BUF)
{
	NRF24L01_CE0;
  	NRF24L01_Wbuf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);     // 写入发送地址
  	NRF24L01_Wbuf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH);  // 为了应答接收设备，接收通道0地址和发送地址相同
  	NRF24L01_Wbuf(WR_TX_PLOAD, BUF, TX_PLOAD_WIDTH);                  // 写数据包到TX FIFO
  	NRF24L01_Wreg(WRITE_REG + EN_AA, 0x01);       // 使能接收通道0自动应答
  	NRF24L01_Wreg(WRITE_REG + EN_RXADDR, 0x01);   // 使能接收通道0
  	NRF24L01_Wreg(WRITE_REG + SETUP_RETR, 0x0a);  // 自动重发延时等待250us+86us，自动重发10次
  	NRF24L01_Wreg(WRITE_REG + RF_CH, 40);         // 选择射频通道0x40
  	NRF24L01_Wreg(WRITE_REG + RF_SETUP, 0x07);    // 数据传输率1Mbps，发射功率0dBm，低噪声放大器增益
  	NRF24L01_Wreg(WRITE_REG + CONFIG, 0x0e);      // CRC使能，16位CRC校验，上电
	NRF24L01_CE1;
}