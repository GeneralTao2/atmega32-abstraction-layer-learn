#include "ticker.h"

#include "../dio/dio.h"
#include "../defines.h"
#include "../signalgateway/signalgateway.h"

/*
 * \def: LED_QUANTITY
 * \brief: The quantity of leds to be used in the program
 */
#define LED_QUANTITY 8

/*
 * \def: LED_*
 * \brief: Green leds enum name
 */
#define GREEN_LED_1 LED_0
#define GREEN_LED_2 LED_2
#define GREEN_LED_3 LED_4
#define GREEN_LED_4 LED_6

/*
 * \def: LED_*
 * \brief: Red leds enum name
 */
#define RED_LED_1 LED_1
#define RED_LED_2 LED_3
#define RED_LED_3 LED_5
#define RED_LED_4 LED_7

/*
 * \def: TCK_INCREMENTOR_*
 * \brief: A value to be added to TCK_Counter
 */
#define TCK_INCREMENTOR_0 0
#define TCK_INCREMENTOR_1 1
#define TCK_INCREMENTOR_2 2

/*
 * \def: TCK_DIRECTION_FRWD
 * \brief: Shows a direction of the ticker. Ticker steps forward
 */
#define TCK_DIRECTION_FRWD 0

/*
 * \def: TCK_DIRECTION_BACK
 * \brief: Shows a direction of the ticker. Ticker steps back
 */
#define TCK_DIRECTION_BACK 1

/*
 * \def: TCK_COUNTER_MAX
 * \brief: The maximum value of TCK_Counter
 */
#define TCK_COUNTER_MAX 2

/*
 * \def: const uint8_t TCK_leds[LED_QUANTITY]
 * \brief: The array of TCK_leds enums. Sest a sequence of the TCK_leds actuation
 */
const uint8_t TCK_leds[LED_QUANTITY] = {
	GREEN_LED_1,
	RED_LED_1,
	GREEN_LED_2,
	RED_LED_2,
	GREEN_LED_3,
	RED_LED_3,
	GREEN_LED_4,
	RED_LED_4,
};

/*
 * \def: int8_t TCK_indexPlace
 * \brief: The index of a current active led.
 */
int8_t TCK_indexPlace = 0;

/*
 * \def: uint8_t TCK_counter
 * \brief: The counter, what counts the period of one ticker step
 */
uint8_t TCK_counter = 0;

/*
 * \def: uint8_t TCK_incrementor
 * \brief: The value, what will be added to TCK_counter every TCK_Run execution
 */
uint8_t TCK_incrementor = 0;

/*
 * \def: uint8_t TCK_lastSpeed
 * \brief: The identifire of the last speed of ticking
 */
uint8_t TCK_lastSpeed = TCK_SPEED_0;

/*
 * \def: uint8_t TCK_direction
 * \brief: An identifire of the current ticker direction, 
 *      can be TCK_DIRECTION_FRWD - forward or TCK_DIRECTION_BACK - BACK
 */
uint8_t TCK_direction = TCK_DIRECTION_FRWD;

void TCK_StepFrwd(void);
void TCK_StepBack(void);
void TCK_Step(void);

/**
 * void TCK_Init(void)
 * \brief: 
 * 		Initializes the ticker 	 
 * \description: 
 * 		This function initializes each led of ticker 
 * \return value:
 * 		No return value
 */
void TCK_Init(void)
{
	DIO_ConfigurePin(GREEN_LED_1, 	CP_C, CP_0, CP_I, CP_ON, CP_WR);
	DIO_ConfigurePin(RED_LED_1, 	CP_C, CP_1, CP_I, CP_OFF, CP_WR);
	DIO_ConfigurePin(GREEN_LED_2, 	CP_C, CP_2, CP_I, CP_OFF, CP_WR);
	DIO_ConfigurePin(RED_LED_2, 	CP_C, CP_3, CP_I, CP_OFF, CP_WR);
	DIO_ConfigurePin(GREEN_LED_3, 	CP_C, CP_4, CP_I, CP_OFF, CP_WR);
	DIO_ConfigurePin(RED_LED_3,		CP_C, CP_5, CP_I, CP_OFF, CP_WR);
	DIO_ConfigurePin(GREEN_LED_4, 	CP_C, CP_6, CP_I, CP_OFF, CP_WR);
	DIO_ConfigurePin(RED_LED_4, 	CP_C, CP_7, CP_I, CP_OFF, CP_WR);
}

/**
 * void TCK_StepFrwd(void) 
 * \brief: 
 * 		Does step forward in ticker	 
 * \description: 
 * 		This function turns off the currect led and turns on the next led (ahead one) 
 * \return value:
 * 		No return value
 */
void TCK_StepFrwd(void) 
{
	TCK_indexPlace++;

	if(TCK_indexPlace > LED_QUANTITY-1) {
        TCK_indexPlace = 0;
    }

    if(TCK_indexPlace == 0) {
        DIO_PinOff( TCK_leds[LED_QUANTITY-1] );
        DIO_PinOn( TCK_leds[0] );
    } else {
        DIO_PinOff( TCK_leds[TCK_indexPlace-1] );
        DIO_PinOn( TCK_leds[TCK_indexPlace] );
    }
}

/**
 * void TCK_StepBack(void) 
 * \brief: 
 * 		Does step back in ticker	 
 * \description: 
 * 		This function turns off the currect led and turns on the next led 
 *      (from behind one) 
 * \return value:
 * 		No return value
 */
void TCK_StepBack(void) 
{
	TCK_indexPlace--;

	if(TCK_indexPlace < 0) {
        TCK_indexPlace = LED_QUANTITY-1;
    }
	
    if(TCK_indexPlace == LED_QUANTITY-1) {
        DIO_PinOff( TCK_leds[0] );
        DIO_PinOn( TCK_leds[LED_QUANTITY-1] );
    } else {
        DIO_PinOff( TCK_leds[TCK_indexPlace+1] );
        DIO_PinOn( TCK_leds[TCK_indexPlace] );
    }
}


/**
 * void TCK_Refresh(void)
 * \brief: 
 * 		Refreshs data about speed and direction of ticking
 * \description: 
 * 		This function takes data about current speed and direction
 *      of  ticker from signal gateway. 
 * \return value:
 * 		No return value
 */
void TCK_Refresh(void) 
{
    uint8_t tmpSpeed = TCK_SPEED_0;
    GW_Read_TCK_CurrentSpeed(&tmpSpeed);
    switch (tmpSpeed)
    {
    case TCK_SPEED_0:
        TCK_incrementor = TCK_INCREMENTOR_0;
        break;
    case TCK_SPEED_1_BACK:
        TCK_incrementor = TCK_INCREMENTOR_1;
        TCK_direction = TCK_DIRECTION_BACK;
        break;
    case TCK_SPEED_2_BACK:
        TCK_incrementor = TCK_INCREMENTOR_2;
        TCK_direction = TCK_DIRECTION_BACK;
        break;
    case TCK_SPEED_1_FRWD:
        TCK_incrementor = TCK_INCREMENTOR_1;
        TCK_direction = TCK_DIRECTION_FRWD;
        break;
    case TCK_SPEED_2_FRWD:
        TCK_incrementor = TCK_INCREMENTOR_2;
        TCK_direction = TCK_DIRECTION_FRWD;
        break;
    default:
        break;
    }
    if(tmpSpeed != TCK_lastSpeed) {
        TCK_counter = 0;
    }
    TCK_lastSpeed = tmpSpeed;
}

/**
 * void TCK_Step(void) 
 * \brief: 
 * 		Does step forward or back 
 * \description: 
 * 		This function does one step depending on the current direction
 * \return value:
 * 		No return value
 */
void TCK_Step(void) 
{
    if(TCK_direction == TCK_DIRECTION_FRWD) 
    {
        TCK_StepFrwd();
    } else {
        TCK_StepBack();
    }
}

/**
 * void TCK_Step(void) 
 * \brief: 
 * 		Does step forward or back in with certain period
 * \description: 
 * 		This function does one step depending on the current direction. Also controls
 *      a preiod of stepping. So, period can take 1, 2 or inf function execution. Inf means,
 *      that TCK_incrementor is 0, so in this this case ticker will not do step
 * \return value:
 * 		No return value
 */
void TCK_Run(void) 
{
    TCK_counter += TCK_incrementor;
    if(TCK_counter >= TCK_COUNTER_MAX) 
    {
        TCK_Step();
        TCK_counter = 0;
    }
} 
