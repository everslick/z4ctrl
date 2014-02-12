#ifndef _Z4CTRL_SERIAL_H_
#define _Z4CTRL_SERIAL_H_

#define SERIAL_DEVICE           "/dev/ttyUSB0"

#define SERIAL_OK                0 ///< no error
#define SERIAL_ERR              -1 ///< unknown error
#define SERIAL_ERR_OPEN         -2 ///< error while opening the serial port
#define SERIAL_ERR_READ         -3 ///< reading from port failed
#define SERIAL_ERR_WRITE        -4 ///< could not write to port
#define SERIAL_ERR_INIT         -5 ///< parameter mismatch error
#define SERIAL_ERR_TIMEOUT      -6 ///< read did not complete in time

int SerialOpen(const char *device);
int SerialInit(int baud, const char *format, int rtscts);
int SerialClose(void);
int SerialFlush(void);
int SerialSetTimeout(int ms);
int SerialSendBuffer(const void *buf, unsigned int len);
int SerialReceiveBuffer(void *buf, unsigned int *len, int timeout);

#endif // _Z4CTRL_SERIAL_H_
