#include "twsi.h"

#include <util/twi.h>
#include "defines.h"
#include "../signalgateway/signalgateway.h"
#include "../dio/dio.h"
#include <avr/delay.h>
#include <avr/pgmspace.h>


//2500 ns - 400 kHz
#define TWI_SCL_FREQ 400 /* todo may be 630? */
#define TWI_TWBR (((F_CPU/TWI_SCL_FREQ)/1000 - 16)/2)
#define TWI_LOOP_TIMEOUT 254
#define TWI_DEBUG

#define TWI_ADDRESS_WRITE 0x00
#define TWI_ADDRESS_READ 0x01
#define TWI_ADDRESS_MASK 0x01


/*
	TWI - two wire interface
	TWDR - Adress/data shift
	TWBR - Bit rate
	TWAR - Adress
	TWSR - Status
	TWCR - Control
*/

/* State machine */
#define TWI_POINT_NONE 0u
#define TWI_POINT_START 1u
#define TWI_POINT_WRITE_ADDR 2u
#define TWI_POINT_WRITE_DATA 3u
#define TWI_POINT_STOP 4u

static uint8_t TWI_currentPoint = TWI_POINT_STOP;
static uint8_t TWI_validStatus = 0u;

static const uint8_t *TWI_outputBuffer = 0u;
static uint16_t TWI_outputBufferLength = 0u;
static uint8_t TWI_outputAddress = 0u;
static uint8_t *TWI_inputBuffer = 0u;
static uint16_t TWI_inputBufferLength = 0u;
static uint16_t TWI_outputBufferIdx = 0u;
static uint16_t TWI_inputBufferIdx = 0u;
static uint16_t TWI_bufferSpace = TWI_SPACE_FLASH;

void TWI_WriteAddr(uint8_t u8data);

void TWI_SendBlockT(const uint8_t address, const uint8_t *buffer, const uint16_t bufferLength, const uint8_t bufferSpace)
{
	TWI_bufferSpace = bufferSpace;
	TWI_outputBuffer = buffer;
	TWI_outputBufferLength = bufferLength;
	TWI_outputAddress = address | TWI_ADDRESS_WRITE;
	TWI_currentPoint = TWI_POINT_START;
}	

void TWI_SendBlockR(const uint8_t address, const uint8_t *outputBuffer, const uint16_t outputBufferLength, uint8_t *inputBuffer, const uint16_t inputBufferLength, const uint8_t bufferSpace)
{
	TWI_bufferSpace = bufferSpace;
	TWI_inputBuffer = inputBuffer;
	TWI_inputBufferLength = inputBufferLength;
	TWI_outputBuffer = outputBuffer;
	TWI_outputBufferLength = outputBufferLength;
	TWI_outputAddress = address | TWI_ADDRESS_READ;
	TWI_currentPoint = TWI_POINT_START;
}	

uint8_t TWI_IsBusy(void)
{
	uint8_t retVal = D_FALSE;
	if(TWI_currentPoint == TWI_POINT_NONE)
	{
		retVal = D_FALSE;
	} else 
	{
		retVal = D_TRUE;
	}
	return retVal;
}

void TWI_Run(void)
{
	switch (TWI_currentPoint)
	{
	case TWI_POINT_NONE:
		/* Nothing to do */
		break;
	case TWI_POINT_START:
		TWI_Start();

		TWI_outputBufferIdx = 0u;
		TWI_inputBufferIdx = 0u;

		TWI_validStatus = TW_START;
		TWI_currentPoint = TWI_POINT_WRITE_ADDR;
		break;
	case TWI_POINT_WRITE_ADDR:
		TWI_CheckStatus();

		TWI_WriteAddr(TWI_outputAddress);
		if( (TWI_outputAddress & TWI_ADDRESS_MASK) == TWI_ADDRESS_WRITE)
		{
			TWI_validStatus = TW_MT_SLA_ACK;
		} else
		{
			TWI_validStatus = TW_MR_SLA_ACK;
		}
		TWI_currentPoint = TWI_POINT_WRITE_DATA;
		break;
	case TWI_POINT_WRITE_DATA:
		TWI_CheckStatus();

		if(TWI_outputBufferIdx < TWI_outputBufferLength)
		{
			if(TWI_bufferSpace == TWI_SPACE_FLASH)
			{
				TWI_WriteData( pgm_read_byte( &(TWI_outputBuffer[TWI_outputBufferIdx]) ) );
			} else
			{
				TWI_WriteData(TWI_outputBuffer[TWI_outputBufferIdx]);
			}
			TWI_outputBufferIdx++;
			TWI_currentPoint = TWI_POINT_WRITE_DATA;
		} else
		{
			TWI_currentPoint = TWI_POINT_STOP;
			/*todo*/
		}
		TWI_validStatus = TW_MT_DATA_ACK;
		break;
	case TWI_POINT_STOP:
		TWI_CheckStatus();
		
		TWI_Stop();
		TWI_currentPoint = TWI_POINT_NONE;
		break;

	default:
		break;
	}
}

/* Check TWE status after hardware side handling */
void TWI_CheckStatus() 
{
	uint8_t idx = 0u;
	uint8_t counter = 0u;
	uint8_t currentStatus = TWSR & 0xF8;

	/* ~22-24 us empty */
	for(idx = 0u; (!(TWCR & (1<<TWINT) )) && (idx < TWI_LOOP_TIMEOUT); idx++)
	{
		counter++;
		/* ~1.625 us empty */
	}
	
	/* todo 
		50F8 
		TW_START -> TW_NO_INFO
		5108 
		TW_MT_SLA_ACK -> TW_START
		5318
		TW_MT_DATA_ACK -> TW_MT_SLA_ACK
		Delay ~40 us after command sending solves problem 
	*/
#ifdef TWI_DEBUG
	if(counter >= (TWI_LOOP_TIMEOUT - 1) )
	{
		switch (TWI_validStatus)
		{
		case TW_START:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_TW_START_TIMEOUT, currentStatus);
			break;
		case TW_MT_SLA_ACK:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_TW_MT_SLA_ACK_TIMEOUT, currentStatus);
			break;
		case TW_MR_SLA_ACK:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_TW_MR_SLA_ACK_TIMEOUT, currentStatus);
			break;
		case TW_MT_DATA_ACK:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_TW_MT_DATA_ACK_TIMEOUT, currentStatus);
			break;
		case TW_MR_DATA_ACK:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_TW_MR_DATA_ACK_TIMEOUT, currentStatus);
			break;
		case TW_MR_DATA_NACK:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_TW_MR_DATA_NACK_TIMEOUT, currentStatus);
			break;
		default:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_UNDEFIND, currentStatus);
			break;
		}
	}

	if (currentStatus != TWI_validStatus)
    {
		DIO_TogglePin(TIME_MEASURENMENT2);
		switch (TWI_validStatus)
		{
		case TW_START:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_TW_START_ERROR, currentStatus);
			break;
		case TW_MT_SLA_ACK:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_TW_MT_SLA_ACK_ERROR, currentStatus);
			break;
		case TW_MR_SLA_ACK:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_TW_MR_SLA_ACK_ERROR, currentStatus);
			break;
		case TW_MT_DATA_ACK:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_TW_MT_DATA_ACK_ERROR, currentStatus);
			break;
		case TW_MR_DATA_ACK:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_TW_MR_DATA_ACK_ERROR, currentStatus);
			break;
		case TW_MR_DATA_NACK:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_TW_MR_DATA_NACK_ERROR, currentStatus);
			break;
		default:
			GW_Push_ETL_errorBuffer(ETL_TWI_OBJ, ETL_TWI_UNDEFIND, currentStatus);
			break;
		}
    }
#endif
}
#define _FF 30
/* Start TWE transmission */
void TWI_Start(void) 
{
	TWCR = 0;
	TWBR = TWI_TWBR;
	
	/* Send START condition */
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
}

/* Stop TWE transmission */
void TWI_Stop(void) 
{
	/* Transmit STOP condition */
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

/* Send address byte in Writing mode */
void TWI_WriteAddrW(uint8_t u8data) 
{
	/* 	Load SLA_W into TWDR Register. Clear TWINT bit in
		TWCR to start transmission of address */
	TWDR = u8data;
	TWCR = (1<<TWINT)|(1<<TWEN);

	TWI_CheckStatus(TW_MT_SLA_ACK);
}

/* Send address byte in Writing mode */
void TWI_WriteAddr(uint8_t u8data) 
{
	/* 	Load SLA_W into TWDR Register. Clear TWINT bit in
		TWCR to start transmission of address */
	TWDR = u8data;
	TWCR = (1<<TWINT)|(1<<TWEN);

	TWI_CheckStatus(TW_MT_SLA_ACK);
}

/* Send address byte in Reading mode */
void TWI_WriteAddrR(uint8_t u8data) 
{
	/*	Load SLA_R into TWDR Register. Clear TWINT bit in
		TWCR to start transmission of address */
	TWDR = u8data | 1;
	TWCR = (1<<TWINT)|(1<<TWEN);
		
	TWI_CheckStatus(TW_MR_SLA_ACK);
}

/* Send data byte */
void TWI_WriteData(uint8_t u8data) 
{
	TWDR = u8data;
	TWCR = (1<<TWINT)|(1<<TWEN);

	TWI_CheckStatus(TW_MT_DATA_ACK);
}

/* Read byte with ACK tail */
uint8_t TWI_ReadACK(void) 
{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	
	TWI_CheckStatus(TW_MR_DATA_ACK);
	
	return TWDR;
}
/* Read byte with NACK tail */
uint8_t TWI_ReadNACK(void) 
{
	TWCR = (1<<TWINT)|(1<<TWEN);
	
	TWI_CheckStatus(TW_MR_DATA_NACK);
	
	return TWDR;
}