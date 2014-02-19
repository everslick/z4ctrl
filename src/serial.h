#ifndef _Z4CTRL_SERIAL_H_
#define _Z4CTRL_SERIAL_H_

#include <termios.h>

#define SERIAL_OK                0 ///< no error
#define SERIAL_ERR              -1 ///< unknown error
#define SERIAL_ERR_OPEN         -2 ///< error while opening the serial port
#define SERIAL_ERR_READ         -3 ///< reading from port failed
#define SERIAL_ERR_WRITE        -4 ///< could not write to port
#define SERIAL_ERR_INIT         -5 ///< parameter mismatch error
#define SERIAL_ERR_TIMEOUT      -6 ///< read did not complete in time

typedef struct Serial {
   int fd;
   struct termios settings;
} Serial;

int SerialListDevices(char *device[], unsigned int *number);

Serial *SerialOpen(const char *device);

int SerialClose(Serial *serial);
int SerialInit(Serial *serial, int baud, const char *format, int rtscts);
int SerialFlush(Serial *serial);
int SerialSetTimeout(Serial *serial, int ms);
int SerialSendBuffer(Serial *serial, const void *buf, unsigned int len);
int SerialReceiveBuffer(Serial *serial, void *buf, unsigned int *len, int timeout);

#endif // _Z4CTRL_SERIAL_H_
