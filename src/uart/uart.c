#include "uart.h"

#include <avr/interrupt.h>
#include "../defines.h"
#include "../utils/utils.h"
#include "../dio/dio.h"


#define BOUD 250000
#define UBRR (uint16_t)( ( ( (F_CPU / 16) / BOUD) ) - 1)


volatile static uint8_t UART_TX_buffer[UART_TX_BUFFER_SIZE];
volatile static uint8_t UART_TX_readPos = 0u;
volatile static uint8_t UART_TX_writePos = 0u;

volatile static uint8_t UART_RX_buffer[UART_RX_BUFFER_SIZE];
volatile static uint8_t UART_RX_readPos = 0u;
volatile static uint8_t UART_RX_writePos = 0u;
volatile static uint8_t UART_RX_busyState = UART_RX_FREE;

volatile static uint16_t UART_RX_newDataLength = 0u;

const static uint8_t UART_startSeq[UART_START_SEQ_LENGTH] = {'A', 'S', 'K'};
const static uint8_t UART_stopSeq[UART_STOP_SEQ_LENGTH] = {'E', 'N', 'D', '\n'};

static void UART_TX_Append(const uint8_t ch);
static uint8_t UART_RX_ReadChar(void);


uint16_t UART_Get_RX_newDataLength(void)
{
	uint16_t tmpNewDataLength = 0u;
	
	if(UART_RX_busyState == UART_RX_FREE) 
	{
		tmpNewDataLength = UART_RX_newDataLength;
		UART_RX_newDataLength = 0u;
	}

	return tmpNewDataLength;
}

void UART_Init(void)
{
    /* Set initial values */
    UCSRA = 0u;
    UCSRB = 0u;
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
    UBRRH = 0u;
    UBRRL = 0u;

    /* Set baud rate */
    UBRRH = (uint8_t)(UBRR >> 8);
    UBRRL = (uint8_t)UBRR;
    /* Enable receiver and transmitter, and corresponding interrputs */
    UCSRB = (1<<RXEN) | (1<<TXEN) | (1<<RXCIE) | (1<<TXCIE);
    /* Set frame format: 8data, 2stop bit */
    UCSRC = (1<<URSEL) | (1<<USBS) | (3<<UCSZ0);

}

void UART_TX_Append(const uint8_t ch) 
{
    /* Write character to the currern position of TX buffer */
	UART_TX_buffer[UART_TX_writePos] = ch;

    /* And move index to the next position */
	UART_TX_writePos++;
	if(UART_TX_writePos >= UART_TX_BUFFER_SIZE) {
		UART_TX_writePos = 0u;
	}
}

void UART_TX_WriteStr(const uint8_t chArr[], const uint8_t length) 
{
    uint8_t idx = 0u;
    /* Append the whole input array to TX buffer */
	for(idx = 0u; idx < length; idx++) 
    {
		UART_TX_Append(chArr[idx]);
	}
    /* Make shore if UDR is ready to receive new data */
	if(UCSRA & (1<<UDRE)) 
    {
        /* UDR assignment for TXC interrupt triggering */
		UDR = 0u;
	}
}

uint8_t UART_RX_ReadChar(void) 
{
	uint8_t retVal = 0u;
	/* Check if there is charcter in rx buffer to read */
	if(UART_RX_readPos != UART_RX_writePos) 
    {
		/* Read */
		retVal = UART_RX_buffer[UART_RX_readPos];
		/* And shift pointer to the next char */
		UART_RX_readPos++;
		if(UART_RX_readPos == UART_RX_BUFFER_SIZE) 
        {
			UART_RX_readPos = 0u;
		}
	}
	return retVal;
}

void UART_RX_ReadArray(uint8_t *dst, const uint8_t length)
{
	uint8_t idx = 0u;
	uint8_t tmpChar = 0u;
	/* While there are rx character to read and
			 dst array is not full and
			 rx buffer was not compleatle read */
	for(idx = 0u; (UART_RX_readPos != UART_RX_writePos) && (idx < length) && (idx < UART_RX_BUFFER_SIZE); idx++)
	{
		/* Copy chars to dst */
		tmpChar = UART_RX_ReadChar();
		dst[idx] = tmpChar;
	}
}

void UART_TX_WritePackage(const uint8_t body[], const uint8_t bodyLength)
{
	uint8_t newBodyLength = bodyLength;
	uint8_t tmpArr[STR_8BIT_STRING_LENGTH] = {'0'};

	/* Cut body length if it is too big */
	if(newBodyLength > UART_MAX_BODY_LENGTH) 
	{
		newBodyLength = UART_MAX_BODY_LENGTH;
	}
	/* Write start sequence */
	UART_TX_WriteStr(UART_startSeq, UART_START_SEQ_LENGTH);
	/* body length */
	STR_8BitHexToString(tmpArr, newBodyLength);
	UART_TX_WriteStr(tmpArr, STR_8BIT_STRING_LENGTH);
	/* body */
	UART_TX_WriteStr(body, newBodyLength);
	/* and stop sequence */
	UART_TX_WriteStr(UART_stopSeq, UART_STOP_SEQ_LENGTH);
}

void UART_RX_ReadPackage(uint8_t body[], uint8_t *bodyLength, uint8_t *error)
{
	uint8_t RX_bufferIdx = 0u;
	uint8_t aligned_RX_buffer[UART_RX_BUFFER_SIZE] = {0u}; 
	uint8_t seqIdx = 0u;

	uint8_t startSeqFound = D_FALSE;
	uint8_t stopSeqFound = D_FALSE;

	uint8_t startSeqPos = 0u;
	uint8_t bodyPos = 0u;
	/* To easer work with rx buffer we convert ring-buffer to regular array-buffer */
	for(RX_bufferIdx = 0; RX_bufferIdx < UART_RX_BUFFER_SIZE; RX_bufferIdx++)
	{
		aligned_RX_buffer[RX_bufferIdx] = UART_RX_ReadChar();
	}
	//STR_WriteStringToLCD(LCD_LINE_2, 0, 16, (char*)aligned_RX_buffer);
	/* The rirst thing we need to do is to find start sequence */
	/* For each buffer position */
	for(RX_bufferIdx = 0u; RX_bufferIdx < (UART_RX_BUFFER_SIZE - UART_START_SEQ_LENGTH); RX_bufferIdx++)
	{
		/* we compare part of buffer matching with start sequence */
		for(seqIdx = RX_bufferIdx; seqIdx < UART_START_SEQ_LENGTH; seqIdx++)
		{
			/* and if wi found it */
			if( (U_ArrCmp(&aligned_RX_buffer[RX_bufferIdx], UART_startSeq, UART_START_SEQ_LENGTH) == 0) && (startSeqFound == D_FALSE) )
			{
				/* save sequence start position */
				startSeqPos = RX_bufferIdx;
				/* And rise the flag that shows we found this seq */
				startSeqFound = D_TRUE;
			}
		}
	}
	/* If we found start sequence */
	if(startSeqFound == D_TRUE)
	{
		/* then we can define body length thant goes rigth after start sequence */
		(*bodyLength) = STR_StringTo8BitHex( &aligned_RX_buffer[startSeqPos + UART_START_SEQ_LENGTH], error);
		/* If conversion was success */
		if( (*error) == ERR_NO_ERROR)
		{
			/* and if body has acceptable length */
			if( (*bodyLength) < UART_MAX_BODY_LENGTH) 
			{
				/* then we calculate body start position in rx buffer */
				bodyPos = startSeqPos + UART_START_SEQ_LENGTH + UART_LENGTH_OF_BODY_LENGTH;
				/* We have eficient info to find stop sequence, so if we found it */
				if( U_ArrCmp( &aligned_RX_buffer[bodyPos + (*bodyLength)], UART_stopSeq, UART_START_SEQ_LENGTH ) == 0 )
				{
					/* then rise the corresponding flag */
					stopSeqFound = D_TRUE;
				}
			}
		} 
	}
	//ASK101234567890ABCDEFEND
	/* If stop sequence is found */ 
	if( stopSeqFound == D_TRUE )
	{
		/* then we can finaly read the body */
		U_ArrCpy(body, &aligned_RX_buffer[bodyPos], (*bodyLength));
	} else 
	{
		/* else we assign bodyLength to zero to show what package was not found */
		(*bodyLength) = 0u;
	}
}
ISR(USART_TXC_vect) 
{
	if(UART_TX_writePos != UART_TX_readPos) 
    {
		UDR = UART_TX_buffer[UART_TX_readPos];

		UART_TX_readPos++;
		if(UART_TX_readPos >= UART_TX_BUFFER_SIZE) 
        {
			UART_TX_readPos = 0u;
		}
	}
}
ISR(USART_RXC_vect) 
{
	UART_RX_busyState = UART_RX_BUSY;
	UART_RX_buffer[UART_RX_writePos] = UDR;

	if(UART_RX_buffer[UART_RX_writePos] == '\n')
	{
		UART_RX_busyState = UART_RX_FREE;
	}

	UART_RX_writePos++;
	if(UART_RX_writePos >= UART_RX_BUFFER_SIZE) 
    {
		UART_RX_writePos = 0u;
	}
	
	UART_RX_newDataLength++;
}
