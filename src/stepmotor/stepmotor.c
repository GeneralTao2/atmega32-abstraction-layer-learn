#include "stepmotor.h"

#include "../utils/utils.h"

ts_SM_Motor SM_motor;

void SM_WritePhase(ts_SM_Motor *motor, uint8_t state1, uint8_t state2, uint8_t state3, uint8_t state4);

void SM_InitMotor(ts_SM_Motor *motor, te_DIO_Pins in1, te_DIO_Pins in2, te_DIO_Pins in3, te_DIO_Pins in4) 
{
	motor->direction = SM_CLOCKWISE;
	motor->phase = 0u;
	motor->position = 0u;
	motor->stepsToBeDone = 0;
    motor->in1 = in1;
    motor->in2 = in2;
    motor->in3 = in3;
    motor->in4 = in4;
}

void SM_Init(void)
{
    DIO_ConfigurePin(SM_IN1, CP_D, CP_7, CP_I, CP_OFF, CP_WR);
    DIO_ConfigurePin(SM_IN2, CP_D, CP_6, CP_I, CP_OFF, CP_WR);
    DIO_ConfigurePin(SM_IN3, CP_D, CP_5, CP_I, CP_OFF, CP_WR);
    DIO_ConfigurePin(SM_IN4, CP_D, CP_4, CP_I, CP_OFF, CP_WR);
    SM_InitMotor(&SM_motor, SM_IN1, SM_IN2, SM_IN3, SM_IN4);
}

/* Sets the state of the step motor by ruling on 4 сoils */
void SM_WritePhase(ts_SM_Motor *motor, uint8_t state1, uint8_t state2, uint8_t state3, uint8_t state4) 
{
	DIO_WritePin(motor->in1, state1);
	DIO_WritePin(motor->in2, state2);
	DIO_WritePin(motor->in3, state3);
	DIO_WritePin(motor->in4, state4);
}

/* Alternates between 4 states of the step motor:
 * 1: 1 0 0 0
 * 2: 0 1 0 0
 * 3: 0 0 1 0
 * 4: 0 0 0 1
 */
void SM_Rotate(ts_SM_Motor *motor) 
{
	if(motor->stepsToBeDone > 0)
	{
		switch(motor->phase) 
		{
		case 0:
			SM_WritePhase(motor, CP_ON, CP_OFF, CP_OFF, CP_OFF);
			break;
		case 1:
			SM_WritePhase(motor, CP_OFF, CP_ON, CP_OFF, CP_OFF);
			break;
		case 2:
			SM_WritePhase(motor, CP_OFF, CP_OFF, CP_ON, CP_OFF);
			break;
		case 3:
			SM_WritePhase(motor, CP_OFF, CP_OFF, CP_OFF, CP_ON);
			break;
		default:
			SM_WritePhase(motor, CP_OFF, CP_OFF, CP_OFF, CP_OFF);
		}

		if(motor->direction == SM_CLOCKWISE) 
		{
			motor->phase++;
			motor->position++;
		} else 
		{
			motor->phase--;
			motor->position--;
		}

		if(motor->phase > 3) 
		{
			motor->phase = 0;
		} else
		if(motor->phase < 0) 
		{
			motor->phase = 3;
		} else 
		{
			/* Nothing to do */
		}
		motor->stepsToBeDone--;
	} else
	{
		SM_WritePhase(motor, CP_OFF, CP_OFF, CP_OFF, CP_OFF);
	}
}

void SM_Run(void)
{
    SM_Rotate(&SM_motor);
}