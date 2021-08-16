#include "oled.h"
#include "images.h"

#include <avr/pgmspace.h>

#include "../utils/utils.h"
#include "../twsi/twsi.h"
#include "../stringmanager/stringmanager.h"
#include "../signalgateway/signalgateway.h"
#include "../dio/dio.h"
#include "../defines.h"

#define OLED_ADDRESS (0x3C << 1)
#define OLED_BUFFER_SIZE ((OLED_WIDTH * OLED_HEIGHT) / 8)

/* Commands */
#define OLED_SET_DISPLAY_START_LINE_CMD 0xB0
#define OLED_PAGE_0 0x00
#define OLED_PAGE_1 0x01
#define OLED_PAGE_2 0x02
#define OLED_PAGE_3 0x03
#define OLED_PAGE_4 0x04
#define OLED_PAGE_5 0x05
#define OLED_PAGE_6 0x06
#define OLED_PAGE_7 0x07

#define OLED_PAGE_QUANTITY 8u

#define OLED_SET_DISPLAY_ON_OFF_CMD 0xAE
#define OLED_DISPLAY_OFF 0x00
#define OLED_DISPLAY_ON 0x01

#define OLED_SET_MEMORY_ADRESSING_MODE_CMD 0x20
#define OLED_HORIZONTAL_ADRESSING_MODE_DATA 0x00
#define OLED_VERTICAL_ADRESSING_MODE_DATA 0x01
#define OLED_PAGE_ADRESSING_MODE_DATA 0x10

#define OLED_SET_COM_OUTPUT_SCAN_DIRECTION_CMD 0xC0
#define OLED_NORMAL_MODE 0x00
#define OLED_REMAPPED_MODE 0x08

#define OLED_SET_LOWER_COLUMN_START_ADRESS_CMD 0x00

#define OLED_SET_HIGHER_COLUMN_START_ADRESS_CMD 0x10

#define OLED_SET_CONTRAST_CONTROL_CMD 0x81

#define OLED_SET_SEGMENT_RE_MAP_CMD 0xA0
#define OLED_COLUMN_ADRESS_0_IS_MAPPED_TO_SEG0 0x00
#define OLED_COLUMN_ADRESS_127_IS_MAPPED_TO_SEG0 0x01

#define OLED_SET_NORMAL_INVERSE_DISPLAY_CMD 0xA6
#define OLED_NORMAL_DISPLAY 0x00
#define OLED_INVERSE_DISPLAY 0x01

#define OLED_SET_MULTIPLEX_RATIO_CMD 0xA8

#define OLED_SET_DISPLAY_OFFSET_CMD 0xD3

#define OLED_SET_DISPLAY_CLOCK_DIVIDE_RATIO_OSCILATOR_REQUENCY_CMD 0xD5

#define OLED_SET_PRE_CHARGE_PERIOD_CMD 0xD9

#define OLED_SET_COM_PIN_HARDWARE_CONFIGURATION_CMD 0xDA
#define OLED_COM_PIN_HARDWARE_CONFIGURATION_DATA 0x02
#define OLED_SEQUENTIAL_COM_PIN_CONFIGURATION 0x00
#define OLED_ALTERNATIVE_COM_PIN_CONFIGURATION 0x10
#define OLED_DISABLE_COM_LEFT_RIGHT_RE_MAP 0x00
#define OLED_ENABLE_COM_LEFT_RIGHT_RE_MAP 0x20

#define OLED_SET_V_COMH_DESELECT_LEVEL_CMD 0xDB
#define OLED_V_COMH_DESELECT_LEVEL_DATA 0x00
#define OLED_0_65_X_VCC 0x00
#define OLED_0_77_X_VCC 0x20
#define OLED_0_83_X_VCC 0x30

#define OLED_CHARGE_PUMP_SETTINGS_CMD 0x8D
#define OLED_PUMP_SETTINGS_DATA 0x10
#define OLED_DISABLE_CHATGE_PUMP 0x00
#define OLED_ENABLE_CHARGE_PUMP_DURING_DISPLAY_ON 0x04

#define OLED_ENTIRE_DISPLAY_ON_CMD 0xA4
#define OLED_OUTPUT_FOLLOWS_RAM_CONTENT 0x00
#define OLED_IGNORES_FOLLOWS_RAM_CONTENT 0x01

#define OLED_CONTINIOUS_HORIZONTAL_SCROLL_SETUP_CMD 0x26
#define OLED_RIGHT_HORIZONTAL_SCROLL 0x00
#define OLED_LEFT_HORIZONTAL_SCROLL 0x01
#define OLED_CONTINIOUS_HORIZONTAL_SCROLL_DUMMY_BYTE_A_DATA 0x00
#define OLED_TIME_INTERVAL_5_FRAMES_DATA 0x00
#define OLED_TIME_INTERVAL_64_FRAMES_DATA 0x01
#define OLED_TIME_INTERVAL_128_FRAMES_DATA 0x02
#define OLED_TIME_INTERVAL_256_FRAMES_DATA 0x03
#define OLED_TIME_INTERVAL_3_FRAMES_DATA 0x04
#define OLED_TIME_INTERVAL_4_FRAMES_DATA 0x05
#define OLED_TIME_INTERVAL_25_FRAMES_DATA 0x06
#define OLED_TIME_INTERVAL_2_FRAMES_DATA 0x07
#define OLED_CONTINIOUS_HORIZONTAL_SCROLL_DUMMY_BYTE_E_DATA 0x00
#define OLED_CONTINIOUS_HORIZONTAL_SCROLL_DUMMY_BYTE_F_DATA 0xFF

#define OLED_DEACTIVATESCROLL_CMD 0x2E
#define OLED_ACTIVATESCROLL_CMD 0x2F

#define OLED_INIT_SEQUENCE_LENGTH 35u
#define OLED_TWO_BYTE_SEQUENCE_LENGTH 2u

/* State Machine */
#define OLED_POINT_STOP 0u
#define OLED_POINT_START 1u
#define OLED_POINT_TURN_ON_DISPLAY 2u
#define OLED_POINT_INIT_SEQ 3u
#define OLED_POINT_FILL_BUFFER 4u
#define OLED_POINT_SET_PAGE 5u
#define OLED_POINT_DRAW 6u
#define OLED_POINT_DELAY 7u

static uint8_t OLED_buffer[OLED_BUFFER_SIZE + OLED_PAGE_QUANTITY];
uint8_t OLED_currentImage = 0u;
uint8_t OLED_currentPoint = OLED_POINT_START;
uint8_t OLED_nextPoint = OLED_POINT_STOP;
int16_t OLED_delayCounter = 0u;
int8_t OLED_pageIdx = OLED_PAGE_0;
uint16_t OLED_pagesBufferBeginningIdx = 0u;

static const uint8_t PROGMEM OLED_initSequence[OLED_INIT_SEQUENCE_LENGTH] = {
    OLED_CONTROL_BYTE_COMMAND,
    /* First of all turn off displayy */
    OLED_SET_DISPLAY_ON_OFF_CMD | OLED_DISPLAY_OFF,
    /* Choose horizontal adressing mode */
    OLED_SET_MEMORY_ADRESSING_MODE_CMD,   
    OLED_HORIZONTAL_ADRESSING_MODE_DATA,
    /* Begin from PAGE 0 */
    OLED_SET_DISPLAY_START_LINE_CMD | OLED_PAGE_0,
    /* Remapped mode, so scan from COM[N-1] to COM[0], where N is the multiplex ratio ??? */
    OLED_SET_COM_OUTPUT_SCAN_DIRECTION_CMD | OLED_REMAPPED_MODE, 
    /* Set lower and higher column start adresses ??? */
    OLED_SET_LOWER_COLUMN_START_ADRESS_CMD | 0x00, 
    OLED_SET_HIGHER_COLUMN_START_ADRESS_CMD | 0x00,
    /* Set contrast */
    OLED_SET_CONTRAST_CONTROL_CMD, 
    0xFF,
    /* Drow content from the left to the right direction */ 
    OLED_SET_SEGMENT_RE_MAP_CMD | OLED_COLUMN_ADRESS_127_IS_MAPPED_TO_SEG0, 
    /* Keep display normal */
    OLED_SET_NORMAL_INVERSE_DISPLAY_CMD | OLED_NORMAL_DISPLAY, 
    /* Some kind of offset, but we keep it max */
    OLED_SET_MULTIPLEX_RATIO_CMD, 
    0x3F, 
    /* Set offset, vertically shift by circle, keep 0 */
    OLED_SET_DISPLAY_OFFSET_CMD, 
    0x00, 
    /* Set maximum divid ratio of display clocks and minimum oscilator frequency */
    OLED_SET_DISPLAY_CLOCK_DIVIDE_RATIO_OSCILATOR_REQUENCY_CMD,
    0xF0 | 0x00,
    /* Looks like it changes contrast a bit but not shure ??? */
    OLED_SET_PRE_CHARGE_PERIOD_CMD, 
    0x20 | 0x02, 
    /* Here you can make image "stretched" vertically and drow with strips */
    OLED_SET_COM_PIN_HARDWARE_CONFIGURATION_CMD,
    OLED_COM_PIN_HARDWARE_CONFIGURATION_DATA | OLED_ALTERNATIVE_COM_PIN_CONFIGURATION | OLED_DISABLE_COM_LEFT_RIGHT_RE_MAP,
    /* Select voltage ??? */
    OLED_SET_V_COMH_DESELECT_LEVEL_CMD, 
    OLED_V_COMH_DESELECT_LEVEL_DATA | OLED_0_77_X_VCC, 
    /* Enable charge pump ??? */
    OLED_CHARGE_PUMP_SETTINGS_CMD, 
    OLED_PUMP_SETTINGS_DATA | OLED_ENABLE_CHARGE_PUMP_DURING_DISPLAY_ON, 
    /* Not shure about reason of this commad, but keep it ??? */
    OLED_ENTIRE_DISPLAY_ON_CMD | OLED_OUTPUT_FOLLOWS_RAM_CONTENT,
    /* And finaly turn on display */
    OLED_SET_DISPLAY_ON_OFF_CMD | OLED_DISPLAY_ON,
    /* -------------------- */
    /* Set right horiontal scroll */
    OLED_CONTINIOUS_HORIZONTAL_SCROLL_SETUP_CMD | OLED_RIGHT_HORIZONTAL_SCROLL,
    /* send some dummy byte */
    OLED_CONTINIOUS_HORIZONTAL_SCROLL_DUMMY_BYTE_A_DATA,
    /* define start page adress */
    OLED_PAGE_0,
    /* set time interval between each scroll step in terms of frame frequency */
    OLED_TIME_INTERVAL_2_FRAMES_DATA,
    /* and end page adress */
    OLED_PAGE_7,
    /* send some dummy byte */
    OLED_CONTINIOUS_HORIZONTAL_SCROLL_DUMMY_BYTE_E_DATA,
    /* another dummy byte */
    OLED_CONTINIOUS_HORIZONTAL_SCROLL_DUMMY_BYTE_F_DATA,
};

static uint8_t OLED_twoByteSequence[OLED_TWO_BYTE_SEQUENCE_LENGTH] = {0, 0};

void OLED_Fill(const uint8_t *buffer);
void OLED_ResetDrawingProgress(void);
void OLED_SendTwoByteSequenceUnsecured(uint8_t controlByte, uint8_t dataByte);

void OLED_StopDrawing(uint8_t *error)
{
    if( (OLED_currentPoint == OLED_POINT_DRAW) || (OLED_nextPoint == OLED_POINT_DRAW) ||
        (OLED_currentPoint == OLED_POINT_SET_PAGE) || (OLED_nextPoint == OLED_POINT_SET_PAGE) )
    {
        OLED_currentPoint = OLED_POINT_STOP;
        OLED_nextPoint = OLED_POINT_STOP;
    } else
    {
        (*error) = ERR_CMD_OLED_FAIL_TO_STOP;
    }
}

void OLED_StartDrawing(uint8_t *error)
{
    if(OLED_currentPoint == OLED_POINT_STOP)
    {
        OLED_ResetDrawingProgress();
        OLED_currentPoint = OLED_POINT_SET_PAGE;
    } else
    {
        (*error) = ERR_CMD_OLED_FAIL_TO_START;
    }
}

void OLED_ResetDrawingProgress(void)
{
    OLED_pageIdx = OLED_PAGE_0;
    OLED_pagesBufferBeginningIdx = 0u;
}

void OLED_Fill(const uint8_t *buffer) 
{
    uint8_t pageIdx = 0u;
    uint8_t columnIdx = 0u;
    uint16_t bufferIdx = 0u;
    
    for(pageIdx = OLED_PAGE_0; pageIdx < OLED_PAGE_QUANTITY; pageIdx++) 
    {
        OLED_buffer[bufferIdx + pageIdx] = OLED_CONTROL_BYTE_DATA;
        for(columnIdx = 0u; columnIdx < OLED_WIDTH; columnIdx++)
        {
            OLED_buffer[bufferIdx + (pageIdx + 1) ] = pgm_read_byte( &(buffer[bufferIdx]) );
            bufferIdx++;
        }
    }
}

void OLED_Run(void)
{
    switch (OLED_currentPoint)
    {
    case OLED_POINT_STOP:
        /* Nothing to do */
        break;
    case OLED_POINT_START:
        /* Firt thing we need to do is to reset display */
        /* To do this we just rise and lower GND pin of display */
        /* Here we rise */
        DIO_ConfigurePin(OLED_GND, CP_B, CP_6, CP_R, CP_ON, CP_WR);
        /* wait some time */
        OLED_delayCounter = 100u;
        OLED_currentPoint = OLED_POINT_DELAY;
        OLED_nextPoint = OLED_POINT_TURN_ON_DISPLAY;
        break;
    case OLED_POINT_TURN_ON_DISPLAY:
        /* and lower the GND pin */
        DIO_PinOff(OLED_GND);
        /* and let display start, giving some time */
        OLED_delayCounter = 100u;
        OLED_currentPoint = OLED_POINT_DELAY;
        OLED_nextPoint = OLED_POINT_INIT_SEQ;
        break;
    case OLED_POINT_INIT_SEQ:
        /* Now we can initialize our display */
        TWI_SendBlockT(OLED_ADDRESS, OLED_initSequence, OLED_INIT_SEQUENCE_LENGTH, TWI_SPACE_FLASH);
        
        OLED_delayCounter = OLED_INIT_SEQUENCE_LENGTH + 3u;
        OLED_currentPoint = OLED_POINT_DELAY;
        OLED_nextPoint = OLED_POINT_FILL_BUFFER;
        break;
    case OLED_POINT_FILL_BUFFER:
        /* Here swap images to be drew for more various content */
        if(OLED_currentImage == 0u)
        {   
            OLED_Fill(IMG_microcontroller);
        } else 
        {
            OLED_Fill(IMG_arobs);
        }
        OLED_currentImage = !OLED_currentImage;
        /* Clear error buffer for TWI debug */
        GW_Clear_ETL_errorBufferPointer();

        OLED_ResetDrawingProgress();

        OLED_currentPoint = OLED_POINT_SET_PAGE;
        break;  
    case OLED_POINT_SET_PAGE:
        OLED_SendTwoByteSequenceUnsecured(OLED_CONTROL_BYTE_COMMAND, OLED_SET_DISPLAY_START_LINE_CMD | OLED_pageIdx);

        OLED_delayCounter = OLED_TWO_BYTE_SEQUENCE_LENGTH + 3u;
        OLED_currentPoint = OLED_POINT_DELAY;
        OLED_nextPoint = OLED_POINT_DRAW;
        break;
    case OLED_POINT_DRAW:
        /* and just start to drow */

        TWI_SendBlockT(OLED_ADDRESS, &OLED_buffer[OLED_pagesBufferBeginningIdx], OLED_WIDTH + 1u, TWI_SPACE_RAM);

        OLED_pageIdx++;

        if(OLED_pageIdx >= OLED_PAGE_QUANTITY)
        {
            OLED_nextPoint = OLED_POINT_FILL_BUFFER;
        } else
        {
            OLED_nextPoint = OLED_POINT_SET_PAGE;
        }
       
        OLED_pagesBufferBeginningIdx += OLED_WIDTH + 1u;

        OLED_delayCounter = (OLED_WIDTH + 1u) + 3u;
        OLED_currentPoint = OLED_POINT_DELAY;
        break;
    case OLED_POINT_DELAY:
        if(OLED_delayCounter > 0)
        {
            OLED_delayCounter--;
        } else 
        {
            OLED_currentPoint = OLED_nextPoint;
        }
        break;
    default:
        /* Nothing to do */
        break;
    }
    
}

void OLED_SendTwoByteSequenceUnsecured(uint8_t controlByte, uint8_t dataByte)
{
    OLED_twoByteSequence[0] = controlByte;
    OLED_twoByteSequence[1] = dataByte;
    TWI_SendBlockT(OLED_ADDRESS, (const uint8_t*)OLED_twoByteSequence, OLED_TWO_BYTE_SEQUENCE_LENGTH, TWI_SPACE_RAM);
}

void OLED_SendTwoByteSequenceSecured(uint8_t controlByte, uint8_t dataByte, uint8_t *error)
{
    if(TWI_IsBusy() == D_FALSE)
    {
        OLED_SendTwoByteSequenceUnsecured(controlByte, dataByte);
    } else 
    {
        (*error) = ERR_CMD_TWI_IS_BUSY;
    }
}