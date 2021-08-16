#include "leddisplay.h"
#include "../dio/dio.h"
#include "../signalgateway/signalgateway.h"

/*
 *          a
 *      f       c
 *          g
 *      e       b
 *          d       dp
 * 
 * 0  7 6 5 4 3 2 1
 * dp g f e d c b a
 */ 
#define LD_DIGIT_0 0b00111111
#define LD_DIGIT_1 0b00000110
#define LD_DIGIT_2 0b01011011
#define LD_DIGIT_3 0b01001111
#define LD_DIGIT_4 0b01100110
#define LD_DIGIT_5 0b01101101
#define LD_DIGIT_6 0b01111101
#define LD_DIGIT_7 0b00000111
#define LD_DIGIT_8 0b01111111
#define LD_DIGIT_9 0b01101111
#define LD_DIGIT_A 0b01110111
#define LD_DIGIT_B 0b01111100
#define LD_DIGIT_C 0b00111001
#define LD_DIGIT_D 0b01011110
#define LD_DIGIT_E 0b01111001
#define LD_DIGIT_F 0b01110001
#define LD_DIGIT_r 0b01010000
#define LD_DIGIT_DEG 0b01011010 /* ° */
#define LD_DIGIT_ERR 0b10010010

#define LD_DECADE_1_ID 0
#define LD_DECADE_2_ID 1
#define LD_DECADE_3_ID 2
#define LD_DECADE_4_ID 3

#define LD_SEGS_QUANTITY 7

#define LD_DECADE_QUANTITY 4

#define DECADE_OFF 0
#define DECADE_ON 1

#define LD_CHAR_QUANTITY 18

/* Min max value of LED display */
#define LD_MAX_VALUE (int16_t)9999
#define LD_MIN_VALUE (int16_t)0

uint8_t LD_selectedDecade = LD_DECADE_1_ID;

/* Access by ID */
uint8_t LD_decadeContain[LD_DECADE_QUANTITY] = {
    1, 2, 3, 4
};

uint8_t LD_chars[LD_CHAR_QUANTITY] = {
    LD_DIGIT_0,
    LD_DIGIT_1,
    LD_DIGIT_2,
    LD_DIGIT_3,
    LD_DIGIT_4,
    LD_DIGIT_5,
    LD_DIGIT_6,
    LD_DIGIT_7,
    LD_DIGIT_8,
    LD_DIGIT_9,
    LD_DIGIT_A,
    LD_DIGIT_B,
    LD_DIGIT_C,
    LD_DIGIT_D,
    LD_DIGIT_E,
    LD_DIGIT_F,
    LD_DIGIT_r,
    LD_DIGIT_DEG
};

uint8_t decadeSegments[LD_SEGS_QUANTITY] = {
    LD_SEG_A,
    LD_SEG_B,
    LD_SEG_C,
    LD_SEG_D,
    LD_SEG_E,
    LD_SEG_F,
    LD_SEG_G,
};

uint8_t decadeDIOs[LD_DECADE_QUANTITY] = {
    LD_DECADE_1,
    LD_DECADE_2,
    LD_DECADE_3,
    LD_DECADE_4
};

void LD_Init(void);
void LD_numberToDecadeNumber(int16_t number);
void LD_WriteDecadeByDIO(uint8_t decade, uint8_t segments, uint8_t point);
void LD_WriteDecadeByID(uint8_t decadeID, uint8_t digit, uint8_t point);
uint8_t LD_DigitToSegments(uint8_t digit);

void LD_numberToDecadeNumber(int16_t number) 
{
    LD_decadeContain[LD_DECADE_1_ID] = number/1000;
    LD_decadeContain[LD_DECADE_2_ID] = (number%1000)/100;
    LD_decadeContain[LD_DECADE_3_ID] = (number%100)/10;
    LD_decadeContain[LD_DECADE_4_ID] = number%10;
}

void LD_Init(void) 
{
    /* Katode. Common */
    DIO_ConfigurePin(LD_SEG_DOT,    CP_D, CP_7, CP_R, CP_ON, CP_WR);
    DIO_ConfigurePin(LD_SEG_A,      CP_D, CP_6, CP_R, CP_ON, CP_WR);
    DIO_ConfigurePin(LD_SEG_B,      CP_D, CP_5, CP_R, CP_ON, CP_WR);
    DIO_ConfigurePin(LD_SEG_C,      CP_D, CP_4, CP_R, CP_ON, CP_WR);
    DIO_ConfigurePin(LD_SEG_D,      CP_D, CP_3, CP_R, CP_ON, CP_WR);
    DIO_ConfigurePin(LD_SEG_E,      CP_D, CP_2, CP_R, CP_ON, CP_WR);
    DIO_ConfigurePin(LD_SEG_F,      CP_B, CP_5, CP_R, CP_ON, CP_WR);
    DIO_ConfigurePin(LD_SEG_G,      CP_B, CP_4, CP_R, CP_ON, CP_WR);

    /* Anode. Using inverse logic because  of NPN transistor */
    DIO_ConfigurePin(LD_DECADE_1, CP_B, CP_0, CP_I, CP_OFF, CP_WR);
    DIO_ConfigurePin(LD_DECADE_2, CP_B, CP_1, CP_I, CP_OFF, CP_WR);
    DIO_ConfigurePin(LD_DECADE_3, CP_B, CP_2, CP_I, CP_OFF, CP_WR);
    DIO_ConfigurePin(LD_DECADE_4, CP_B, CP_3, CP_I, CP_OFF, CP_WR);

    /* Conversion of init value */
    LD_Refresh();
}

void LD_ShowNextDecade(void) 
{
    /* Increment current decade place */
    LD_selectedDecade++;/*todo*/
    if(LD_selectedDecade >= LD_DECADE_QUANTITY) 
    {
        LD_selectedDecade = 0;
    }

    /* And drow a character */
    LD_WriteDecadeByID(LD_selectedDecade, LD_decadeContain[LD_selectedDecade], CP_OFF);
}

void LD_WriteDecadeByDIO(uint8_t decade, uint8_t segments, uint8_t point) 
{
    uint8_t segValue = CP_OFF;
    uint8_t i = 0u;

    for(i=0; i<LD_SEGS_QUANTITY; i++) 
    {
        segValue = READ_BIT(segments, i);
        if(segValue == CP_OFF) {
            DIO_PinOn( decadeSegments[i] );
        } else {
            DIO_PinOff( decadeSegments[i] );
        }
    }
    for(uint8_t i=0; i<LD_DECADE_QUANTITY; i++) 
    {    
        DIO_PinOff( decadeDIOs[i] );
    }

    DIO_PinOn(decade);

    if(point == CP_OFF) 
    {
        DIO_PinOn(LD_SEG_DOT);
    } else 
    {
        DIO_PinOff(LD_SEG_DOT);
    }
}

uint8_t LD_DigitToSegments(uint8_t digit) 
{
    uint8_t segments = LD_DIGIT_ERR;

    if(digit < LD_CHAR_QUANTITY) {
        segments = LD_chars[digit];
    } else {
        segments = LD_DIGIT_ERR;
    }

    return segments;
}

void LD_WriteDecadeByID(uint8_t decadeID, uint8_t digit, uint8_t point) 
{
    uint8_t decadeSegments = LD_DIGIT_ERR;

    decadeSegments = LD_DigitToSegments(digit);

    switch(decadeID)
    {
    case LD_DECADE_1_ID:
        LD_WriteDecadeByDIO(LD_DECADE_1, decadeSegments, point);
        break;
    case LD_DECADE_2_ID:
        LD_WriteDecadeByDIO(LD_DECADE_2, decadeSegments, point);
        break;
    case LD_DECADE_3_ID:
        LD_WriteDecadeByDIO(LD_DECADE_3, decadeSegments, point);
        break;
    case LD_DECADE_4_ID:
        LD_WriteDecadeByDIO(LD_DECADE_4, decadeSegments, point);
        break;
    default:
        break;
    }
}


void LD_Refresh(void)
{
    uint16_t tmpDispayValue = 0;
    GW_Read_LedDispayValue(&tmpDispayValue);
    LD_numberToDecadeNumber(tmpDispayValue);
}

