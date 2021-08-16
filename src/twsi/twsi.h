#ifndef TWSI_H_
#define TWSI_H_

#include <avr/io.h>

#define TWI_SPACE_FLASH 0u
#define TWI_SPACE_RAM 1u

void TWI_Start(void);
void TWI_Stop(void);
void TWI_WriteAddrW(uint8_t u8data);
void TWI_WriteAddrR(uint8_t u8data);
void TWI_WriteData(uint8_t u8data);
void TWI_CheckStatus();
uint8_t TWI_ReadACK(void);
uint8_t TWI_ReadNACK(void);
uint8_t TWI_IsBusy(void);

void TWI_Run(void);

void TWI_SendBlockT(const uint8_t address, const uint8_t *buffer, const uint16_t bufferLength, const uint8_t bufferSpace);

#endif