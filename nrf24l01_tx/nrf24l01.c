
#include "bsp/config.h"



//����SPI�Ĳ���IO

#define NRF24L01_CE0	P11 = 0	
#define NRF24L01_CE1	P11 = 1
#define SPI_SCK0		P10 = 0
#define SPI_SCK1		P10 = 1
#define SPI_MISO		P37
#define SPI_CS0			P36 = 0
#define SPI_CS1			P36 = 1
#define SPI_MOSI0		P33 = 0
#define SPI_MOSI1		P33 = 1


//SPI��С��ʱ��������spi������
#define SPI_DELAY		delay_us(10)
//����SPI����ţ������ε��ø��ļ�����Ҫ��֤��Ų��ظ�
#define SPI()				SPI ## 1
#include "io_spi/io_spi.h"


u8 const TX_ADDRESS[TX_ADR_WIDTH] = {0x34, 0x43, 0x10, 0x10, 0x01};	//���ص�ַ
u8 const RX_ADDRESS[RX_ADR_WIDTH] = {0x34, 0x43, 0x10, 0x10, 0x01};	//���յ�ַ


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
NRF24L01��ʼ��
*/
void nRF24L01_Init(void)
{
	u8 i;
	for(i=0;i<10;i++)DELAY_US(100);

	NRF24L01_CE0;
	SPI()_Init();
    
    NRF24L01_Wbuf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);    // д���ص�ַ
    NRF24L01_Wbuf(WRITE_REG + RX_ADDR_P0, RX_ADDRESS, RX_ADR_WIDTH); // д���ն˵�ַ
    NRF24L01_Wreg(WRITE_REG + EN_AA, 0x01);      //  Ƶ��0�Զ�	ACKӦ������
    NRF24L01_Wreg(WRITE_REG + EN_RXADDR, 0x01);  //  ������յ�ַֻ��Ƶ��0�������Ҫ��Ƶ�����Բο�Page21
    NRF24L01_Wreg(WRITE_REG + RF_CH, 0x40);        //   �����ŵ�����Ϊ2.4GHZ���շ�����һ��
    NRF24L01_Wreg(WRITE_REG + RX_PW_P0, RX_PLOAD_WIDTH); //���ý������ݳ��ȣ���������Ϊ32�ֽ�
    NRF24L01_Wreg(WRITE_REG + RF_SETUP, 0x27);   		//���÷�������Ϊ1MB/S�����书��Ϊ���ֵ+7dB��������X2401L���ţ�ʵ��+21dbm���
}

/****************************************************************************************************/
/*������void SetRX_Mode(void)
/*���ܣ����ݽ�������
/****************************************************************************************************/
void nRF24L01_SetMode(u8 tx)
{
	u8 i;

    NRF24L01_CE0;
	if(tx)
		NRF24L01_Wreg(WRITE_REG + CONFIG, 0x5e);
	else
    	NRF24L01_Wreg(WRITE_REG + CONFIG, 0x0f);
    	
	NRF24L01_CE1;//ʹ��
	for(i=0;i<20;i++)DELAY_US(100);
}

/*
���ܣ�NRF24L01��д�Ĵ�������
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
����: ���ڶ����ݣ�
addr��Ϊ�Ĵ�����ַ��pBuf��Ϊ���������ݵ�ַ��len���������ݵĸ���
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
����: ����д���ݣ�Ϊ�Ĵ�����ַ��pBuf��Ϊ��д�����ݵ�ַ��uchars��д�����ݵĸ���
*/
u8 NRF24L01_Wbuf(u8 addr, u8 *pBuf, u8 len)
{
    u8 status, i;

    SPI()_CS(0);             //SPIʹ��
    status = SPI()_Byte(addr);
    for(i = 0; i < len; i++) //
        SPI()_Byte(pBuf[i]);
    SPI()_CS(1);           //�ر�SPI
    return(status);    //
}

u8 sta;
/******************************************************************************************************/
/*������unsigned char nRF24L01_RxPacket(unsigned char* rx_buf)
/*���ܣ����ݶ�ȡ�����rx_buf���ջ�������
/******************************************************************************************************/
u8 nRF24L01_RxPacket(u8 *rx_buf)
{
    u8 rx_ok = 0;
	
	//
    sta = NRF24L01_Wreg(STATUS,0);	// ��ȡ״̬�Ĵ������ж����ݽ���״��
    if(sta & ST_RX_DR)				// �ж��Ƿ���յ�����
    {
		//NRF24L01_CE0;//�رս���
        NRF24L01_Rbuf(RD_RX_PLOAD, rx_buf, TX_PLOAD_WIDTH); 
        rx_ok = 1;			//��ȡ������ɱ�־
		//P05 = ~P05;
    }
	//���յ����ݺ�RX_DR,TX_DS,MAX_PT���ø�Ϊ1��ͨ��д1����
    NRF24L01_Wreg(WRITE_REG + STATUS, sta);
    //NRF24L01_CE1;
    return rx_ok;
}
/***********************************************************************************************************
/*������void nRF24L01_TxPacket(unsigned char * tx_buf)
/*���ܣ����� tx_buf������
/**********************************************************************************************************/
void nRF24L01_TxPacket(u8 *tx_buf)
{
	NRF24L01_CE0;
    NRF24L01_Wreg(WRITE_REG + STATUS, 0xff);
    NRF24L01_Wreg(0xE1, 0xff);
	
    NRF24L01_Wbuf(WR_TX_PLOAD, tx_buf, TX_PLOAD_WIDTH);
    //NRF24L01_Wreg(WRITE_REG + CONFIG, 0x0e);
    NRF24L01_CE1;
    DELAY_US(20);   //CE�ߵ�ƽ����10us���ܽ��뷢��ģʽ


}

void RX_Mode(void)
{
	NRF24L01_CE0;
  	NRF24L01_Wbuf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH);  // �����豸����ͨ��0ʹ�úͷ����豸��ͬ�ķ��͵�ַ
  	NRF24L01_Wreg(WRITE_REG + EN_AA, 0x01);               // ʹ�ܽ���ͨ��0�Զ�Ӧ��
  	NRF24L01_Wreg(WRITE_REG + EN_RXADDR, 0x01);           // ʹ�ܽ���ͨ��0
  	NRF24L01_Wreg(WRITE_REG + RF_CH, 40);                 // ѡ����Ƶͨ��0x40
  	NRF24L01_Wreg(WRITE_REG + RX_PW_P0, TX_PLOAD_WIDTH);  // ����ͨ��0ѡ��ͷ���ͨ����ͬ��Ч���ݿ��
  	NRF24L01_Wreg(WRITE_REG + RF_SETUP, 0x07);            // ���ݴ�����1Mbps�����书��0dBm���������Ŵ�������
  	NRF24L01_Wreg(WRITE_REG + CONFIG, 0x0f);              // CRCʹ�ܣ�16λCRCУ�飬�ϵ磬����ģʽ
  	NRF24L01_CE1;                                            // ����CE���������豸
}


void TX_Mode(u8 * BUF)
{
	NRF24L01_CE0;
  	NRF24L01_Wbuf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);     // д�뷢�͵�ַ
  	NRF24L01_Wbuf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH);  // Ϊ��Ӧ������豸������ͨ��0��ַ�ͷ��͵�ַ��ͬ
  	NRF24L01_Wbuf(WR_TX_PLOAD, BUF, TX_PLOAD_WIDTH);                  // д���ݰ���TX FIFO
  	NRF24L01_Wreg(WRITE_REG + EN_AA, 0x01);       // ʹ�ܽ���ͨ��0�Զ�Ӧ��
  	NRF24L01_Wreg(WRITE_REG + EN_RXADDR, 0x01);   // ʹ�ܽ���ͨ��0
  	NRF24L01_Wreg(WRITE_REG + SETUP_RETR, 0x0a);  // �Զ��ط���ʱ�ȴ�250us+86us���Զ��ط�10��
  	NRF24L01_Wreg(WRITE_REG + RF_CH, 40);         // ѡ����Ƶͨ��0x40
  	NRF24L01_Wreg(WRITE_REG + RF_SETUP, 0x07);    // ���ݴ�����1Mbps�����书��0dBm���������Ŵ�������
  	NRF24L01_Wreg(WRITE_REG + CONFIG, 0x0e);      // CRCʹ�ܣ�16λCRCУ�飬�ϵ�
	NRF24L01_CE1;
}