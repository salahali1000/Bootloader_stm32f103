/*
 * bootloader.c
 *
 *  Created on: Jan 22, 2024
 *      Author: salah
 */


#include "bootloader.h"


static void BL_Send_ACK(uint8_t Data_len);
static void BL_Send_NACK();
static uint32_t BL_CRC_Verify(uint8_t* Pdata, uint32_t DataLen, uint32_t HostCRC) ;
static void CBL_GET_Version(uint8_t *Host_Buffer) ;
static void CBL_GET_Help(uint8_t *Host_Buffer) ;
static void CBL_GET_Chip_Identification_Number(uint8_t *Host_Buffer) ;
static void CBL_Flash_Erase(uint8_t *Host_Buffer) ;
static uint8_t Perform_Flash_Erase(uint32_t Page_Address , uint8_t Page_Number) ;
static void CBL_Write_Data(uint8_t *Host_Buffer);
static uint8_t BL_Address_Verification(uint32_t Address);
static uint8_t FlashMemory_Write(uint16_t *Pdata , uint32_t Start_Address , uint8_t Payload_Len) ;
static void bootloader_jump_to_user_app(uint8_t *Host_buffer) ;

static uint8_t Host_Buffer[HOST_MAX_SIZE] ;

void BL_SendMessage(char* format,...)
{
	char message[100]={0};
	va_list args;
	va_start(args,format);
	vsprintf(message,format,args);
	HAL_UART_Transmit(&huart2, (uint8_t*)message, sizeof(message), HAL_MAX_DELAY) ;
	va_end(args);

}

static void CBL_GET_Version(uint8_t *Host_Buffer)
{
	uint8_t version[4]= {CBL_VENDOR_ID,CBL_SW_MAJOR_VERSION,CBL_SW_MINOR_VERSION,CBL_SW_PATCH_VERSION} ;
	uint16_t Host_Packet_Len = 0 ;
	uint32_t CRC_Value = 0;
	Host_Packet_Len = Host_Buffer[0] + 1 ;
	CRC_Value = *(uint32_t*)(Host_Buffer + Host_Packet_Len-4) ;
	if(CRC_VERIFYING_PASS == BL_CRC_Verify((uint8_t*) &Host_Buffer[0], Host_Packet_Len-4, CRC_Value))
	{
		BL_Send_ACK(4) ;
		HAL_UART_Transmit(&huart2,(uint8_t*)version,4,HAL_MAX_DELAY);
	}
	else
	{
		BL_Send_NACK() ;
	}
}

static void CBL_GET_Help(uint8_t *Host_Buffer)
{
	uint8_t BL_Supported_CMS[6]=
	{
			CBL_GET_VER_CMD,
			CBL_GET_HELP_CMD,
			CBL_GET_CID_CMD,
			CBL_GO_TO_ADDR_CMD,
			CBL_FLASH_ERASE_CMD,
			CBL_MEM_WRITE_CMD
	};
	uint16_t Host_Packet_Len = 0 ;
	uint32_t CRC_Value = 0;
	Host_Packet_Len = Host_Buffer[0] + 1 ;
	CRC_Value = *(uint32_t*)(Host_Buffer + Host_Packet_Len-4) ;
	if(CRC_VERIFYING_PASS == BL_CRC_Verify((uint8_t*) &Host_Buffer[0], Host_Packet_Len-4, CRC_Value))
	{
		BL_Send_ACK(6) ;
		HAL_UART_Transmit(&huart2,(uint8_t*)BL_Supported_CMS,6,HAL_MAX_DELAY);
	}
	else
	{
		BL_Send_NACK() ;
	}
}

static void CBL_GET_Chip_Identification_Number(uint8_t *Host_Buffer)
{
	uint16_t Chip_ID = 0 ;
	uint16_t Host_Packet_Len = 0 ;
	uint32_t CRC_Value = 0;
	Host_Packet_Len = Host_Buffer[0] + 1 ;
	CRC_Value = *(uint32_t*)(Host_Buffer + Host_Packet_Len-4) ;
	if(CRC_VERIFYING_PASS == BL_CRC_Verify((uint8_t*) &Host_Buffer[0], Host_Packet_Len-4, CRC_Value))
	{
		Chip_ID = (uint16_t)(DBGMCU->IDCODE & 0x00000FFF) ;
		BL_Send_ACK(2) ;
		HAL_UART_Transmit(&huart2,(uint8_t*)&Chip_ID,2,HAL_MAX_DELAY);
	}
	else
	{
		BL_Send_NACK() ;
	}
}

static void CBL_Flash_Erase(uint8_t *Host_Buffer)
{
	uint8_t Erase_Status = UNSUCCESSFUL_ERASE ;
	uint16_t Host_Packet_Len = 0 ;
	uint32_t CRC_Value = 0;
	Host_Packet_Len = Host_Buffer[0] + 1 ;
	CRC_Value = *(uint32_t*)(Host_Buffer + Host_Packet_Len-4) ;
	if(CRC_VERIFYING_PASS == BL_CRC_Verify((uint8_t*) &Host_Buffer[0], Host_Packet_Len-4, CRC_Value))
	{
		Erase_Status = Perform_Flash_Erase(*((uint32_t*)&Host_Buffer[2]),Host_Buffer[6]);
		BL_Send_ACK(1) ;
		HAL_UART_Transmit(&huart2,(uint8_t*)&Erase_Status,1,HAL_MAX_DELAY);
	}
	else
	{
		BL_Send_NACK() ;
	}
}


static void CBL_Write_Data(uint8_t *Host_Buffer)
{
	uint8_t Address_Verify = ADDRESS_IS_INVALID ;
	uint32_t Address_Host = 0 ;
	uint8_t Data_Length = 0 ;
	uint8_t Payload_Status = FLASH_PAYLOAD_WRITE_FAILED ;
	uint16_t Host_Packet_Len = 0 ;
	uint32_t CRC_Value = 0;
	Host_Packet_Len = Host_Buffer[0] + 1 ;
	CRC_Value = *(uint32_t*)(Host_Buffer + Host_Packet_Len-4) ;
	if(CRC_VERIFYING_PASS == BL_CRC_Verify((uint8_t*) &Host_Buffer[0], Host_Packet_Len-4, CRC_Value))
	{
		BL_Send_ACK(1) ;
		Address_Host = *((uint32_t*)&Host_Buffer[2]) ;
		Data_Length = Host_Buffer[6] ;
		Address_Verify = BL_Address_Verification(Address_Host);
		if(Address_Verify == ADDRESS_IS_VALID)
		{
			Payload_Status = FlashMemory_Write((uint16_t*)&Host_Buffer[7], Address_Host, Data_Length) ;
			HAL_UART_Transmit(&huart2,(uint8_t*)&Payload_Status,1,HAL_MAX_DELAY);

		}
		else
		{
			HAL_UART_Transmit(&huart2,(uint8_t*)&Address_Verify,1,HAL_MAX_DELAY);
		}
	}
	else
	{
		BL_Send_NACK() ;
	}
}

static uint8_t BL_Address_Verification(uint32_t Address)
{
	uint8_t Address_Verify = ADDRESS_IS_INVALID ;
	if(Address >= FLASH_BASE && Address<= STM32F103_FLASH_END)
	{
		Address_Verify = ADDRESS_IS_VALID ;
	}
	else if(Address >= SRAM_BASE && Address<= STM32F103_SRAM_END)
	{
		Address_Verify = ADDRESS_IS_VALID ;
	}
	else
	{
		Address_Verify = ADDRESS_IS_VALID ;
	}
	return Address_Verify ;
}


static void bootloader_jump_to_user_app(uint8_t *Host_buffer)
{
	/* Extract Packet length Sent by the HOST */
	uint16_t Host_Packet_Len = Host_buffer[0]+1;
	uint32_t CRC_Value = 0;

	/* Extract CRC32 Sent by the HOST */
	CRC_Value = *(uint32_t*)(Host_Buffer + Host_Packet_Len-4) ;
	if(CRC_VERIFYING_PASS == BL_CRC_Verify((uint8_t*) &Host_Buffer[0], Host_Packet_Len-4, CRC_Value))
	{
		BL_Send_ACK(1);
		HAL_UART_Transmit(&huart2,(uint8_t*)FLASH_SECTOR2_BASE_ADDRESS,4,HAL_MAX_DELAY);

		//just a function pointer to hold the address of the reset handler of the user app.
		void (*Reset_Handler)(void);

		//disbale interuppts
		__set_PRIMASK(1);
		__disable_irq();

		SCB->VTOR = FLASH_SECTOR2_BASE_ADDRESS;

		// 1. configure the MSP by reading the value from the base address of the sector 2
		uint32_t msp_value = *(__IO uint32_t *)FLASH_SECTOR2_BASE_ADDRESS;

		__set_MSP(msp_value);

		uint32_t resethandler_address = *(__IO uint32_t *) (FLASH_SECTOR2_BASE_ADDRESS + 4);

		Reset_Handler = (void*) resethandler_address;

		//3. jump to reset handler of the user application
		Reset_Handler();
	}
	else
	{
		BL_Send_NACK();
	}
}



BL_Status BL_FetchCmd()
{

	uint8_t Data_Length = 0;
	BL_Status status = BL_ACK ;
	HAL_StatusTypeDef HAL_Status= HAL_ERROR ;
	memset(Host_Buffer,0,HOST_MAX_SIZE);
	HAL_Status =  HAL_UART_Receive(&huart2,Host_Buffer,1,HAL_MAX_DELAY);
	if(HAL_Status != HAL_OK)
	{
		status = BL_NACK ;
	}
	else {
		Data_Length = Host_Buffer[0] ;
		HAL_Status =  HAL_UART_Receive(&huart2,&Host_Buffer[1],Data_Length,HAL_MAX_DELAY);
		if(HAL_Status != HAL_OK)
		{
			status = BL_NACK ;
		}
		else {
			switch(Host_Buffer[1])
			{
			case CBL_GET_VER_CMD :CBL_GET_Version(Host_Buffer);break;
			case CBL_GET_HELP_CMD :CBL_GET_Help(Host_Buffer);break;
			case CBL_GET_CID_CMD :CBL_GET_Chip_Identification_Number(Host_Buffer);break;
			case CBL_GO_TO_ADDR_CMD :bootloader_jump_to_user_app(Host_Buffer);break;
			case CBL_FLASH_ERASE_CMD :CBL_Flash_Erase(Host_Buffer);break;
			case CBL_MEM_WRITE_CMD :CBL_Write_Data(Host_Buffer);break;
			default : status = BL_NACK ;
			}

		}
	}
	return status;
}





static void BL_Send_ACK(uint8_t Data_len)
{
	uint8_t ACK_Value[2]= {0};
	ACK_Value[0]=Send_ACK;
	ACK_Value[1]=Data_len ;
	HAL_UART_Transmit(&huart2,(uint8_t*)ACK_Value,2,HAL_MAX_DELAY);
}



static void BL_Send_NACK()
{
	uint8_t ACK_Value=Send_NACK;
	HAL_UART_Transmit(&huart2,&ACK_Value,sizeof(ACK_Value),HAL_MAX_DELAY);

}
static uint32_t BL_CRC_Verify(uint8_t* Pdata, uint32_t DataLen, uint32_t HostCRC)
{
	uint8_t CRC_Status = CRC_VERIFYING_FAILED ;
	uint32_t MCU_CRC = 0 ;
	uint32_t DataBuffer=0 ;
	for(uint8_t count = 0; count<DataLen ; count++)
	{
		DataBuffer = (uint32_t)Pdata[count] ;
		MCU_CRC = HAL_CRC_Accumulate(&hcrc,&DataBuffer,1) ;
	}
	__HAL_CRC_DR_RESET(&hcrc) ;
	if(HostCRC == MCU_CRC)
	{
		CRC_Status = CRC_VERIFYING_PASS ;
	}
	else
	{
		CRC_Status = CRC_VERIFYING_FAILED ;
	}
	return CRC_Status;
}
static uint8_t Perform_Flash_Erase(uint32_t Page_Address , uint8_t Page_Number)
{
	HAL_StatusTypeDef HAL_Status = HAL_ERROR ;
	FLASH_EraseInitTypeDef pEraseInit ;
	uint32_t PageError = 0;
	uint8_t Page_Status = INVALID_PAGE_NUMBER ;
	if(Page_Number > CBL_FLASH_MAX_PAGE_NUMBER)
	{
		Page_Status = INVALID_PAGE_NUMBER ;
	}
	else
	{
		Page_Status = VALID_PAGE_NUMBER ;
		if(Page_Number <= (CBL_FLASH_MAX_PAGE_NUMBER-1) || Page_Address == CBL_FLASH_MASS_ERASE )
		{
			if(Page_Address == CBL_FLASH_MASS_ERASE)
			{
				pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES ;
				pEraseInit.Banks = FLASH_BANK_1 ;
				pEraseInit.PageAddress = 0x8008000 ;
				pEraseInit.NbPages = 12 ;
			}
			else
			{
				pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES ;
				pEraseInit.Banks = FLASH_BANK_1 ;
				pEraseInit.PageAddress = Page_Address ;
				pEraseInit.NbPages = Page_Number ;
			}
			HAL_FLASH_Unlock();
			HAL_Status = HAL_FLASHEx_Erase(&pEraseInit, &PageError);
			HAL_FLASH_Lock();
			if(PageError == HAL_SUCCESSFUL_ERASE)
			{
				Page_Status = SUCCESSFUL_ERASE ;
			}
			else
			{
				Page_Status = UNSUCCESSFUL_ERASE ;
			}

		}
		else
		{
			Page_Status = INVALID_PAGE_NUMBER ;
		}
	}
	return Page_Status ;
}

static uint8_t FlashMemory_Write(uint16_t *Pdata , uint32_t Start_Address , uint8_t Payload_Len)
{
	HAL_StatusTypeDef HAL_Status = HAL_ERROR ;
	uint32_t address = 0 ;
	uint8_t Update_Address = 0 ;
	uint8_t Payload_Status = FLASH_PAYLOAD_WRITE_FAILED ;
	HAL_FLASH_Unlock();

	for(uint8_t payload_count = 0,Update_Address = 0 ; payload_count< Payload_Len/2 ; payload_count++,Update_Address+=2)
	{
		address = Start_Address + Update_Address ;
		HAL_Status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address, Pdata[payload_count]);
		if(HAL_Status != HAL_OK)
		{
			Payload_Status = FLASH_PAYLOAD_WRITE_FAILED ;
		}
		else
		{
			Payload_Status = FLASH_PAYLOAD_WRITE_PASSED ;
		}
	}
	//HAL_FLASH_Lock();
	return Payload_Status ;
}

