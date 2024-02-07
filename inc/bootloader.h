/*
 * bootloader.h
 *
 *  Created on: Jan 22, 2024
 *      Author: salah
 */

#ifndef INC_BOOTLOADER_H_
#define INC_BOOTLOADER_H_

#include "usart.h"
#include "crc.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define CBL_GET_VER_CMD					0x10
#define CBL_GET_HELP_CMD				0x11
#define CBL_GET_CID_CMD					0x12

#define CBL_GO_TO_ADDR_CMD				0x14
#define CBL_FLASH_ERASE_CMD				0x15
#define CBL_MEM_WRITE_CMD				0x16


#define HOST_MAX_SIZE					200


#define Send_NACK				0xAB
#define Send_ACK				0xCD

#define CRC_VERIFYING_FAILED			0x00
#define CRC_VERIFYING_PASS				0x01


#define CBL_VENDOR_ID				100
#define CBL_SW_MAJOR_VERSION		1
#define CBL_SW_MINOR_VERSION		1
#define CBL_SW_PATCH_VERSION		0


#define CBL_FLASH_MAX_PAGE_NUMBER	16
#define CBL_FLASH_MASS_ERASE		0xff


#define INVALID_PAGE_NUMBER			0x00
#define VALID_PAGE_NUMBER			0x01
#define UNSUCCESSFUL_ERASE			0x02
#define SUCCESSFUL_ERASE			0x03


#define HAL_SUCCESSFUL_ERASE		0xffffffffU


#define ADDRESS_IS_INVALID			0x00
#define ADDRESS_IS_VALID			0x01


#define STM32F103_SRAM_SIZE			(20 * 1024)
#define STM32F103_FLASH_SIZE		(64 * 1024)
#define STM32F103_SRAM_END			(SRAM_BASE + STM32F103_SRAM_SIZE)
#define STM32F103_FLASH_END			(FLASH_BASE + STM32F103_FLASH_SIZE)


#define FLASH_PAYLOAD_WRITE_FAILED	0x00
#define FLASH_PAYLOAD_WRITE_PASSED	0x01


#define 	FLASH_SECTOR2_BASE_ADDRESS   			0x08008000U


typedef enum {
	BL_NACK = 0,
	BL_ACK
}BL_Status;

void BL_SendMessage(char* format,...);
BL_Status BL_FetchCmd();



#endif /* INC_BOOTLOADER_H_ */
