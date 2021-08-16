#ifndef stepmotor_h
#define stepmotor_h

#include "../dio/dio.h"
#include "defines.h"

#define SM_CLOCKWISE 0u
#define SM_COUNTERCLOCKWISE 1u

#define SM_STEPS_IN_360_DEGREES 2048u

typedef struct
{
	te_DIO_Pins in1;
	te_DIO_Pins in2;
	te_DIO_Pins in3;
	te_DIO_Pins in4;
	/* Direction of rotation */
	uint8_t direction;
	/* Working state */
	uint8_t state;
	/* Current phase */
	int8_t phase;
	/* Current position (in passes) */
	int16_t position;
	/* Number of steps to be done */
	int16_t stepsToBeDone;
} ts_SM_Motor;

extern ts_SM_Motor motor;

extern void SM_Init(void);
extern void SM_Run(void);

#endif