#ifndef _Z4CTRL_SANYO_H_
#define _Z4CTRL_SANYO_H_

#define UNKNOWN_COMMAND          1
#define INVALID_ARGUMENT         2
#define OPEN_FAILED              3
#define WRITE_ERROR              4
#define READ_TIMEOUT             5
#define NOT_CONNECTED            6

#define STRING_SIZE             32

#include "serial.h"

extern Serial *sanyo_serial;

int SanyoProbeDevice(const char *device);

int ReadPowerStatus(char ret[]);
int ReadInputMode(char ret[]);
int ReadLampHours(char ret[]);
int ReadTempSensors(char ret[]);
int ReadModelNumber(char ret[]);

int ExecStatusRead(char ret[], const char *arg);
int ExecPowerCommand(char ret[], const char *arg);
int ExecInputCommand(char ret[], const char *arg);
int ExecScalerCommand(char ret[], const char *arg);
int ExecLampCommand(char ret[], const char *arg);
int ExecColorCommand(char ret[], const char *arg);
int ExecMuteCommand(char ret[], const char *arg);
int ExecLogoCommand(char ret[], const char *arg);
int ExecMenuCommand(char ret[], const char *arg);
int ExecPressCommand(char ret[], const char *arg);
int ExecGenericCommand(char ret[], const char *arg);

#endif // _Z4CTRL_SANYO_H_
