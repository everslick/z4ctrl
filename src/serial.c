#define _BSD_SOURCE // CRTSCTS

#include <termios.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "serial.h"

static int fd = -1;

static struct termios settings;

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

int
SerialOpen(const char *device) {
   // open port
   if ((fd = open(device, O_RDWR | O_NOCTTY)) < 0) {
      return (SERIAL_ERR_OPEN);
   }

   return (0);
}

int
SerialClose(void) {
   close(fd);

   return (SERIAL_OK);
}

int
SerialFlush(void) {
   tcflush(fd, TCIOFLUSH);

   return (SERIAL_OK);
}

int
SerialSetTimeout(int ms) {
   if (ms < 0) {
      settings.c_cc[VMIN]  = 0;
      settings.c_cc[VTIME] = 0;
   } else {
      settings.c_cc[VMIN]  = (ms) ? 0 : 1;
      settings.c_cc[VTIME] = (ms + 99) / 100; // recalculate from ms to ds
   }

   tcsetattr(fd, TCSANOW, &settings);

   return (SERIAL_OK);
}

int
SerialInit(int baud, const char *format, int rtscts) {
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
   memset(&settings, 0, sizeof (settings));
   settings.c_cflag = cflags;
   settings.c_iflag = IGNPAR;

   SerialSetTimeout(0);
   SerialFlush();

   return (0);
}

int
SerialSendBuffer(const void *buf, unsigned int len) {
   int written;

   while (len > 0) {
      written = write(fd, buf, len);

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
SerialReceiveBuffer(void *buf, unsigned int *len, int timeout) {
   unsigned int length = 0;
   int received;

   SerialSetTimeout(timeout);

   while (*len > 0) {
      received = read(fd, buf, *len);

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
