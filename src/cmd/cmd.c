#include "cmd.h"

#include "../utils/utils.h"
#include "../dio/dio.h"
#include "../buzzer/buzzer.h"
#include "../stringmanager/stringmanager.h"
#include "../defines.h"
#include "../oled/oled.h"
#include "../stepmotor/stepmotor.h"

#define CMD_OLED_STOP_DRAWING_CMD 0u
#define CMD_OLED_START_DRAWING_CMD 1u
#define CMD_OLED_SEND_TWI_CMD 2u

#define CMD_MOT_STEPS_TO_BE_DONE_ACTIVE_BITS 0x0FFF

typedef struct
{
    uint8_t name[3];
    uint8_t qnt;
    uint8_t data[4];
} ts_CMD_Command;

static ts_CMD_Command CmdCommands[CMD_COMMAND_QUANTITY] = {
    { "led", 2, {0, 0, 0, 0} },
    { "lcd", 4, {0, 0, 0, 0} },
    { "bip", 1, {0, 0, 0, 0} },
    { "old", 4, {0, 0, 0, 0} },
    { "mot", 4, {0, 0, 0, 0} },
};

static uint8_t CmdCurrentCommand = CMD_EMPTY;

extern ts_SM_Motor SM_motor;

void CMD_Init(void)
{

}

void CMD_ExecMotCommand(uint8_t *error)
{
	uint8_t directionId = 0u;
	uint16_t stepsToBeDone = 0u;

	if(SM_motor.stepsToBeDone <= 0)
	{
		directionId = STR_CharToHexDigit(CmdCommands[CmdCurrentCommand].data[0], error);
		if( (*error) == ERR_NO_ERROR )
		{
			stepsToBeDone = STR_StringTo16BitHex(&CmdCommands[CmdCurrentCommand].data[0], error);
			stepsToBeDone &= CMD_MOT_STEPS_TO_BE_DONE_ACTIVE_BITS;
			if( (*error) == ERR_NO_ERROR )
			{
				if(directionId == 0u)
				{
					SM_motor.direction = SM_CLOCKWISE;
				} else 
				if(directionId == 1u)
				{
					SM_motor.direction = SM_COUNTERCLOCKWISE;
				} else
				{
					(*error) = ERR_CMD_MOT_WRONG_DIRECTION_ID;
				}
				if( (*error) == ERR_NO_ERROR )
				{
					SM_motor.stepsToBeDone = (int16_t)stepsToBeDone;
				}
			}
		}
	} else
	{
		(*error) = ERR_CMD_MOT_IS_BUSY;
	}
}

//ASK05led01END
void CMD_ExecLedCommand(uint8_t *error)
{
	uint8_t ledId = 0u;
	uint8_t ledState = 0u;
	te_DIO_Pins ledDio = 0u;

	ledId = STR_CharToHexDigit(CmdCommands[CmdCurrentCommand].data[0], error);
	ledState = STR_CharToHexDigit(CmdCommands[CmdCurrentCommand].data[1], error);
	if( (*error) == ERR_NO_ERROR)
	{
		if( (ledId >= 0u) && (ledId <= 2u) )
		{
			ledDio = ledId + LED_0;
		
			if(ledState == 0u)
			{
				DIO_PinOff(ledDio);
			} else 
			if(ledState == 1u)
			{
				DIO_PinOn(ledDio);
			} else 
			{
				(*error) = ERR_CMD_LED_WRONG_LED_STATE;
			}
		} else
		{
			(*error) = ERR_CMD_LED_WRONG_LED_ID;
		}
	}
	
	CmdCurrentCommand = CMD_EMPTY;
}

//ASK07lcd0888END
void CMD_ExecLCDCommand(uint8_t *error)
{
	uint8_t lineId = 0u;
	uint8_t position = 0u;
	uint8_t line = LCD_LINE_1;

	lineId = STR_CharToHexDigit(CmdCommands[CmdCurrentCommand].data[0], error);
	position = STR_CharToHexDigit(CmdCommands[CmdCurrentCommand].data[1], error);
	if( (*error) == ERR_NO_ERROR)
	{
		if( (position >= 0u) & (position < LCD_LINE_LENGTH) )
		{
			if(lineId == 0u)
			{
				line = LCD_LINE_1;
			} else
			if(lineId == 1u)
			{
				line = LCD_LINE_2;
			} else
			{
				(*error) = ERR_CMD_LCD_WRONG_LINE_ID;
			}
		} else
		{
			(*error) = ERR_CMD_LCD_WRONG_POSITION;
		}
	}
	
	STR_WriteStringToLCD(line, position, 2, (char*)&CmdCommands[CmdCurrentCommand].data[2]);
}

//ASK04bip3END
void CMD_ExecBipCommand(uint8_t *error)
{
	uint8_t time = 0u;
	
	time = STR_CharToHexDigit(CmdCommands[CmdCurrentCommand].data[0], error);
	if( (*error) == ERR_NO_ERROR)
	{
		if(time != 0u)
		{
			BZ_Bip(time);
		} else 
		{
			(*error) = ERR_CMD_BZ_WRONG_TIME;
		}
	}
	CmdCurrentCommand = CMD_EMPTY;
}

void CMD_ExecOLEDCommand(uint8_t *error)
{
	uint8_t commandId = 0u;
	uint8_t dataByte = 0u;
	uint8_t controlByteId = 0u;
	uint8_t controlByte = OLED_CONTROL_BYTE_COMMAND;

	commandId = STR_CharToHexDigit(CmdCommands[CmdCurrentCommand].data[0], error);
	controlByteId = STR_CharToHexDigit(CmdCommands[CmdCurrentCommand].data[1], error);
	if( (*error) == ERR_NO_ERROR)
	{
		dataByte = STR_StringTo8BitHex(&CmdCommands[CmdCurrentCommand].data[2], error);
		if( (*error) == ERR_NO_ERROR)
		{
			switch (commandId)
			{
			case CMD_OLED_STOP_DRAWING_CMD:
				/* todo comannd can not work from first attempt */
				OLED_StopDrawing(error);
				break;
			case CMD_OLED_START_DRAWING_CMD:
				OLED_StartDrawing(error);
				break;
			case CMD_OLED_SEND_TWI_CMD:
				if(controlByteId == 0u)
				{
					controlByte = OLED_CONTROL_BYTE_COMMAND;
				} else 
				if(controlByteId == 1u)
				{
					controlByte = OLED_CONTROL_BYTE_DATA;
				} else 
				{
					(*error) = ERR_CMD_OLED_WRONG_CONTROL_BYTE;
				}
				if( (*error) == ERR_NO_ERROR)
				{
					OLED_SendTwoByteSequenceSecured(controlByte, dataByte, error);
				}
				break;
			default:
				(*error) = ERR_CMD_OLED_WRONG_COMMAND_ID;
				break;
			}
		}
	}
	//STR_WriteStringToLCD(LCD_LINE_2, 5, 4, CmdCommands[CmdCurrentCommand].data);
	//ASK07old0800END
	//ASK07old08AEEND
}

void CMD_Execute(uint8_t *error)
{
	switch (CmdCurrentCommand)
	{
	case CMD_LED:
		CMD_ExecLedCommand(error);
		break;
	case CMD_LCD:
		CMD_ExecLCDCommand(error);
		break;
	case CMD_BIP:
		CMD_ExecBipCommand(error);
		break;
	case CMD_OLD:
		CMD_ExecOLEDCommand(error);
		break;
	case CMD_MOT:
		CMD_ExecMotCommand(error);
		break;
	default:
		(*error) = ERR_CMD_COMMAND_NOT_FOUND;
		break;
	}
}

void CMD_ResponcePackage(const uint8_t error)
{
	switch (error)
	{
	case ERR_NO_ERROR:
		UART_TX_WritePackage((const uint8_t*)"_OK_", 4);
		break;
	case ERR_STR_WRONG_CHARACTER:
		UART_TX_WritePackage((const uint8_t*)"_STRWRCR_", 9);
		break;
	case ERR_STR_WRONG_HEX_DIGIT:
		UART_TX_WritePackage((const uint8_t*)"_STRSWHX_", 9);
		break;
	case ERR_CMD_COMMAND_NOT_FOUND:
		UART_TX_WritePackage((const uint8_t*)"_CMDCMNF_", 9);
		break;
	case ERR_CMD_CURROPTED_PACKAGE:
		UART_TX_WritePackage((const uint8_t*)"_CMDCRPG_", 9);
		break;
	case ERR_CMD_LED_WRONG_LED_ID:
		UART_TX_WritePackage((const uint8_t*)"_LEDWLID_", 9);
		break;
	case ERR_CMD_LED_WRONG_LED_STATE:
		UART_TX_WritePackage((const uint8_t*)"_LEDWLST_", 9);
		break;
	case ERR_CMD_LCD_WRONG_LINE_ID:
		UART_TX_WritePackage((const uint8_t*)"_LCDWLID_", 9);
		break;
	case ERR_CMD_LCD_WRONG_POSITION:
		UART_TX_WritePackage((const uint8_t*)"_LCDWPOS_", 9);
		break;
	case ERR_CMD_BZ_WRONG_TIME:
		UART_TX_WritePackage((const uint8_t*)"_BUZWRTM_", 9);
		break;
	case ERR_CMD_OLED_WRONG_CONTROL_BYTE:
		UART_TX_WritePackage((const uint8_t*)"_OLDWRCB_", 9);
		break;
	case ERR_CMD_OLED_WRONG_COMMAND_ID:
		UART_TX_WritePackage((const uint8_t*)"_OLDWCID_", 9);
		break;
	case ERR_CMD_OLED_FAIL_TO_STOP:
		UART_TX_WritePackage((const uint8_t*)"_OLDFSOP_", 9);
		break;
	case ERR_CMD_OLED_FAIL_TO_START:
		UART_TX_WritePackage((const uint8_t*)"_OLDFSRT_", 9);
		break;
	case ERR_CMD_MOT_WRONG_DIRECTION_ID:
		UART_TX_WritePackage((const uint8_t*)"_MOTWDID_", 9);
		break;
	case ERR_CMD_MOT_IS_BUSY:
		UART_TX_WritePackage((const uint8_t*)"_MOTBUSY_", 9);
	case ERR_CMD_TWI_IS_BUSY:
		UART_TX_WritePackage((const uint8_t*)"_TWIBUSY_", 9);
		break;
	default:
		UART_TX_WritePackage((const uint8_t*)"_NTEX_", 6);
		break;
	}
}

void CMD_Run(void)
{
	//DIO_PinOn(TIME_MEASURENMENT);
	/* ~22-360 us */
    uint8_t recievedMessage[UART_MAX_BODY_LENGTH] = {0};
	uint8_t length = 0u;
    uint8_t commandIdx = 0u;
	uint16_t UART_RX_newDataLength = 0u;
	uint8_t error = ERR_NO_ERROR;

	UART_RX_newDataLength = UART_Get_RX_newDataLength();
	if(UART_RX_newDataLength > 0u)
	{
		CmdCurrentCommand = CMD_EMPTY;
		UART_RX_ReadPackage(recievedMessage, &length, &error);
		if(error == ERR_NO_ERROR)
		{
			/*todo if*/
			if(length > 0u)
			{		
				//STR_WriteStringToLCD(LCD_LINE_2, 5, length, recievedMessage);
				for(commandIdx = 0u; commandIdx < CMD_COMMAND_QUANTITY;  commandIdx++)
				{
					if( (length == CMD_COMMAND_LENGTH + CmdCommands[commandIdx].qnt) && (U_ArrCmp(recievedMessage, CmdCommands[commandIdx].name, CMD_COMMAND_LENGTH) == 0) )
					{
						DIO_TogglePin(LED_7);	
						U_ArrCpy(CmdCommands[commandIdx].data, &recievedMessage[CMD_COMMAND_LENGTH], CmdCommands[commandIdx].qnt);
						CmdCurrentCommand = commandIdx;
					}
				}
				if(CmdCurrentCommand == CMD_EMPTY)
				{
					CMD_ResponcePackage(ERR_CMD_COMMAND_NOT_FOUND);
				} else
				{
					CMD_Execute(&error);
					CMD_ResponcePackage(error);
				}
			} else
			{
				CMD_ResponcePackage(ERR_CMD_CURROPTED_PACKAGE);
			}
		} else 
		{
			CMD_ResponcePackage(error);
		}
	}
	//DIO_PinOff(TIME_MEASURENMENT);
    //ASK04hellEND
	//ASK09amstupid?END
    
}

