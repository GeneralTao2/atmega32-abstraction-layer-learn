#include "buzzer.h"

#include "../dio/dio.h"

static int8_t BuzzerActiveTime = 0u;
static uint8_t BuzzerState = CP_OFF;

void BZ_Init(void)
{
    DIO_ConfigurePin(BUZZER, CP_C, CP_4, CP_R, CP_OFF, CP_WR);
}

void BZ_Bip(const int8_t time)
{
    BuzzerActiveTime = time;
    BuzzerState = CP_ON;
    DIO_PinOn(BUZZER);
}

void BZ_Run(void)
{
    if(BuzzerState == CP_ON)
    {
        if(BuzzerActiveTime <= 0)
        {
            DIO_PinOff(BUZZER);
            BuzzerState = CP_OFF;
        }
        BuzzerActiveTime--;
    }
}