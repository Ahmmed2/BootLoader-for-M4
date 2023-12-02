/*
 * Bootloader.h
 *
 *  Created on: Nov 8, 2023
 *      Author: Ahmed
 */

#ifndef INC_BOOTLOADER_H_
#define INC_BOOTLOADER_H_


/************************ Include ************************/
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include "usart.h"
#include "crc.h"



/************************ Defines ************************/

#define BL_DEBUG_UART									&huart2
#define BL_HOST_COMMUNICATION_UART						&huart2
/* Debug message sent or not */
#define BL_UART_DEBUG_MESSAGE							BL_ENABLE_DEBUG_MESSAGE
#define BL_ENABLE_DEBUG_MESSAGE 						 1
#define BL_DISABLE_DEBUG_MESSAGE						 0
#define BL_HOST_BUFFER_RX_LENGTH						200

/* Version Related */
#define BL_VENDOR_ID									100
#define BL_MAJOR_VER									 1
#define BL_MINOR_VER									 0
#define BL_PATCH_VER									 0

/* CRC Type */
#define CRC_TYPE_SIZE_BYTE								 4
#define CRC_OK 											 1
#define CRC_NOT_OK 										 0

/* Commands List */

#define CBL_GET_VER_CMD					0X10
#define CBL_GET_HELP_CMD				0X11
#define CBL_GET_CID_CMD					0X12
#define CBL_GET_RDP_STATUS_CMD			0X13
#define CBL_GO_TO_ADDR_CMD				0X14
#define CBL_FLASH_ERASE_CMD				0X15
#define CBL_MEM_WRITE_CMD				0X16
#define CBL_EN_R_W_PROTECT_CMD			0X17
#define CBL_MEM_READ_CMD				0X18
#define CBL_READ_SECTOR_STATUS_CMD		0X19
#define CBL_OTP_READ_CMD				0X20
#define CBL_CHANGE_ROP_Level_CMD		0X21

/* ACK or NACK */
#define BL_SEND_ACK						0XCD
#define BL_SEND_NACK					0XAB

/* Sector 2 "Application is there " */
#define FLASH_SECTOR2_BASE_ADDRESS 		0x08008000U

/* Address Verification */
#define ADDRESS_VALID 					1
#define ADDRESS_INVALID					0

#define STM32F407_SRAM1_SIZE			(112*1024)
#define STM32F407_SRAM2_SIZE			(16*1024)
#define STM32F407_SRAM3_SIZE			(64*1024)
#define	STM32F407_FLASH_SIZE			(1024*1024)

#define STM32F407_SRAM1_END				(SRAM1_BASE+STM32F407_SRAM1_SIZE)
#define STM32F407_SRAM2_END				(SRAM2_BASE+STM32F407_SRAM2_SIZE)
#define STM32F407_SRAM3_END				(CCMDATARAM_BASE+STM32F407_SRAM3_SIZE)
#define STM32F407_FLASH_END				(FLASH_BASE+STM32F407_FLASH_SIZE)

#define ERASE_INVALID					2
#define ERASE_VALID						3
#define MASS_ERASE						0xFF
#define SUCCESSFUL_ERASE_REPORT			0xFFFFFFFFU

#define FLASH_WRITE_DONE				1
#define FLASH_WRITE_FAIL				0

#define ROP_LEVEL_CHANGE_VALID			1
#define ROP_LEVEL_CHANGE_INVALID		0


/***************** DataType Deceleration *****************/

typedef enum
{
	BL_NACK = 0 ,
	BL_ACK
}BL_Status;

/* pointer to function Data Type */
typedef void (*pMainApp)(void) ;
typedef void (*Jump_ptr)(void) ; // Used in Jump to certain Address

/******************** SW Implementation *******************/


void Print_Message (char *Format , ...) ;
BL_Status UART_Fetch_Host_Commands (void) ;

#endif /* INC_BOOTLOADER_H_ */
