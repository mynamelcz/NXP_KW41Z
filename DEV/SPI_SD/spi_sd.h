#ifndef	__SPI_SD_H
#define	__SPI_SD_H
#include "common.h"
//#include "diskio.h"



#define SD_REV_SEC_NUM		0






typedef enum {
	SD_STA_OFF_LINE = 0,
	SD_STA_ON_LINE,
}SD_Dev_state;




typedef enum {
	SD_TYPE_UNKNOW,
	SD_TYPE_MMC3,
	SD_TYPE_V1_X,
    SD_TYPE_SDSC,   // <2G 
    SD_TYPE_SDHC,   //  2G ~ 32G
    SD_TYPE_SDXC,   // 32G ~ 2T
}SD_Type;



typedef enum {
	SD_NO_ERR,
    SD_ERR_CHECK_V1,
    SD_ERR_CHECK_V2V3,
	SD_ERR_SET_BLOCK_SIZE,
    SD_ERR_CHECK_TOKEN,
	SD_ERR_GET_CSD,
	SD_ERR_RES_FAILURE,
    SD_ERR_ACTIVE_CARD,
	SD_ERR_TIME_OUT,
    SD_ERR_CHECK_VOL,
    SD_ERR_GO_IDLE,
}spi_sd_err;

typedef struct
{
  volatile u8  CSDStruct;            /*!< CSD structure                         */
  volatile u8  SysSpecVersion;       /*!< System specification version          */
  volatile u8  Reserved1;            /*!< Reserved                              */
  volatile u8  TAAC;                 /*!< Data read access time 1               */
  volatile u8  NSAC;                 /*!< Data read access time 2 in CLK cycles */
  volatile u8  MaxBusClkFrec;        /*!< Max. bus clock frequency              */
  volatile u16 CardComdClasses;      /*!< Card command classes                  */
  volatile u8  RdBlockLen;           /*!< Max. read data block length           */
  volatile u8  PartBlockRead;        /*!< Partial blocks for read allowed       */
  volatile u8  WrBlockMisalign;      /*!< Write block misalignment              */
  volatile u8  RdBlockMisalign;      /*!< Read block misalignment               */
  volatile u8  DSRImpl;              /*!< DSR implemented                       */
  volatile u8  Reserved2;            /*!< Reserved                              */
  volatile u32 DeviceSize;           /*!< Device Size                           */
  volatile u8  MaxRdCurrentVDDMin;   /*!< Max. read current @ VDD min           */
  volatile u8  MaxRdCurrentVDDMax;   /*!< Max. read current @ VDD max           */
  volatile u8  MaxWrCurrentVDDMin;   /*!< Max. write current @ VDD min          */
  volatile u8  MaxWrCurrentVDDMax;   /*!< Max. write current @ VDD max          */
  volatile u8  DeviceSizeMul;        /*!< Device size multiplier                */
  volatile u8  EraseGrSize;          /*!< Erase group size                      */
  volatile u8  EraseGrMul;           /*!< Erase group size multiplier           */
  volatile u8  WrProtectGrSize;      /*!< Write protect group size              */
  volatile u8  WrProtectGrEnable;    /*!< Write protect group enable            */
  volatile u8  ManDeflECC;           /*!< Manufacturer default ECC              */
  volatile u8  WrSpeedFact;          /*!< Write speed factor                    */
  volatile u8  MaxWrBlockLen;        /*!< Max. write data block length          */
  volatile u8  WriteBlockPaPartial;  /*!< Partial blocks for write allowed      */
  volatile u8  Reserved3;            /*!< Reserved                              */
  volatile u8  ContentProtectAppli;  /*!< Content protection application        */
  volatile u8  FileFormatGroup;     /*!< File format group                     */
  volatile u8  CopyFlag;             /*!< Copy flag (OTP)                       */
  volatile u8  PermWrProtect;        /*!< Permanent write protection            */
  volatile u8  TempWrProtect;        /*!< Temporary write protection            */
  volatile u8  FileFormat;           /*!< File format                           */
  volatile u8  ECC;                  /*!< ECC code                              */
  volatile u8  CSD_CRC;              /*!< CSD CRC                               */
  volatile u8  Reserved4;            /*!< Always 1                              */
  
}SD_CSD_T;

typedef struct
{
  volatile u8  ManufacturerID;  /*!< Manufacturer ID       */
  volatile u16 OEM_AppliID;     /*!< OEM/Application ID    */
  volatile u32 ProdName1;       /*!< Product Name part1    */
  volatile u8  ProdName2;       /*!< Product Name part2    */
  volatile u8  ProdRev;         /*!< Product Revision      */
  volatile u32 ProdSN;          /*!< Product Serial Number */
  volatile u8  Reserved1;       /*!< Reserved1             */
  volatile u16 ManufactDate;    /*!< Manufacturing Date    */
  volatile u8  CID_CRC;         /*!< CID CRC               */
  volatile u8  Reserved2;       /*!< Always 1              */

}SD_CID_T;














/*============== SD Respond============*/
#define	SD_SPI_R1_LEN		1
#define	SD_SPI_R2_LEN		2
#define	SD_SPI_R3_LEN		5
#define	SD_SPI_R7_LEN		5
typedef enum{	
	SD_SPI_R1 = 1,
	SD_SPI_R2 = 2,
	SD_SPI_R3 = 3,
	SD_SPI_R7 = 7,
}SD_SPI_Rx_type;



/** SPI MODE R1   8 bits **/
#define SPI_R1_IDLE_STATE   	BIT(0)
#define SPI_R1_ERASE_RESET    	BIT(1)
#define SPI_R1_ILL_CMD      	BIT(2)
#define SPI_R1_CMD_CRC_ERR  	BIT(3)
#define SPI_R1_ERASE_ERR   		BIT(4)
#define SPI_R1_ADDR_ERR		   	BIT(5)
#define SPI_R1_PARAM_ERR	   	BIT(6)
#define SPI_R1_ALWAYS_0  	 	BIT(7)


/***   IO CTROL CMD ***/
typedef enum{
	/* file sys cmd */
//	SD_CTRL_SYNC 		= CTRL_SYNC,
//	SD_GET_SECTOR_COUNT	= GET_SECTOR_COUNT,
//	SD_GET_SECTOR_SIZE  = GET_SECTOR_SIZE,
//    SD_GET_BLOCK_SIZE	= GET_BLOCK_SIZE,
//	SD_CTRL_TRIM	    = CTRL_TRIM,
//	SD_CTRL_POWER		= CTRL_POWER,
//    SD_CTRL_LOCK		= CTRL_LOCK,
//	SD_CTRL_EJECT		= CTRL_EJECT,
//	SD_CTRL_FORMAT		= CTRL_FORMAT,
	/* user cmd */
	SD_CTRL_GET_SIZE, 	   //unit KB 
	SD_CTRL_CHIP_ERASE,
	SD_CTRL_POWER_ON,
	SD_CTRL_POWER_OFF,
}SD_CTR_CMD;





/******* CONFIG	**********/
#define DRIVER_SD_SPI 			Driver_SPI0
#define SD_SPI_DMA_MUX_BASEADDR    DMAMUX0
#define SD_SPI_DMA_BASEADDR 		DMA0

#define SD_CLK_SPEED_HIGH	(20*1000*1000)		// max:25M
#define SD_CLK_SPEED_LOW	(180*1000)			// max:400k

#define MAX_RETRY_TIME			0xff
#define HOST_SUPPORT_HIGH_CAP	1				//if host supports high capacity HCS=1, else HCS = 0 
#define SD_BLOCK_SIZE			512

#define	SPI_SET_SPEED_CMD ARM_SPI_SET_BUS_SPEED

typedef struct{
	void (*init)(void);
	void (*cs_str)(u8 en);
	void (*read)(u8 *buf, u32 len);
	void (*write)(const u8 *buf, u32 len);
	u8   (*w_r_byte)(u8 dat);
    u8  (*io_ctr)(u32 cmd, void *buf);	
}__spi_ctr_obj;

typedef struct
{	
	SD_Type     	type;
	u8				dev_state;
	SD_CSD_T 		CSD;
	SD_CID_T 		CID;
    unsigned int   capacity;  /*!< Card Capacity */
	u32 			block_sz;
	
}__sd_inf_t;



typedef struct
{
	__spi_ctr_obj  *hd_io;
	__sd_inf_t		sd_inf;
	
	spi_sd_err (*init)(__spi_ctr_obj *hd_io);
    u8  (*status)(void);
	u8  (*read)(u8 *buf, u32 start_blk, u16 blk_bum);
	u8  (*write)(const u8 *buf,u32 start_blk, u16 blk_bum);
	void (*erase)(u32 start_sec, u32 sct_num);
	bool (*io_ctr)(u8 cmd, void *buff);		

}__spi_sd_obj;

extern __spi_ctr_obj spi_obj;
extern __spi_sd_obj spi_sd_obj;

#endif



