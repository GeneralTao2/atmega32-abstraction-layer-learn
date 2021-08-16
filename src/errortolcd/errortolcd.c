#include "errortolcd.h"

void ETL_Run(void)
{
    ts_ETL_ErrorLog *errorBuffer = 0u;
    uint8_t hexString[STR_8BIT_STRING_LENGTH];
    uint8_t errorIdx = 0u;
    uint8_t tmpErrorIdx = 0u;
    uint8_t errorQuantity = 0u;
    uint8_t LCDidx = 0u;
    

    errorQuantity = GW_Get_ETL_errorBufferPointer();
    errorBuffer = GW_Get_ETL_errorBuffer();
    for(errorIdx = 0; (errorIdx < errorQuantity) && (LCDidx < LCD_LINE_LENGTH); errorIdx++)
    {
        STR_8BitHexToString(hexString, (errorBuffer[errorIdx].object << HALF_OF_BYTE_LENTH) | errorBuffer[errorIdx].error);
        STR_WriteStringToLCD(LCD_LINE_1, LCDidx, STR_8BIT_STRING_LENGTH, (const char*)hexString);
        LCDidx += STR_8BIT_STRING_LENGTH;
        STR_8BitHexToString(hexString, errorBuffer[errorIdx].data);
        STR_WriteStringToLCD(LCD_LINE_1, LCDidx, STR_8BIT_STRING_LENGTH, (const char*)hexString);
        LCDidx += STR_8BIT_STRING_LENGTH;
        tmpErrorIdx++;
    }
    LCDidx = 0u;
    for(errorIdx = tmpErrorIdx; errorIdx < errorQuantity; errorIdx++)
    {
        STR_8BitHexToString(hexString, (errorBuffer[errorIdx].object << HALF_OF_BYTE_LENTH) | errorBuffer[errorIdx].error);
        STR_WriteStringToLCD(LCD_LINE_2, LCDidx, STR_8BIT_STRING_LENGTH, (const char*)hexString);
        LCDidx += STR_8BIT_STRING_LENGTH;
        STR_8BitHexToString(hexString, errorBuffer[errorIdx].data);
        STR_WriteStringToLCD(LCD_LINE_2, LCDidx, STR_8BIT_STRING_LENGTH, (const char*)hexString);
        LCDidx += STR_8BIT_STRING_LENGTH;
    }
}