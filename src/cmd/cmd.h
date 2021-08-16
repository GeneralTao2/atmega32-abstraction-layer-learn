#ifndef cmd_h
#define cmd_h

#include "../uart/uart.h"

#define CMD_EMPTY 0xFF
#define CMD_COMMAND_QUANTITY 5u
#define CMD_COMMAND_LENGTH 3u

#define CMD_LED 0
#define CMD_LCD 1
#define CMD_BIP 2
#define CMD_OLD 3
#define CMD_MOT 4

extern void CMD_Init(void);
extern void CMD_Run(void);
extern void CMD_ExecLedCommand(uint8_t *error);
extern void CMD_ExecLCDCommand(uint8_t *error);
extern void CMD_ExecBipCommand(uint8_t *error);
extern void CMD_Execute(uint8_t *error);

#endif