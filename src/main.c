#include "defines.h"
#include <avr/interrupt.h>
#include "tasktimer/tasktimer.h"
#include "leddisplay/leddisplay.h"
#include "button/button.h"
#include "blinker/blinker.h"
#include "ticker/ticker.h"
#include "adc/adc.h"
#include "signalgateway/signalgateway.h"
#include "lcddispay/lcddisplay.h"
#include "stringmanager/stringmanager.h"
#include "uart/uart.h"
#include "cmd/cmd.h"
#include "buzzer/buzzer.h"
#include "stepmotor/stepmotor.h"
#include "oled/oled.h"
#include "errortolcd/errortolcd.h"
#include "twsi/twsi.h"
#include <avr/pgmspace.h>

#include <util/delay.h>

int main(void)
{
	DIO_Init();
	LCD_Init();
    TT_Init();
	CMD_Init();
	BLK_Init();
	//SM_Init();
	UART_Init();
	BZ_Init();

	DIO_ConfigurePin(LED_0, CP_C, CP_7, CP_I, CP_OFF, CP_WR);
	DIO_ConfigurePin(LED_1, CP_C, CP_6, CP_I, CP_OFF, CP_WR);
	DIO_ConfigurePin(LED_2, CP_C, CP_5, CP_I, CP_OFF, CP_WR);

	DIO_ConfigurePin(TIME_MEASURENMENT, CP_B, CP_5, CP_R, CP_OFF, CP_WR);   

    sei();
	
    while (1) 
    {	
		if(TT_Event1ms == EVENT_ARRIVE) 
		{
			/* 20-38 us */
			LCD_Run();
			TWI_Run();
			OLED_Run();
			TT_Event1ms = EVENT_WAIT;
		}
		if(TT_Event5ms == EVENT_ARRIVE) 
		{
			
			TT_Event5ms = EVENT_WAIT;
		}
		if(TT_Event10ms == EVENT_ARRIVE) 
		{
			/* 43 us */
			BZ_Run();
			//SM_Run();
			CMD_Run();
			
			TT_Event10ms = EVENT_WAIT;
		}
		if(TT_Event100ms == EVENT_ARRIVE) 
		{
			/* 9 us */
			
			BLK_Blink();
			
			
			TT_Event100ms = EVENT_WAIT;
		}
		if(TT_Event1000ms == EVENT_ARRIVE) 
		{
			LCD_FillCurrentCharacters();
			ETL_Run();
			TT_Event1000ms = EVENT_WAIT;
		}
    }
}


