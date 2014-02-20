#ifndef _Z4CTRL_ONKYO_H_
#define _Z4CTRL_ONKYO_H_

#define UNKNOWN_COMMAND          1
#define INVALID_ARGUMENT         2
#define OPEN_FAILED              3
#define WRITE_ERROR              4
#define READ_TIMEOUT             5
#define NOT_CONNECTED            6

#define STRING_SIZE             32

#include "serial.h"

extern Serial *onkyo_serial;

int OnkyoProbeDevice(const char *device);

int OnkyoReadStatus(char ret[]);

int OnkyoExecCommand(char ret[], const char *arg);

#endif // _Z4CTRL_ONKYO_H_
