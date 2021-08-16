#ifndef buzzer_h
#define buzzer_h

#include <avr/io.h>

extern void BZ_Init(void);
extern void BZ_Bip(const int8_t time);
extern void BZ_Run(void);

#endif