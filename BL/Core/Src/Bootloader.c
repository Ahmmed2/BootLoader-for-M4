/*
 * Bootloader.c
 *
 *  Created on: Nov 8, 2023
 *      Author: Ahmed
 */

/************** Includes **************/

#include "Bootloader.h"

/**** Static Function Deceleration ****/
static void 	BL_Get_Version(uint8_t *Host_Buffer) 								  										;
static void     BL_Get_Help(uint8_t *Host_Buffer)    								  										;
static void     BL_Get_Chip_Identification_Number(uint8_t *Host_Buffer)               										;
static void     BL_Read_Protection_Level(uint8_t *Host_Buffer)                		  										;
static void     BL_Jump_To_Address(uint8_t *Host_Buffer)                			  										;
static void     BL_Erase_Flash(uint8_t *Host_Buffer)               					  										;
static void     BL_Memory_Write(uint8_t *Host_Buffer)                				  										;
static void 	BL_Change_Read_Protection(uint8_t *Host_Buffer)																	;

static uint8_t 	CRC_Verify(uint8_t *pData , uint32_t Data_Len , uint32_t HOST_CRC) 											;
static void 	Send_ACK_Reply(uint8_t Reply_Len) 								  											;
static void 	Send_NACK()														  											;

static void 	Jump_To_User_App (void)											 											;
static uint8_t 	HOST_Jump_Address_Verification(uint32_t Host_Address)			  											;
static uint8_t  Perform_Flash_Erase(uint8_t Sector_Number , uint8_t Number_of_Sectors) 							  			;
static uint8_t  Flash_Memory_Write_Payload(uint8_t *Host_Payload , uint32_t Payload_Start_Address , uint32_t Payloadlen) 	;
static uint8_t  Get_RDP_Level (void)																						;
static uint8_t  Change_RDP_Level (uint32_t RDP_Level) 																		;
/**** Global Variables Definitions ****/

static uint8_t BL_Host_Buffer[BL_HOST_BUFFER_RX_LENGTH] ;
static uint8_t BL_Supported_Commands [12] =
{
		CBL_GET_VER_CMD,
		CBL_GET_HELP_CMD ,
		CBL_GET_CID_CMD ,
		CBL_GET_RDP_STATUS_CMD ,
		CBL_GO_TO_ADDR_CMD ,
		CBL_FLASH_ERASE_CMD ,
		CBL_MEM_WRITE_CMD ,
		CBL_EN_R_W_PROTECT_CMD ,
		CBL_MEM_READ_CMD,
		CBL_READ_SECTOR_STATUS_CMD ,
		CBL_OTP_READ_CMD ,
		CBL_CHANGE_ROP_Level_CMD
};

/**** SW Functions Implementations ****/

/* For Debugging */
void Print_Message (char *Format , ...)
{

	char  Message[100] = {0} ;

	va_list args ;

	va_start(args,Format) ;

	vsprintf(Message,Format,args) ;

#if BL_UART_DEBUG_MESSAGE == BL_ENABLE_DEBUG_MESSAGE

	HAL_UART_Transmit(BL_DEBUG_UART , (uint8_t *)Message , sizeof(Message) , HAL_MAX_DELAY) ;

#endif

	va_end(args) ;
}

/* Calculate CRC among data Received and check if it correct or not */
static uint8_t 	CRC_Verify(uint8_t *pData , uint32_t Data_Len , uint32_t HOST_CRC)
{
	uint8_t CRC_State = CRC_NOT_OK ;
	uint32_t CRC_Value ;
	uint8_t DataCounter ;
	uint32_t Data_Buffer ;

	/* Calculate CRC on Data */
	for (DataCounter = 0 ; DataCounter < Data_Len ; DataCounter ++)
	{
		Data_Buffer = (uint32_t)pData[DataCounter] ;
		CRC_Value = HAL_CRC_Accumulate(&hcrc, &Data_Buffer,1) ;
	}

	__HAL_CRC_DR_RESET(&hcrc) ;

	if (CRC_Value == HOST_CRC )
	{
		CRC_State = CRC_OK ;
	}

	return CRC_State ;

}

/* Send ACK in case of positive ACK */
static void Send_ACK_Reply(uint8_t Reply_Len)
{
	uint8_t ACK_Value[2] ;
	ACK_Value[0] = BL_SEND_ACK ;
	ACK_Value[1] = Reply_Len ;
	HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, ACK_Value, 2, HAL_MAX_DELAY) ;
}
/* Send NACK in case of NACK */
static void Send_NACK()
{
	uint8_t ACK_Value ;
	ACK_Value = BL_SEND_NACK ;
	HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &ACK_Value, 1, HAL_MAX_DELAY) ;
}

static void BL_Get_Version(uint8_t *Host_Buffer)
{
	uint8_t BL_Version[4] = {BL_VENDOR_ID,BL_MAJOR_VER,BL_MINOR_VER,BL_PATCH_VER};
	uint16_t HOST_Whole_Packet_Length = 0 ;
	uint32_t HOST_CRC32 = 0 ;
	uint8_t CRC_State ;

	/* Whole packet length (Including the first Byte ) */
	HOST_Whole_Packet_Length = Host_Buffer[0] + 1 ;

	/* Store CRC value (4 Byte) */
	HOST_CRC32 = *(uint32_t *)(Host_Buffer + HOST_Whole_Packet_Length - CRC_TYPE_SIZE_BYTE) ;

	/* CRC Verification */
	CRC_State = CRC_Verify (Host_Buffer,2,HOST_CRC32) ;
	if (CRC_State == CRC_OK)
	{
		Send_ACK_Reply(4) ;
		HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, BL_Version, 4, HAL_MAX_DELAY) ;
	}
	else
	{
		Send_NACK() ;
	}

}

static void  BL_Get_Help(uint8_t *Host_Buffer)
{
	uint16_t HOST_Whole_Packet_Length = 0 ;
	uint32_t HOST_CRC32 = 0 ;
	uint8_t CRC_State ;

	/* Whole packet length (Including the first Byte ) */
	HOST_Whole_Packet_Length = Host_Buffer[0] + 1 ;

	/* Store CRC value (4 Byte) */
	HOST_CRC32 = *(uint32_t *)(Host_Buffer + HOST_Whole_Packet_Length - CRC_TYPE_SIZE_BYTE) ;

	/* CRC Verification */
	CRC_State = CRC_Verify (Host_Buffer,HOST_Whole_Packet_Length-4,HOST_CRC32) ;
	if (CRC_State == CRC_OK)
	{
		Send_ACK_Reply(12) ;
		HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, BL_Supported_Commands, 12, HAL_MAX_DELAY) ;
	}
	else
	{
		Send_NACK() ;
	}

}

static void  BL_Get_Chip_Identification_Number(uint8_t *Host_Buffer)
{
	uint16_t HOST_Whole_Packet_Length = 0 ;
	uint16_t MCU_ID_Number ;
	uint32_t HOST_CRC32 = 0 ;
	uint8_t CRC_State ;

	/* Whole packet length (Including the first Byte ) */
	HOST_Whole_Packet_Length = Host_Buffer[0] + 1 ;

	/* Store CRC value (4 Byte) */
	HOST_CRC32 = *(uint32_t *)(Host_Buffer + HOST_Whole_Packet_Length - CRC_TYPE_SIZE_BYTE) ;

	/* CRC Verification */
	CRC_State = CRC_Verify (Host_Buffer,HOST_Whole_Packet_Length-4,HOST_CRC32) ;
	if (CRC_State == CRC_OK)
	{
		/* Get Chip Identification Number (only LS 11 bit i want )  */
		MCU_ID_Number = (uint16_t)(DBGMCU->IDCODE & (0x00000FFF)) ;

		Send_ACK_Reply(2) ;
		HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, (uint8_t*)&MCU_ID_Number,2,HAL_MAX_DELAY) ;
	}
	else
	{
		Send_NACK() ;
	}
}

/* Make sure Address that Host want to jump to is valid or not */
static uint8_t	HOST_Jump_Address_Verification(uint32_t Host_Address)
{
	uint8_t Return_Status = ADDRESS_INVALID ;

	if ((Host_Address>=SRAM1_BASE && Host_Address<=STM32F407_SRAM1_END)||
		(Host_Address>=SRAM2_BASE && Host_Address<=STM32F407_SRAM2_END)||
		(Host_Address>=CCMDATARAM_BASE && Host_Address<=STM32F407_SRAM3_END)||
		(Host_Address>=FLASH_BASE && Host_Address<=STM32F407_FLASH_END)
		)
	{
		Return_Status = ADDRESS_VALID ;
	}

	return Return_Status ;

}

static uint8_t Get_RDP_Level (void)
{
	FLASH_OBProgramInitTypeDef Flash_Info ;
	HAL_FLASHEx_OBGetConfig(&Flash_Info) ;
/*
 * Options :
 * 	OB_RDP_LEVEL_0   ((uint8_t)0xAA)
 *	OB_RDP_LEVEL_1   ((uint8_t)0x55)
 *	OB_RDP_LEVEL_2   ((uint8_t)0xCC)
 */
	return (uint8_t)(Flash_Info.RDPLevel) ;

}
static void BL_Read_Protection_Level(uint8_t *Host_Buffer)
{
	uint16_t HOST_Whole_Packet_Length = 0 ;
	uint32_t HOST_CRC32 = 0 ;
	uint8_t CRC_State ;
	uint8_t Acquired_Level = 0 ;

	/* Whole packet length (Including the first Byte ) */
	HOST_Whole_Packet_Length = Host_Buffer[0] + 1 ;

	/* Store CRC value (4 Byte) */
	HOST_CRC32 = *(uint32_t *)(Host_Buffer + HOST_Whole_Packet_Length - CRC_TYPE_SIZE_BYTE) ;

	/* CRC Verification */
	CRC_State = CRC_Verify (Host_Buffer,HOST_Whole_Packet_Length-4,HOST_CRC32) ;

	if (CRC_State == CRC_OK)
	{
		Send_ACK_Reply(1) ;

		/* Read Protection Level */
		Acquired_Level = Get_RDP_Level() ;
		/* Report Protection Level */
		HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &Acquired_Level,1,HAL_MAX_DELAY) ;
	}
	else
	{
		Send_NACK() ;
	}

}

/* To Jump to a certain Address whatever the place */
static void BL_Jump_To_Address(uint8_t *Host_Buffer)
{
	uint16_t HOST_Whole_Packet_Length = 0 ;
	uint32_t HOST_CRC32 = 0 ;
	uint8_t CRC_State ;
	uint32_t HOST_Jump_Add = 0 ;
	uint8_t Address_Verification = ADDRESS_INVALID ;

	/* Whole packet length (Including the first Byte ) */
	HOST_Whole_Packet_Length = Host_Buffer[0] + 1 ;

	/* Store CRC value (4 Byte) */
	HOST_CRC32 = *(uint32_t *)(Host_Buffer + HOST_Whole_Packet_Length - CRC_TYPE_SIZE_BYTE) ;

	/* CRC Verification */
	CRC_State = CRC_Verify (Host_Buffer,HOST_Whole_Packet_Length-4,HOST_CRC32) ;



	if (CRC_State == CRC_OK)
	{
		Send_ACK_Reply(1) ;

		/* Extract Desired Address */
		HOST_Jump_Add = *((uint32_t*)&Host_Buffer[2]) ;
		/* Address Verification */
		Address_Verification = HOST_Jump_Address_Verification(HOST_Jump_Add) ;
			if (Address_Verification == ADDRESS_VALID)
			{
				// Add 1 as indication of Thumb not ARM instruction
				Jump_ptr Jump_Address = (Jump_ptr) (HOST_Jump_Add+1) ;
				Print_Message("Jump to : 0x%X \r\n",Jump_Address) ;
				HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &Address_Verification,1,HAL_MAX_DELAY) ;
				Jump_Address() ;
 			}
			else
			{
				HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &Address_Verification,1,HAL_MAX_DELAY) ;
			}
	}
	else
	{
		Send_NACK() ;
	}

}

static uint8_t  Perform_Flash_Erase(uint8_t Sector_Number , uint8_t Number_of_Sectors)
{
	uint8_t Erase_Status = ERASE_INVALID ;
	FLASH_EraseInitTypeDef pEraseInit ;
	HAL_StatusTypeDef Flash_Status = HAL_ERROR;
	uint32_t Sector_Error = 0 ;

	if (Sector_Number == MASS_ERASE)
	{
		pEraseInit.Banks = FLASH_BANK_1 ;  					/* BANK 1 */
		pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3 ;	 /*Device operating range: 2.7V to 3.6V */
		pEraseInit.TypeErase = FLASH_TYPEERASE_MASSERASE ;
		Flash_Status = HAL_FLASH_Unlock() ;
		Flash_Status = HAL_FLASHEx_Erase(&pEraseInit, &Sector_Error) ;
		if (SUCCESSFUL_ERASE_REPORT ==Sector_Error && Flash_Status==HAL_OK )
		{
			Erase_Status = ERASE_VALID ;
		}

	}
	else if ((Sector_Number >=0) && (Sector_Number<=11) && ((Sector_Number+Number_of_Sectors)<=11))
	{
		pEraseInit.Banks = FLASH_BANK_1 ; 					  /* BANK 1 */
		pEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3 ;	  /*Device operating range: 2.7V to 3.6V */
		pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS ;
		pEraseInit.Sector = Sector_Number ;
		pEraseInit.NbSectors = Number_of_Sectors  ;
		Flash_Status = HAL_FLASH_Unlock() ;
		Flash_Status = HAL_FLASHEx_Erase(&pEraseInit, &Sector_Error) ;
		if (SUCCESSFUL_ERASE_REPORT == Sector_Error && Flash_Status==HAL_OK )
		{
			Erase_Status = ERASE_VALID ;
		}
	}

	Flash_Status = HAL_FLASH_Lock() ;

	return Erase_Status ;
}
static void BL_Erase_Flash(uint8_t *Host_Buffer)
{

	uint16_t HOST_Whole_Packet_Length = 0 ;
	uint32_t HOST_CRC32 = 0 ;
	uint8_t CRC_State ;
	uint8_t Erase_Verification = ERASE_INVALID ;

	/* Whole packet length (Including the first Byte ) */
	HOST_Whole_Packet_Length = Host_Buffer[0] + 1 ;

	/* Store CRC value (4 Byte) */
	HOST_CRC32 = *(uint32_t *)(Host_Buffer + HOST_Whole_Packet_Length - CRC_TYPE_SIZE_BYTE) ;

	/* CRC Verification */
	CRC_State = CRC_Verify (Host_Buffer,HOST_Whole_Packet_Length-4,HOST_CRC32) ;

	Send_ACK_Reply(1) ;

	if (CRC_State == CRC_OK)
	{
		Send_ACK_Reply(1) ;
		/* Erase Verification */
		Erase_Verification = Perform_Flash_Erase(Host_Buffer[2],Host_Buffer[3]);
			if (Erase_Verification == ERASE_VALID)
			{
				/* Report Erase Succeeded */
				HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &Erase_Verification,1,HAL_MAX_DELAY) ;

 			}
			else
			{
				/* Report Erase Failed */
				HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &Erase_Verification,1,HAL_MAX_DELAY) ;
			}
	}
	else
	{
		Send_NACK() ;
	}

}
static uint8_t Flash_Memory_Write_Payload(uint8_t *Host_Payload , uint32_t Payload_Start_Address , uint32_t Payloadlen)
{
	uint8_t Return_Status = FLASH_WRITE_FAIL ;
	HAL_StatusTypeDef Flash_Status = HAL_ERROR;
	uint16_t Payload_Counter = 0 ;

	Flash_Status = HAL_FLASH_Unlock() ;
	if (Flash_Status != HAL_OK)
	{
		Return_Status = FLASH_WRITE_FAIL ;
	}
	else
	{
	for (Payload_Counter = 0 ; Payload_Counter < Payloadlen ; Payload_Counter++ )
	{
		Flash_Status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,(Payload_Start_Address+Payload_Counter) ,(uint64_t)(Host_Payload[Payload_Counter])) ;
		if (Flash_Status != HAL_OK)
		{
			Return_Status = FLASH_WRITE_FAIL ;
			break ;
		}
			Return_Status = FLASH_WRITE_DONE ;
	}
	}
	Flash_Status = HAL_FLASH_Lock() ;

	return Return_Status ;

}
/* I mean by Memory here is flash */
static void BL_Memory_Write(uint8_t *Host_Buffer)
{
	uint16_t HOST_Whole_Packet_Length = 0 ;
	uint32_t HOST_CRC32 = 0 ;
	uint8_t CRC_State ;
	uint32_t HOST_Address = 0 ;
	uint8_t PayLoad_Length = 0 ;
	uint8_t Write_Verification = FLASH_WRITE_FAIL ;
	uint8_t Address_Verification = ADDRESS_INVALID ;

	/* Whole packet length (Including the first Byte ) */
	HOST_Whole_Packet_Length = Host_Buffer[0] + 1 ;

	/* Store CRC value (4 Byte) */
	HOST_CRC32 = *(uint32_t *)(Host_Buffer + HOST_Whole_Packet_Length - CRC_TYPE_SIZE_BYTE) ;

	/* CRC Verification */
	CRC_State = CRC_Verify (Host_Buffer,HOST_Whole_Packet_Length-4,HOST_CRC32) ;


	if (CRC_State == CRC_OK)
	{
		Send_ACK_Reply(1) ;
		HOST_Address   = *((uint32_t*)(&Host_Buffer[2])) ;
		PayLoad_Length = Host_Buffer[6] ;
		/* Check if Address is valid or not */
		Address_Verification = HOST_Jump_Address_Verification(HOST_Address) ;

		if (Address_Verification == ADDRESS_VALID )
		{

			Write_Verification = Flash_Memory_Write_Payload(&Host_Buffer[7],HOST_Address, PayLoad_Length) ;
			if (Write_Verification == FLASH_WRITE_DONE)
			{
				/* Report Writing Succeeded */
				HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &Write_Verification,1,HAL_MAX_DELAY) ;
			}
			else
				/* Report Writing Failed */
				HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &Write_Verification,1,HAL_MAX_DELAY) ;
		}

		else
		{
			/* Report Invalid Address so i can't write */
			HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &Write_Verification,1,HAL_MAX_DELAY) ;
		}
	}
	else
	{
		Send_NACK() ;
	}

}




/* Change Read protection Level */
static uint8_t Change_RDP_Level (uint32_t RDP_Level )
{
	uint8_t Change_State = ROP_LEVEL_CHANGE_INVALID ;
	HAL_StatusTypeDef HAL_Status = HAL_ERROR ;

	/* Unlock Flash option control Register Access */
	HAL_Status = HAL_FLASH_OB_Unlock() ;

	if (HAL_Status == HAL_OK)
	{
		/* Program Option Bytes */
		FLASH_OBProgramInitTypeDef pOBInit ;
		pOBInit.OptionType = OPTIONBYTE_RDP ; /*!< RDP option byte configuration  */
		pOBInit.Banks = FLASH_BANK_1 ;
		pOBInit.RDPLevel = RDP_Level ;
		HAL_Status = HAL_FLASHEx_OBProgram(&pOBInit) ;
		if (HAL_Status == HAL_OK)
		{
			/* Launch Option Byte Loading */
			HAL_Status = HAL_FLASH_OB_Launch() ;

			if (HAL_Status == HAL_OK)
			{
				/* Lock Flash option control Register Access*/
				HAL_Status = HAL_FLASH_OB_Lock() ;
				if (HAL_Status == HAL_OK)
				{
					Change_State = ROP_LEVEL_CHANGE_VALID ;

				}
			}
		}

	}
	HAL_Status = HAL_FLASH_OB_Lock() ;

	return Change_State ;
}

static void BL_Change_Read_Protection(uint8_t *Host_Buffer)
{
	uint16_t HOST_Whole_Packet_Length = 0 ;
	uint32_t HOST_CRC32 = 0 ;
	uint8_t CRC_State ;
	uint8_t Change_Status = ROP_LEVEL_CHANGE_INVALID ;
	uint8_t HOST_ROP_Level = 0 ;

	/* Whole packet length (Including the first Byte ) */
	HOST_Whole_Packet_Length = Host_Buffer[0] + 1 ;

	/* Store CRC value (4 Byte) */
	HOST_CRC32 = *(uint32_t *)(Host_Buffer + HOST_Whole_Packet_Length - CRC_TYPE_SIZE_BYTE) ;

	/* CRC Verification */
	CRC_State = CRC_Verify (Host_Buffer,HOST_Whole_Packet_Length-4,HOST_CRC32) ;


	if (CRC_State == CRC_OK)
	{
		Send_ACK_Reply(1) ;


		HOST_ROP_Level = Host_Buffer[2] ;
		/* To make sure not to enter Level 2 */
		if (HOST_ROP_Level != OB_RDP_LEVEL_2)
		{
		/* Request Change Read out protection Level */
		Change_Status  = Change_RDP_Level(Host_Buffer[2]) ;
		}

		if (Change_Status == ROP_LEVEL_CHANGE_VALID )
		{
			/* Report Writing Succeeded */
			HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &Change_Status,1,HAL_MAX_DELAY) ;
		}
		else
		{
			/* Report Invalid Address so i can't write */
			HAL_UART_Transmit(BL_HOST_COMMUNICATION_UART, &Change_Status,1,HAL_MAX_DELAY) ;
		}
	}
	else
	{
		Send_NACK() ;
	}
}


static void Jump_To_User_App (void)
{
	/* Main Stack Pointer Value */
	uint32_t MSP_Value  = *((volatile uint32_t *) FLASH_SECTOR2_BASE_ADDRESS) ;
	/* Reset Handler Definition Function */
	uint32_t MainAppAdr = *((volatile uint32_t *) FLASH_SECTOR2_BASE_ADDRESS+4) ;

	/* Pointer to Function points to Reset Handler */
	pMainApp Reset_Handler_Address  = (pMainApp) MainAppAdr ;

	/* Set the Value of MSP */
	__set_MSP(MSP_Value) ;

	/* Re-Initialize clock and peripherals Again */
	HAL_RCC_DeInit() ;

	/* Call Reset Handler */
	Reset_Handler_Address () ;

}

BL_Status BL_UART_Fetch_Host_Commands (void)
{
	BL_Status Status = BL_NACK ;
	/* UART RX return */
	HAL_StatusTypeDef HAL_Status = HAL_ERROR ;
	uint8_t Data_Length = 0 ;

	/* Array Elements = 0 */
	memset(BL_Host_Buffer,0,BL_HOST_BUFFER_RX_LENGTH) ;

	/* Receiving the  Command Length */
	HAL_Status = HAL_UART_Receive(BL_HOST_COMMUNICATION_UART, BL_Host_Buffer, 1, HAL_MAX_DELAY) ;

	if (HAL_Status != HAL_OK)
	{
		/* Report Error */
		Status = BL_NACK ;
	}
	else
	{

	Data_Length = BL_Host_Buffer[0] ;
	/* Receive the rest of Record */
	HAL_Status = HAL_UART_Receive(BL_HOST_COMMUNICATION_UART, &(BL_Host_Buffer[1]), Data_Length, HAL_MAX_DELAY) ;

			if (HAL_Status != HAL_OK)
			{
				/* Report Error */
				Status = BL_NACK ;
			}
			else
			{

				switch (BL_Host_Buffer[1])
				{
				case CBL_GET_VER_CMD  		 	 :
					Status = BL_ACK ;
					Print_Message("CBL_GET_VER_CMD \r\n") ;
					BL_Get_Version(BL_Host_Buffer) ;
					break ;
				case CBL_GET_HELP_CMD 		 	 :
					Status = BL_ACK ;
					Print_Message("CBL_GET_HELP_CMD \r\n") ;
					BL_Get_Help(BL_Host_Buffer) ;
					break ;
				case CBL_GET_CID_CMD  		 	 :
					Status = BL_ACK ;
					Print_Message("CBL_GET_CID_CMD \r\n") ;
					BL_Get_Chip_Identification_Number(BL_Host_Buffer);
					break ;
				case CBL_GET_RDP_STATUS_CMD  	 :
					Status = BL_ACK ;
					Print_Message("CBL_GET_RDP_STATUS_CMD \r\n") ;
					BL_Read_Protection_Level(BL_Host_Buffer) ;
					break ;
				case CBL_GO_TO_ADDR_CMD  		 :
					Status = BL_ACK ;
					Print_Message("CBL_GO_TO_ADDR_CMD \r\n") ;
					BL_Jump_To_Address(BL_Host_Buffer) ;
					break ;
				case CBL_FLASH_ERASE_CMD  		 :
					Status = BL_ACK ;
					Print_Message("CBL_FLASH_ERASE_CMD \r\n") ;
					BL_Erase_Flash(BL_Host_Buffer) ;
					break ;
				case CBL_MEM_WRITE_CMD  		 :
					Status = BL_ACK ;
					Print_Message("CBL_MEM_WRITE_CMD \r\n") ;
					BL_Memory_Write(BL_Host_Buffer) ;
					break ;
				case CBL_EN_R_W_PROTECT_CMD  	 :
					Status = BL_ACK ;
					Print_Message("CBL_EN_R_W_PROTECT_CMD \r\n") ;
					break ;
				case CBL_MEM_READ_CMD  			 :
					Status = BL_ACK ;
					Print_Message("CBL_MEM_READ_CMD \r\n") ;
					break ;
				case CBL_READ_SECTOR_STATUS_CMD  :
					Status = BL_ACK ;
					Print_Message("CBL_READ_SECTOR_STATUS_CMD \r\n") ;
					break ;
				case CBL_OTP_READ_CMD  			 :
					Status = BL_ACK ;
					Print_Message("CBL_OTP_READ_CMD \r\n") ;
					break ;
				case CBL_CHANGE_ROP_Level_CMD  	 :
					Status = BL_ACK ;
					Print_Message("Change Read Protection Level \r\n") ;
					BL_Change_Read_Protection(BL_Host_Buffer) ;
					break ;
				default :
					Print_Message("Invalid Command \r\n") ;
					Status = BL_NACK ;
					break ;


				}
			}

	}




	return Status ;
}
