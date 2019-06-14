#include "spi_sd.h"
#include "fsl_dspi_cmsis.h"
#include "fsl_gpio.h"
#include "board.h"
#include "fsl_port.h"
#include "pin_mux.h"


/*******	DATA TRAN	TOKEN 	*******/
#define SD_START_DATA_SINGLE_BLOCK_READ    0xFE  //	Start Single Block Read 
#define SD_START_DATA_MULTIPLE_BLOCK_READ  0xFE  // Start Multiple Block Read 
#define SD_START_DATA_SINGLE_BLOCK_WRITE   0xFE  //	Start Single Block Write 
#define SD_START_DATA_MULTIPLE_BLOCK_WRITE 0xFC  // Start Multiple Block Write 
#define SD_STOP_DATA_MULTIPLE_BLOCK_WRITE  0xFD  // Stop Multiple Block Write 



/*******	SD 	CMD 	 	*******/
#define SD_CMD_GO_IDLE_STATE          0   /*!< CMD0 = 0x40 */
#define SD_CMD_SEND_OP_COND           1   /*!< CMD1 = 0x41 */
#define SD_CMD_IF_COND                8   /*!< CMD8 = 0x48 */
#define SD_CMD_SEND_CSD               9   /*!< CMD9 = 0x49 */
#define SD_CMD_SEND_CID               10  /*!< CMD10 = 0x4A */
#define SD_CMD_STOP_TRANSMISSION      12  /*!< CMD12 = 0x4C */
#define SD_CMD_SEND_STATUS            13  /*!< CMD13 = 0x4D */
#define SD_CMD_SET_BLOCKLEN           16  /*!< CMD16 = 0x50 */
#define SD_CMD_READ_SINGLE_BLOCK      17  /*!< CMD17 = 0x51 */
#define SD_CMD_READ_MULTIPLE_BLOCK    18  /*!< CMD18 = 0x52 */
#define SD_CMD_SEND_NUM_WR_BLOCKS     22  /*!< CMD22 = 0x56 */
#define SD_CMD_SET_BLOCK_COUNT        23  /*!< CMD23 = 0x57 */
#define SD_CMD_WRITE_SINGLE_BLOCK     24  /*!< CMD24 = 0x58 */
#define SD_CMD_WRITE_MULTIPLE_BLOCK   25  /*!< CMD25 = 0x59 */
#define SD_CMD_PROG_CSD               27  /*!< CMD27 = 0x5B */
#define SD_CMD_SET_WRITE_PROT         28  /*!< CMD28 = 0x5C */
#define SD_CMD_CLR_WRITE_PROT         29  /*!< CMD29 = 0x5D */
#define SD_CMD_SEND_WRITE_PROT        30  /*!< CMD30 = 0x5E */
#define SD_CMD_SD_ERASE_GRP_START     32  /*!< CMD32 = 0x60 */
#define SD_CMD_SD_ERASE_GRP_END       33  /*!< CMD33 = 0x61 */
#define SD_CMD_UNTAG_SECTOR           34  /*!< CMD34 = 0x62 */
#define SD_CMD_ERASE_GRP_START        35  /*!< CMD35 = 0x63 */
#define SD_CMD_ERASE_GRP_END          36  /*!< CMD36 = 0x64 */
#define SD_CMD_UNTAG_ERASE_GROUP      37  /*!< CMD37 = 0x65 */
#define SD_CMD_ERASE                  38  /*!< CMD38 = 0x66 */
//------------------------------------------------------------
#define SD_ACMD_SEND_OP_COND     	  41  /*!< ACMD41 */
#define SD_ACMD_SET_CLR_CARD_DETECT   42  /*!< ACMD42= 0x6A */
#define SD_CMD_APP_CMD                55  /*!< CMD55 = 0x77 */
#define SD_CMD_READ_OCR               58  /*!< CMD58 = 0x7A */
#define SD_CMD_CRC_ON_OFF             59  /*!< CMD59 = 0x7B */





static __spi_sd_obj *this_obj = NULL;

static u8 sd_speed_set(u32 speed)
{
	u8 res  = 0;
	res = this_obj->hd_io->io_ctr(SPI_SET_SPEED_CMD, (void *)&speed);	
	return res;
}


static u8 sd_get_state(void)
{
	return this_obj->sd_inf.dev_state;
}
static void sd_set_state(SD_Dev_state state)
{
	this_obj->sd_inf.dev_state = state;
}


static u8 sd_wait_ready(void)  
{  
    u32 t=0;  
	u8 dat = 0;
    do{  
		this_obj->hd_io->read(&dat, 1);
        if(dat == 0xff)return 0;
		//sd_printf("sd wait:0x%x t:%d\n",dat, t);
    }while(++t < 0xfff);
	ERR_printf(0);
    return 0xff;  
}  
  
static void sd_dis_select(void)  
{  
	u8 dat = 0;
    this_obj->hd_io->cs_str(1); 
    this_obj->hd_io->read(&dat, 1);
}  

static u8 sd_select(void)  
{  
	u8 res = 0;
    this_obj->hd_io->cs_str(0);   
    res = sd_wait_ready();
	if(res == 0xff)
		sd_dis_select();  
    return res; 
}  

static u8 sd_cmd_r1(u8 Cmd, u32 Arg, u8 Crc)
{
	u8 retry = 0xf;
	u8 cmd_fream[6];
	u8 r1 = 0xff;
    sd_dis_select();
	r1 = sd_select();
	if(r1) return r1;
	
	cmd_fream[0] = (Cmd | 0x40); 
	cmd_fream[1] = (u8)(Arg >> 24); 
	cmd_fream[2] = (u8)(Arg >> 16); 
	cmd_fream[3] = (u8)(Arg >> 8);
	cmd_fream[4] = (u8)(Arg); 
	cmd_fream[5] = (Crc | 0x01); 
    
	sd_printf("SD CMD%d: ",Cmd);
	sd_puthex((const char *)cmd_fream,6);
	

	retry = 100; 
	
	
	this_obj->hd_io->write(cmd_fream, 6);
	do
	{	
		this_obj->hd_io->read(&r1, SD_SPI_R1_LEN);
	}while ((r1 & SPI_R1_ALWAYS_0) && --retry); 

	if(retry == 0){
	    ERR_printf(0);
		return 0xff;
	}
	return r1;
}




static u8 sd_cmd(u8 Rx, u8 Cmd, u32 Arg, u8 Crc, u8 *r_buf)
{
	u8 r1 = 0xff;
	u8 len = 0;
	switch(Rx){
		case SD_SPI_R1:
		     len = SD_SPI_R1_LEN-1;
		     break;
		case SD_SPI_R2:
			 len = SD_SPI_R2_LEN-1;
			break;
		case SD_SPI_R3:
			len = SD_SPI_R3_LEN-1;
			break;
		case SD_SPI_R7:
			len = SD_SPI_R7_LEN-1;
			break;
		default:
			break;
	}
	 r1 = sd_cmd_r1(Cmd, Arg, Crc);
	 r_buf[0] = r1;
	 if(len){
		this_obj->hd_io->read(&(r_buf[1]), len);		 
	 }
	sd_printf("R%d: ",Rx);
	sd_puthex((const char *)r_buf, len+1);
	return r1;
}


//根据SPI时钟频率作相应修改
#define INIT_RETRY_MAX    400000
#define READ_BLOCK_MAX    200000 //200ms
#define WAIT_READY_MAX    500000 //500ms

spi_sd_err sd_check_res_token(u8 token)
{
	u32 retry = WAIT_READY_MAX; 
	// Check if response is got or a timeout is happen
	while ((this_obj->hd_io->w_r_byte(0xFF) != token) && --retry);
	if (retry){
		return SD_NO_ERR; // Right response got
	}else{
		ERR_printf(SD_ERR_CHECK_TOKEN);
		return SD_ERR_CHECK_TOKEN; // After time out
	}
}





static spi_sd_err sd_get_csd_reg(SD_CSD_T *sd_csd)
{
	spi_sd_err res = SD_NO_ERR;
	SD_Type sd_type = this_obj->sd_inf.type;
	u8 r1;
	u8 CSD_Tab[16];
	sd_cmd(SD_SPI_R1, SD_CMD_SEND_CSD, 0x00, 0xff, &r1);
	if(r1 != 0){
		sd_dis_select();
	    ERR_printf(0);
		return SD_ERR_GET_CSD;
	}
	res = sd_check_res_token(SD_START_DATA_SINGLE_BLOCK_READ);
	if(res != SD_NO_ERR){
		ERR_printf(0);
		return res;
	}
	this_obj->hd_io->read(CSD_Tab, 16);
	sd_dis_select();
	sd_printf("CSD: ");
	sd_puthex((const char *)CSD_Tab, 16);
	
	/*!< Byte 0 */	
	sd_csd->CSDStruct = (CSD_Tab[0] & 0xC0) >> 6;
	sd_csd->SysSpecVersion = (CSD_Tab[0] & 0x3C) >> 2;		// ????
	sd_csd->Reserved1 = CSD_Tab[0] & 0x03;					// ????
	

	/*!< Byte 1 */
	sd_csd->TAAC = CSD_Tab[1];

	/*!< Byte 2 */
	sd_csd->NSAC = CSD_Tab[2];

	/*!< Byte 3 */
	sd_csd->MaxBusClkFrec = CSD_Tab[3];

	/*!< Byte 4 */
	sd_csd->CardComdClasses = CSD_Tab[4] << 4;

	/*!< Byte 5 */
	sd_csd->CardComdClasses |= (CSD_Tab[5] & 0xF0) >> 4;
	sd_csd->RdBlockLen = CSD_Tab[5] & 0x0F;
	
	/*!< Byte 6 */
	sd_csd->PartBlockRead = (CSD_Tab[6] & 0x80) >> 7;
	sd_csd->WrBlockMisalign = (CSD_Tab[6] & 0x40) >> 6;
	sd_csd->RdBlockMisalign = (CSD_Tab[6] & 0x20) >> 5;
	sd_csd->DSRImpl = (CSD_Tab[6] & 0x10) >> 4;
	sd_csd->Reserved2 = 0; /*!< Reserved */

	if (sd_type == SD_TYPE_SDHC) // or SDXC CSD v2.0
	{
		/*!< Byte 7 */
        sd_csd->DeviceSize = (CSD_Tab[7] & 0x3F) << 16;

        /*!< Byte 8 */
        sd_csd->DeviceSize |= (CSD_Tab[8] << 8);

        /*!< Byte 9 */
        sd_csd->DeviceSize |= (CSD_Tab[9]);
	}
	else if (sd_type == SD_TYPE_V1_X || sd_type == SD_TYPE_SDSC) //CSD v1.0
	{
		sd_csd->DeviceSize = (CSD_Tab[6] & 0x03) << 10;
		/*!< Byte 7 */
		sd_csd->DeviceSize |= (CSD_Tab[7]) << 2;

		/*!< Byte 8 */
		sd_csd->DeviceSize |= (CSD_Tab[8] & 0xC0) >> 6;

		sd_csd->MaxRdCurrentVDDMin = (CSD_Tab[8] & 0x38) >> 3;
		sd_csd->MaxRdCurrentVDDMax = (CSD_Tab[8] & 0x07);

		/*!< Byte 9 */
		sd_csd->MaxWrCurrentVDDMin = (CSD_Tab[9] & 0xE0) >> 5;
		sd_csd->MaxWrCurrentVDDMax = (CSD_Tab[9] & 0x1C) >> 2;
		sd_csd->DeviceSizeMul = (CSD_Tab[9] & 0x03) << 1;

		/*!< Byte 10 */
		sd_csd->DeviceSizeMul |= (CSD_Tab[10] & 0x80) >> 7;
	}
	
	sd_csd->EraseGrSize = (CSD_Tab[10] & 0x40) >> 6;
	sd_csd->EraseGrMul = (CSD_Tab[10] & 0x3F) << 1;

	/*!< Byte 11 */
	sd_csd->EraseGrMul |= (CSD_Tab[11] & 0x80) >> 7;
	sd_csd->WrProtectGrSize = (CSD_Tab[11] & 0x7F);

	/*!< Byte 12 */
	sd_csd->WrProtectGrEnable = (CSD_Tab[12] & 0x80) >> 7;
	sd_csd->ManDeflECC = (CSD_Tab[12] & 0x60) >> 5;
	sd_csd->WrSpeedFact = (CSD_Tab[12] & 0x1C) >> 2;
	sd_csd->MaxWrBlockLen = (CSD_Tab[12] & 0x03) << 2;

	/*!< Byte 13 */
	sd_csd->MaxWrBlockLen |= (CSD_Tab[13] & 0xC0) >> 6;
	sd_csd->WriteBlockPaPartial = (CSD_Tab[13] & 0x20) >> 5;
	sd_csd->Reserved3 = 0;
	sd_csd->ContentProtectAppli = (CSD_Tab[13] & 0x01);

	/*!< Byte 14 */
	sd_csd->FileFormatGroup = (CSD_Tab[14] & 0x80) >> 7;
	sd_csd->CopyFlag = (CSD_Tab[14] & 0x40) >> 6;
	sd_csd->PermWrProtect = (CSD_Tab[14] & 0x20) >> 5;
	sd_csd->TempWrProtect = (CSD_Tab[14] & 0x10) >> 4;
	sd_csd->FileFormat = (CSD_Tab[14] & 0x0C) >> 2;
	sd_csd->ECC = (CSD_Tab[14] & 0x03);

	/*!< Byte 15 */
	sd_csd->CSD_CRC = (CSD_Tab[15] & 0xFE) >> 1;
	sd_csd->Reserved4 = 1;
	
//	sd_printf("CSDStruct:\t0x%x \n",sd_csd->CSDStruct);
//	sd_printf("SysSpecVersion:\t0x%x \n",sd_csd->SysSpecVersion);
//	sd_printf("Reserved1\t0x%x \n",sd_csd->Reserved1);

//	sd_printf("TAAC:\t\t0x%x \n",sd_csd->TAAC);
//	sd_printf("NSAC:\t\t0x%x \n",sd_csd->NSAC);
//	sd_printf("MaxBusClkFrec\t0x%x \n",sd_csd->MaxBusClkFrec);	//0x32:25M 0x5A:50M 

//	sd_printf("CardComdClasses\t0x%x \n",sd_csd->CardComdClasses);	
//	sd_printf("RdBlockLen\t0x%x \n",sd_csd->RdBlockLen);	

//	sd_printf("PartBlockRead\t0x%x \n",sd_csd->PartBlockRead);
//	sd_printf("WrBlockMisalign\t0x%x \n",sd_csd->WrBlockMisalign);
//	sd_printf("RdBlockMisalign\t0x%x \n",sd_csd->RdBlockMisalign);
//	sd_printf("DSRImpl  \t0x%x \n",sd_csd->DSRImpl);
//	sd_printf("Reserved2\t0x%x \n",sd_csd->Reserved2);

//	sd_printf("DeviceSize\t0x%x \n",sd_csd->DeviceSize);
//	sd_printf("MaxRdCurrentVDDMin\t0x%x \n",sd_csd->MaxRdCurrentVDDMin);
//	sd_printf("MaxWrCurrentVDDMax\t0x%x \n",sd_csd->MaxWrCurrentVDDMax);
//	sd_printf("DeviceSizeMul\t0x%x \n",sd_csd->DeviceSizeMul);


	return res;

}

#if 0
static spi_sd_err sd_get_cid_reg(SD_CID_T *sd_cid)
{
	spi_sd_err res = SD_NO_ERR;

	u8 r1;
	u8 CID_Tab[16];
	sd_cmd(SD_SPI_R1, SD_CMD_SEND_CID, 0x00, 0xff, &r1);
	if(r1 != 0){
		sd_dis_select();
	    ERR_printf(0);
		return SD_ERR_GET_CSD;
	}
	res = sd_check_res_token(SD_START_DATA_SINGLE_BLOCK_READ);
	if(res != SD_NO_ERR){
		return res;
	}
	this_obj->hd_io->read(CID_Tab, 16);
	sd_dis_select();
	sd_printf("CID: ");
	sd_puthex((const char *)CID_Tab, 16);
	
	
	/*!< Byte 0 */
	sd_cid->ManufacturerID = CID_Tab[0];

	/*!< Byte 1 */
	sd_cid->OEM_AppliID = CID_Tab[1] << 8;

	/*!< Byte 2 */
	sd_cid->OEM_AppliID |= CID_Tab[2];

	/*!< Byte 3 */
	sd_cid->ProdName1 = CID_Tab[3] << 24;

	/*!< Byte 4 */
	sd_cid->ProdName1 |= CID_Tab[4] << 16;

	/*!< Byte 5 */
	sd_cid->ProdName1 |= CID_Tab[5] << 8;

	/*!< Byte 6 */
	sd_cid->ProdName1 |= CID_Tab[6];

	/*!< Byte 7 */
	sd_cid->ProdName2 = CID_Tab[7];

	/*!< Byte 8 */
	sd_cid->ProdRev = CID_Tab[8];

	/*!< Byte 9 */
	sd_cid->ProdSN = CID_Tab[9] << 24;

	/*!< Byte 10 */
	sd_cid->ProdSN |= CID_Tab[10] << 16;

	/*!< Byte 11 */
	sd_cid->ProdSN |= CID_Tab[11] << 8;

	/*!< Byte 12 */
	sd_cid->ProdSN |= CID_Tab[12];

	/*!< Byte 13 */
	sd_cid->Reserved1 |= (CID_Tab[13] & 0xF0) >> 4;
	sd_cid->ManufactDate = (CID_Tab[13] & 0x0F) << 8;

	/*!< Byte 14 */
	sd_cid->ManufactDate |= CID_Tab[14];

	/*!< Byte 15 */
	sd_cid->CID_CRC = (CID_Tab[15] & 0xFE) >> 1;
	sd_cid->Reserved2 = 1;

//	sd_printf("ManufacturerID\t0x%x \n",sd_cid->ManufacturerID);
//	sd_printf("OEM_AppliID\t0x%x \n",sd_cid->OEM_AppliID);
//	sd_printf("ProdName1\t0x%x \n",sd_cid->ProdName1);
//	sd_printf("ProdName2\t0x%x \n",sd_cid->ProdName2);
//	sd_printf("ProdRev    \t0x%x \n",sd_cid->ProdRev);
//	sd_printf("ProdSN    \t0x%x \n",sd_cid->ProdSN);
//	sd_printf("Reserved1\t0x%x \n",sd_cid->Reserved1);
//	sd_printf("CID_CRC    \t0x%x \n",sd_cid->CID_CRC);
//	sd_printf("Reserved2\t0x%x \n",sd_cid->Reserved2);

	/*!< Return the reponse */
	return res;	
}	

#endif



spi_sd_err sd_getcard_inf(__sd_inf_t *sd_inf)
{
	spi_sd_err res = SD_NO_ERR;
	SD_Type sd_type = sd_inf->type;
	res = sd_get_csd_reg(&(this_obj->sd_inf.CSD));
//	res = sd_get_cid_reg(&(this_obj->sd_inf.CID));
	if (sd_type == SD_TYPE_SDHC)
	{
		sd_inf->capacity = ((unsigned long long)sd_inf->CSD.DeviceSize + 1) * 512;
        sd_inf->block_sz = 512;
	}
	else if (sd_type == SD_TYPE_SDSC || sd_type == SD_TYPE_V1_X)
	{
		sd_inf->capacity = (sd_inf->CSD.DeviceSize + 1);
		sd_inf->capacity *= (1 << (sd_inf->CSD.DeviceSizeMul + 2));
		sd_inf->block_sz = 1 << (sd_inf->CSD.RdBlockLen);
		sd_inf->capacity *= sd_inf->block_sz;
		
		sd_inf->capacity >>= 10;
	}
	sd_printf("DeviceSize\t0x%x \n",sd_inf->CSD.DeviceSize);
	sd_printf("block_sz\t0x%x \n",sd_inf->block_sz);
	sd_printf("capacity\t %d  Kb\n",sd_inf->capacity);
	sd_printf("Reserved2\t0x%x \n",sd_inf->type);


	
	
	return res;
}
	
	




static spi_sd_err sd_idle_mode(void)
{
	u8 tmp_buf[15];
	u8 retry = 12;
	u8 r1 = 0xff;
	//0: Reset card  >74  clk **/
	sd_speed_set(SD_CLK_SPEED_LOW);

    this_obj->hd_io->cs_str(1);	

	this_obj->hd_io->read(tmp_buf, 10);
	do{// CMD0--> idle mode **/
		r1 = sd_cmd(SD_SPI_R1, SD_CMD_GO_IDLE_STATE, 0x00, 0x95, &r1);
	}while((r1 != SPI_R1_IDLE_STATE)&&(--retry));
	if(retry == 0){
	    ERR_printf(0);
		return SD_ERR_GO_IDLE;
	}
	return SD_NO_ERR;
}



static u32 block_convey_addr(u32 block)
{
	if((spi_sd_obj.sd_inf.type == SD_TYPE_SDHC)||
	   (spi_sd_obj.sd_inf.type == SD_TYPE_SDXC)){
		return block;
	}
	return (block * spi_sd_obj.sd_inf.block_sz);
}




static spi_sd_err identification_v1_card(void)
{
	u8 retry, r1;
	u8 rx_buf[5] = {0};
    this_obj->hd_io->w_r_byte(0xff);			//提高可靠性
	this_obj->hd_io->w_r_byte(0xff);
    retry = MAX_RETRY_TIME;		 
	do{															// ACMD41: 1.send host capacity support information  2.Active teh card
		r1 = sd_cmd(SD_SPI_R1, SD_CMD_APP_CMD, 0, 0xFF, rx_buf);
#if(HOST_SUPPORT_HIGH_CAP)
		r1 = sd_cmd(SD_SPI_R1, SD_ACMD_SEND_OP_COND, 0x40000000, 0xFF, rx_buf);			// HCS = 1 
#else			
		r1 = sd_cmd(SD_SPI_R1, SD_ACMD_SEND_OP_COND, 0x00000000, 0xFF, rx_buf);			// HCS = 0  
#endif			
	}while(r1 && --retry);		 
	if(r1 != 0){
        ERR_printf(0);
		this_obj->sd_inf.type = SD_TYPE_UNKNOW;	
        return SD_ERR_CHECK_V1;		
	}
	sd_printf(">>>> SD TYPE V1.x ==<<<<<<\n");
	this_obj->sd_inf.type = SD_TYPE_V1_X;
	return SD_NO_ERR;
}


static spi_sd_err identification_v2v3(void)
{
	u32 retry, r1;
	u8 rx_buf[5] = {0};
    this_obj->hd_io->w_r_byte(0xff);			//提高可靠性
	this_obj->hd_io->w_r_byte(0xff);
	this_obj->hd_io->w_r_byte(0xff);
	this_obj->hd_io->w_r_byte(0xff);
	this_obj->hd_io->w_r_byte(0xff);
    retry = MAX_RETRY_TIME;		 
	do{// ACMD41: 1.send host capacity support information 2.Active teh card
		r1 = sd_cmd(SD_SPI_R1, SD_CMD_APP_CMD, 0, 0xFF, rx_buf);
#if(HOST_SUPPORT_HIGH_CAP)
		r1 = sd_cmd(SD_SPI_R1, SD_ACMD_SEND_OP_COND, 0x40000000, 0xFF, rx_buf);			// HCS = 1 
#else			
		r1 = sd_cmd(SD_SPI_R1, SD_ACMD_SEND_OP_COND, 0x00000000, 0xFF, rx_buf);			// HCS = 0  
#endif			
	}while(r1 && --retry);
	if(r1 != 0){
        ERR_printf(0);
		return  SD_ERR_CHECK_V2V3;			
	}	 
	r1 = sd_cmd(SD_SPI_R3, SD_CMD_READ_OCR, 0, 0XFF, rx_buf);	// CMD58:  GET  CCS		 
	if(rx_buf[1] & 0x40){
		sd_printf(">>>> SDHC or SDXC card ==<<<<<<\n");
		this_obj->sd_inf.type = SD_TYPE_SDHC;
		return SD_NO_ERR;			
	}else{
		sd_printf(">>>> SDSC  card ==<<<<<<\n");
	    this_obj->sd_inf.type = SD_TYPE_SDSC;
		return SD_NO_ERR;	
	} 		
}
static spi_sd_err sd_identification(void)
{
	u8 r1;
	u8 rx_buf[5] = {0};
	spi_sd_err res = SD_NO_ERR;
    res = sd_idle_mode();
	if(res != SD_NO_ERR){
        ERR_printf(0);
		return res;		
	}

	r1 = sd_cmd(SD_SPI_R7, SD_CMD_IF_COND, 0x01AA, 0x87, rx_buf);	//CMD8: check 2.7v~3.6v
	
	if(r1&SPI_R1_ILL_CMD){
		res = identification_v1_card();
	 }else if((rx_buf[3] == 0x01)&&(rx_buf[4]==0xAA)){
		res = identification_v2v3();
	 }else{
        res = identification_v2v3();
	 }
	 return res;
}



static bool sd_io_control(u8 cmd, void *buff)
{
    u8 res = false;
    switch(cmd){
/* Generic command (Used by FatFs) */
//        case SD_CTRL_SYNC:			//make sure write end
//			res = true;			    //default ok;  因为写入时已经有等待写入完成，所以这里默认ok
//            break;
//        case SD_GET_SECTOR_COUNT:	
//            *((u32 *)buff) = this_obj->sd_inf.capacity/ this_obj->sd_inf.block_sz;
//            res = true;
//            break;
//        case SD_GET_SECTOR_SIZE:
//            *((u32 *)buff) = (u32)this_obj->sd_inf.block_sz;
//            res = true;
//            break;
//        case SD_GET_BLOCK_SIZE:
//            *((u32 *)buff) = 1;
//            res = true;
//            break;
//        case SD_CTRL_TRIM:
//            break;
///* Generic command (Not used by FatFs) */
//        case SD_CTRL_POWER:
//            break;
//        case SD_CTRL_LOCK:
//            break;
//        case SD_CTRL_EJECT:
//            break;
//        case SD_CTRL_FORMAT:
//            break;
/*    user cmd   */		
		case SD_CTRL_GET_SIZE:
			break;
		case SD_CTRL_POWER_ON:
			break;	
		case SD_CTRL_POWER_OFF:
			break;			
		default:
			break;

    }
    if(res == false){
        ERR_printf(0);
    }
    return res;
} 





/*============	读写函数	=============*/
/*
*	1. addr shall aligned to block boundary
	   a. SDHC clock is 512-Byte
	   b. addr  在SDHC中以(block) 512-byte为单位, 
		    	在SDSC中以byte为单位。
*/
static u8 sd_read_blk(u8 *buf, u32 start_blk, u16 blk_bum)
{
	ASSERT(buf);
	u32 retry = MAX_RETRY_TIME;
	u8 res = 0xff;
	u16 crc = 0;
	u8 *ptr = buf;
	u16 blk_sz = this_obj->sd_inf.block_sz;
	
	u32 addr = 0;
	addr = block_convey_addr(start_blk);
	if(blk_bum ==1){
		sd_cmd(SD_SPI_R1, SD_CMD_READ_SINGLE_BLOCK, addr, 0xFF, &res);
		if(res != 0){
	        ERR_printf(0);
			return res;
		}
		res = sd_check_res_token(SD_START_DATA_SINGLE_BLOCK_READ);
		if(res == SD_NO_ERR){
			this_obj->hd_io->read(buf,blk_sz);
			this_obj->hd_io->read((u8 *)(&crc),sizeof(crc));
		}
		sd_dis_select();
		return res;	
	}else if(blk_bum > 1){
	
		sd_cmd(SD_SPI_R1, SD_CMD_READ_MULTIPLE_BLOCK, addr, 0xFF, &res);
		if(res != 0){
	        ERR_printf(0);
			return res;
		}
		while(blk_bum--){
			res = sd_check_res_token(SD_START_DATA_MULTIPLE_BLOCK_READ);
			if(res == SD_NO_ERR){
				this_obj->hd_io->read(ptr,blk_sz);
				this_obj->hd_io->read((u8 *)(&crc),sizeof(crc));
				ptr += blk_sz;
			}else{
				ERR_printf(0);
			}
		}
		do{
			sd_cmd(SD_SPI_R1, SD_CMD_STOP_TRANSMISSION, 0x00, 0xFF, &res);
		}while((res==0)&&(--retry));		// res:0 indicate sd is not busy
		if(retry == 0){
            
			ERR_printf(0);
			res = 0xff;
		}
		sd_dis_select();
		return res;	
	}else{;}
	return res;	
}


static u8 sd_write_blk(const u8 *buf, u32 start_blk, u16 blk_bum)
{
	ASSERT(buf);
	u8 res = 0xff;
	u8 data_res = 0;
	u16 crc = 0xffff;
    const u8 * ptr = buf;
	u16 blk_sz = this_obj->sd_inf.block_sz;
	
	u32 addr = 0;
	addr = block_convey_addr(start_blk);
	
	if(blk_bum==1){
		sd_cmd(SD_SPI_R1, SD_CMD_WRITE_SINGLE_BLOCK, addr, 0xFF, &res);		//CMD24
		if(res != 0){
            ERR_printf(0);
			return res;
		}	
		this_obj->hd_io->w_r_byte(SD_START_DATA_SINGLE_BLOCK_WRITE);		//token
		this_obj->hd_io->write(buf, blk_sz);								//data
		this_obj->hd_io->write((u8 *)(&crc),sizeof(crc));					//crc
		this_obj->hd_io->read(&data_res , 1);								//data res
		if(data_res & 0x5){
			res = 0;	
		}else{
            ERR_printf(0);
			res = 0xff;				
		}
		sd_dis_select();
		return res;
		
	}else if(blk_bum >1){
		sd_cmd(SD_SPI_R1, SD_CMD_WRITE_MULTIPLE_BLOCK, 0, 0xFF, &res);
		if(res != 0){
            ERR_printf(0);
			return res;
		}
		while(blk_bum--){
		
			this_obj->hd_io->w_r_byte(SD_START_DATA_MULTIPLE_BLOCK_WRITE);		//token
			this_obj->hd_io->write(ptr, blk_sz);								//data
			this_obj->hd_io->write((u8 *)(&crc),sizeof(crc));					//crc
			this_obj->hd_io->read(&data_res , 1);								//data res
			ptr += blk_sz;		
			if(data_res & 0x5){
				res = 0;	
			}else{
                ERR_printf(0);
				res = 0xff;	
				break;
			}
			res=sd_wait_ready();
			if(res){
				break;
			}
		}
        this_obj->hd_io->w_r_byte(SD_STOP_DATA_MULTIPLE_BLOCK_WRITE);		   //token
		sd_dis_select();
		return res;
	}else{
	
	}
	return 0;
}
/*============	SD 卡初始化	=============*/
static spi_sd_err sd_init(__spi_ctr_obj  *hd_io)
{
	sd_printf("FUN:%s\n",__func__);
	ASSERT(hd_io);
	ASSERT(hd_io->read);
	ASSERT(hd_io->write);
    spi_sd_err res;
	u8 r1;
    this_obj = &spi_sd_obj;
	this_obj->hd_io = hd_io;
	this_obj->sd_inf.type = SD_TYPE_UNKNOW;	
    
	if(sd_get_state() == SD_STA_ON_LINE){	// already init
		return SD_NO_ERR;
	}

    res = sd_identification();
	
	if(res != SD_NO_ERR){
        ERR_printf(0);
		return res;
	}
	
	sd_cmd(SD_SPI_R1, SD_CMD_CRC_ON_OFF, 0x00, 0xFF, &r1);			// CMD59 关闭 CRC 校验
	sd_cmd(SD_SPI_R1, SD_CMD_CRC_ON_OFF, 0x00, 0xFF, &r1);
	sd_cmd(SD_SPI_R1, SD_CMD_CRC_ON_OFF, 0x00, 0xFF, &r1);
	
	sd_cmd(SD_SPI_R1, SD_CMD_SET_BLOCKLEN, SD_BLOCK_SIZE, 0xFF, &r1);// CMD16 设置 BLOCK 为 512 Bytes (对 SDHC/SDHX 无效)
	if(r1){
        ERR_printf(0);
		return SD_ERR_SET_BLOCK_SIZE;
	}	
	sd_dis_select();
	sd_speed_set(SD_CLK_SPEED_HIGH);
    sd_getcard_inf(&this_obj->sd_inf);
	
	spi_sd_obj.sd_inf.block_sz = SD_BLOCK_SIZE;
	
	sd_set_state(SD_STA_ON_LINE);
	return SD_NO_ERR;
}

/////////////////////////////////////////////////////////////////////////////////
/*****	>>> soft spi <<<	****/
typedef enum __SPI_TYPE{
	CPOL0_CPHA0,		// CLK  LOW,  1 age
	CPOL0_CPHA1,		// CLK  LOW,  2 age
	CPOL1_CPHA0,
	CPOL1_CPHA1,
}SPI_TYPE_E;

typedef struct _soft_spi_io {
	SPI_TYPE_E type;
	u8  (*MISO)(void);
	void (*MOSI)(u8 en);
	void (*CLK)(u8 en);
}Soft_SPI_hd;

volatile u32 delay_cnt_g = 100;

static void soft_spi_delay(u32 delay)
{
	u32 tmp = delay;
	while(delay--);
	delay_cnt_g = tmp;
}
static u8 soft_spi_tx_rx_byte(Soft_SPI_hd *hd, u8 s_dat)
{
	u8 i = 0;
	u8 r_dat = 0;
	if(hd->type == CPOL0_CPHA0){
		hd->CLK(0);
		soft_spi_delay(delay_cnt_g);
		for(;i<8;i++){		
			if(s_dat & (0x80 >> i)){
				hd->MOSI(1);
			}else{
				hd->MOSI(0);
			}
			soft_spi_delay(delay_cnt_g);
			hd->CLK(1);
			soft_spi_delay(delay_cnt_g);
			
			r_dat <<= 1;
			if(hd->MISO()) r_dat++;
			
			hd->CLK(0);	
			soft_spi_delay(delay_cnt_g);
		}
	}
	if(hd->type == CPOL0_CPHA1){
		hd->CLK(0);
		soft_spi_delay(delay_cnt_g);
		for(;i<8;i++){	
			hd->CLK(1);
			soft_spi_delay(delay_cnt_g);
			if(s_dat & (0x80 >> i)){
				hd->MOSI(1);
			}else{
				hd->MOSI(0);
			}
			soft_spi_delay(delay_cnt_g);	
			hd->CLK(0);				
			r_dat <<= 1;
			if(hd->MISO()) r_dat++;
			soft_spi_delay(delay_cnt_g);
		}
	}
	if(hd->type == CPOL1_CPHA0){
		hd->CLK(1);
		soft_spi_delay(delay_cnt_g);
		for(;i<8;i++){		
			if(s_dat & (0x80 >> i)){
				hd->MOSI(1);
			}else{
				hd->MOSI(0);
			}
			soft_spi_delay(delay_cnt_g);	
			hd->CLK(0);
			soft_spi_delay(delay_cnt_g);	
			r_dat <<= 1;
			if(hd->MISO()) r_dat++;
			hd->CLK(1);	
			soft_spi_delay(delay_cnt_g);
		}
	}
	if(hd->type == CPOL1_CPHA1){
		hd->CLK(1);
		soft_spi_delay(delay_cnt_g);
		for(;i<8;i++){	
			hd->CLK(0);	
			soft_spi_delay(delay_cnt_g);
			if(s_dat & (0x80 >> i)){
				hd->MOSI(1);
			}else{
				hd->MOSI(0);
			}
			soft_spi_delay(delay_cnt_g);
			hd->CLK(1);				
			r_dat <<= 1;
			if(hd->MISO()) r_dat++;	
			soft_spi_delay(delay_cnt_g);
		}	
	}
	return r_dat;
}

static void soft_spi_send_buf(Soft_SPI_hd *hd, const u8 *buf, u32 len)
{
	ASSERT(buf);
	const u8 *ptr = buf;
	if(len == 0) return;
	while(len--){
		soft_spi_tx_rx_byte(hd,*ptr++);
	}
}
static void soft_spi_read_buf(Soft_SPI_hd *hd, u8 *buf, u32 len)
{
	ASSERT(buf);
	u8 *ptr = buf;
	if(len == 0) return;
	while(len--){
		*ptr++ = soft_spi_tx_rx_byte(hd, 0xFF);
	}
}


Soft_SPI_hd soft_spi_hd;
static void soft_spi_cs(u8 en)
{
	en?GPIO_SetPinsOutput(SD_SPI_CS_GPIO, 1<<SD_SPI_CS_PIN):\
	   GPIO_ClearPinsOutput(SD_SPI_CS_GPIO, 1<<SD_SPI_CS_PIN);
}
static u8 soft_spi_miso(void)
{
	return GPIO_ReadPinInput(SD_SPI_MISO_GPIO, SD_SPI_MISO_PIN);
}
static void soft_spi_mosi(u8 en)
{
	en?GPIO_SetPinsOutput(SD_SPI_MOSI_GPIO, 1<<SD_SPI_MOSI_PIN):\
	   GPIO_ClearPinsOutput(SD_SPI_MOSI_GPIO, 1<<SD_SPI_MOSI_PIN);
}
static void soft_spi_clk(u8 en)
{
	en?GPIO_SetPinsOutput(SD_SPI_CLK_GPIO, 1<<SD_SPI_CLK_PIN):\
	   GPIO_ClearPinsOutput(SD_SPI_CLK_GPIO, 1<<SD_SPI_CLK_PIN);
}
static bool soft_spi_ioctr(u8 cmd, void *buf)
{
	u8 res = false;
	return true;
}



static void spi_init(void)
{
    spi_obj.cs_str = soft_spi_cs;
    soft_spi_hd.type = CPOL0_CPHA0;
	soft_spi_hd.CLK = soft_spi_clk;
	soft_spi_hd.MISO= soft_spi_miso;
	soft_spi_hd.MOSI= soft_spi_mosi;
	
	port_pin_config_t config = {
      kPORT_PullUp ,
      kPORT_FastSlewRate,
      kPORT_PassiveFilterDisable,
      kPORT_LowDriveStrength,
      kPORT_MuxAsGpio,
	};
	PORT_SetMultiplePinsConfig(PORTC, SD_SPI_CS_PIN_M|
	                                  SD_SPI_CLK_PIN_M|
	                                  SD_SPI_MOSI_PIN_M|
	                                  SD_SPI_MISO_PIN_M, &config);	
	
	
	GPIOC->PDDR |= (SD_SPI_CS_PIN_M|SD_SPI_CLK_PIN_M|SD_SPI_MOSI_PIN_M);
	GPIOC->PDDR &= ~(SD_SPI_MISO_PIN_M);

}

static void spi_send_buf(const u8 *pData, uint32_t Size)
{
	soft_spi_send_buf(&soft_spi_hd, pData, Size);
}

static void spi_read_buf(uint8_t *pData, uint32_t Size)
{
	soft_spi_read_buf(&soft_spi_hd, pData, Size);
}

static u8 spi_send_read_byte(u8 dat)
{
	return soft_spi_tx_rx_byte(&soft_spi_hd, dat);
}
static u8 spi_ioctr(u32 cmd, void *buf)
{
	u8 res = false;

	return true;
}







__spi_ctr_obj spi_obj = {
	.cs_str = NULL,
	.init 	= spi_init,
	.read   = spi_read_buf,
	.write  = spi_send_buf,
	.w_r_byte = spi_send_read_byte,
	.io_ctr = spi_ioctr,
};

__spi_sd_obj spi_sd_obj = {

	.init 	= sd_init,
	.read 	= sd_read_blk,
	.write 	= sd_write_blk,
	.io_ctr = sd_io_control,
	.status = sd_get_state,
};
/*****=== soft spi end ===****/

#if 0 
static volatile bool isTransferCompleted = false;
static volatile bool isMasterOnTransmit = false;
static volatile bool isMasterOnReceive = false;


static void DSPI_MasterSignalEvent_t(uint32_t event)
{
    if (true == isMasterOnReceive)
    {
//        PRINTF("This is DSPI_MasterSignalEvent_t\r\n");
//        PRINTF("Master receive data from slave has completed!\r\n");
        isMasterOnReceive = false;
    }
    if (true == isMasterOnTransmit)
    {
//        PRINTF("This is DSPI_MasterSignalEvent_t\r\n");
//        PRINTF("Master transmit data to slave has completed!\r\n");
        isMasterOnTransmit = false;
    }
    isTransferCompleted = true;
}
static void spi_cs_init(void)
{
    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput, 0,
    };
	GPIO_PinInit(SD_SPI_CS_GPIO, SD_SPI_CS_PIN, &led_config);
}
static void spi_cs_ctr(u8 en)
{
	if(en){
		GPIO_SetPinsOutput(SD_SPI_CS_GPIO, 1<<SD_SPI_CS_PIN);
	}else{
		GPIO_ClearPinsOutput(SD_SPI_CS_GPIO, 1<<SD_SPI_CS_PIN);
	}
}

static void spi_dma_init(void)
{
    edma_config_t edmaConfig = {0};

    EDMA_GetDefaultConfig(&edmaConfig);
    EDMA_Init(SD_SPI_DMA_BASEADDR, &edmaConfig);
    DMAMUX_Init(SD_SPI_DMA_MUX_BASEADDR);

}



static void spi_init(void)
{
    spi_obj.cs_str = spi_cs_ctr;
	spi_cs_init();
	spi_dma_init();
	DRIVER_SD_SPI.Initialize(DSPI_MasterSignalEvent_t);
    DRIVER_SD_SPI.PowerControl(ARM_POWER_FULL);
    DRIVER_SD_SPI.Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL0_CPHA1, SD_CLK_SPEED_LOW);
}

static void spi_send_buf(const u8 *pData, uint32_t Size)
{
    isTransferCompleted = false;
    isMasterOnTransmit = true;
    DRIVER_SD_SPI.Send(pData, Size);
    while (!isTransferCompleted){;}
}

static void spi_read_buf(uint8_t *pData, uint32_t Size)
{
    isTransferCompleted = false;
    isMasterOnReceive = true;
    DRIVER_SD_SPI.Receive(pData, Size);
	while (!isTransferCompleted){;}
}

static u8 spi_send_read_byte(u8 dat)
{
	u8 tmp = dat;
    isTransferCompleted = false;
    isMasterOnTransmit = true;
    DRIVER_SD_SPI.Send(&tmp, 1);
    while (!isTransferCompleted){;}
	
	isTransferCompleted = false;
    isMasterOnReceive = true;
    DRIVER_SD_SPI.Receive(&tmp, 1);
	while (!isTransferCompleted){;}
	
	return tmp;
}
static u8 spi_ioctr(u32 cmd, void *buf)
{
	u8 res = false;
	u32 dat32  = 0;
	switch(cmd){
		case SPI_SET_SPEED_CMD:
			 dat32 = *((u32 *)(buf));
             DRIVER_SD_SPI.Control(SPI_SET_SPEED_CMD,dat32);
			 res = true;
			 return res;
		default:
			break;
	}
	return res;
}
__spi_ctr_obj spi_obj = {
	.cs_str = NULL,
	.init 	= spi_init,
	.read   = spi_read_buf,
	.write  = spi_send_buf,
	.w_r_byte = spi_send_read_byte,
	.io_ctr = spi_ioctr,
};

__spi_sd_obj spi_sd_obj = {

	.init 	= sd_init,
	.read 	= sd_read_blk,
	.write 	= sd_write_blk,
	.io_ctr = sd_io_control,
	.status = sd_get_state,
};
 

#endif

