#ifndef oled_h
#define oled_h

#include <avr/io.h>

#define OLED_HEIGHT 64u
#define OLED_WIDTH 128u

#define OLED_CONTROL_BYTE_COMMAND 0x00
#define OLED_CONTROL_BYTE_DATA 0x40

void OLED_SendTwoByteSequenceSecured(uint8_t controlByte, uint8_t dataByte, uint8_t *error);
void OLED_Run(void);

void OLED_StopDrawing(uint8_t *error);
void OLED_StartDrawing(uint8_t *error);

#endif
