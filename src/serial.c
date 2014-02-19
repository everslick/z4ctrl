#define _BSD_SOURCE // CRTSCTS

#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "serial.h"

// TODO check for NULL pointer
// TODO check for serial->fd == -1

int
SerialListDevices(char *device[], unsigned int *number) {
   struct dirent **entry = NULL;
   unsigned int count = 0;
   char *file = NULL;

   if (chdir("/dev/serial/by-path")) {
      return (SERIAL_ERR_OPEN);
   }

   int n = scandir(".", &entry, 0, alphasort);

   for (int i=0; i<n; i++) {
      if (strcmp(".", entry[i]->d_name) && strcmp("..", entry[i]->d_name)) {
         if ((count < *number) && (file = realpath(entry[i]->d_name, NULL))) {
            device[count++] = file;
         }
      }
      free(entry[i]);
   }

   free(entry);

   *number = count;

   return (SERIAL_OK);
}

Serial *
SerialOpen(const char *device) {
   int fd = -1;

   // open port
   if ((fd = open(device, O_RDWR | O_NOCTTY)) < 0) {
      return (NULL);
   }

	Serial *serial = malloc(sizeof (Serial));

   memset(serial, 0, sizeof (Serial));
   serial->fd = fd;

   return (serial);
}

int
SerialClose(Serial *serial) {
   close(serial->fd);

   return (SERIAL_OK);
}

int
SerialFlush(Serial *serial) {
   tcflush(serial->fd, TCIOFLUSH);

   return (SERIAL_OK);
}

int
SerialSetTimeout(Serial *serial, int ms) {
   if (ms < 0) {
      serial->settings.c_cc[VMIN]  = 0;
      serial->settings.c_cc[VTIME] = 0;
   } else {
      serial->settings.c_cc[VMIN]  = (ms) ? 0 : 1;
      serial->settings.c_cc[VTIME] = (ms + 99) / 100; // calc from ms to ds
   }

   tcsetattr(serial->fd, TCSANOW, &serial->settings);

   return (SERIAL_OK);
}

int
SerialInit(Serial *serial, int baud, const char *format, int rtscts) {
   int cflags = CLOCAL | CREAD;

   if ((!format) || (strlen(format) != 3)) {
      return (SERIAL_ERR_INIT);
   }

   // datasize
   switch (format[0]) {
      case '5': cflags |= CS5; break;
      case '6': cflags |= CS6; break;
      case '7': cflags |= CS7; break;
      case '8': cflags |= CS8; break;
      default: return (SERIAL_ERR_INIT);
   }

   // parity
   switch (format[1]) {
      case 'N': break;                         // no parity
      case 'O': cflags |= PARODD; // no break! // odd parity
      case 'E': cflags |= PARENB; break;       // even parity
      default: return (SERIAL_ERR_INIT);
   }

   // stopbit
   switch (format[2]) {
      case '1': break;                   // 1 stopbit
      case '2': cflags |= CSTOPB; break; // 2 stopbits
      default: return (SERIAL_ERR_INIT);
   }

   // baudrate
   switch (baud) {
      case 115200: cflags |= B115200; break;
      case  57600: cflags |= B57600;  break;
      case  38400: cflags |= B38400;  break;
      case  19200: cflags |= B19200;  break;
      case   9600: cflags |= B9600;   break;
      case   4800: cflags |= B4800;   break;
      case   2400: cflags |= B2400;   break;
      case    300: cflags |= B300;    break;
      default: return (SERIAL_ERR_INIT);
   }

   // handshake
   if (rtscts) {
      cflags |= CRTSCTS; // hardware handshake
   }

   // clear struct and set port parameters
   memset(&serial->settings, 0, sizeof (serial->settings));
   serial->settings.c_cflag = cflags;
   serial->settings.c_iflag = IGNPAR;

   SerialSetTimeout(serial, 0);
   SerialFlush(serial);

   return (0);
}

int
SerialSendBuffer(Serial *serial, const void *buf, unsigned int len) {
   int written;

   while (len > 0) {
      written = write(serial->fd, buf, len);

      if (written < 0) {
         if (errno == EINTR) continue;

         return (SERIAL_ERR_WRITE);
      }

      len -= written;
      buf = (char *)buf + written;
   }

   return (SERIAL_OK);
}

int
SerialReceiveBuffer(Serial *serial, void *buf, unsigned int *len, int timeout) {
   unsigned int length = 0;
   int received;

   SerialSetTimeout(serial, timeout);

   while (*len > 0) {
      received = read(serial->fd, buf, *len);

      if (received > 0) length += received;

      if (received < 0) {
         if (errno == EINTR) continue;

         *len = length;

         return (SERIAL_ERR_READ);
      } else if (received == 0) {
         *len = length;

         if (timeout < 0) return (SERIAL_OK);

         return (SERIAL_ERR_TIMEOUT);
      }

      *len -= received;
      buf = (char *)buf + received;
   }

   *len = length;

   return (SERIAL_OK);
}
