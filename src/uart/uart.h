#ifndef uart_h
#define uart_h

#include <avr/io.h>
#include "../stringmanager/stringmanager.h"

#define UART_TX_BUFFER_SIZE 64u
#define UART_RX_BUFFER_SIZE 32u

#define UART_RX_BUSY 0
#define UART_RX_FREE 1

#define UART_START_SEQ_LENGTH 3u
#define UART_STOP_SEQ_LENGTH 4u
#define UART_LENGTH_OF_BODY_LENGTH STR_8BIT_STRING_LENGTH

#define UART_MAX_BODY_LENGTH UART_TX_BUFFER_SIZE - (UART_START_SEQ_LENGTH + UART_STOP_SEQ_LENGTH + UART_LENGTH_OF_BODY_LENGTH)

extern void UART_Init(void);
extern void UART_TX_WriteStr(const uint8_t chArr[], const uint8_t length);
extern void UART_RX_ReadArray(uint8_t *dst, const uint8_t length);

extern void UART_TX_WritePackage(const uint8_t body[], const uint8_t bodyLength);
extern void UART_RX_ReadPackage(uint8_t body[], uint8_t *bodyLength, uint8_t *error);

uint16_t UART_Get_RX_newDataLength(void);
#endif