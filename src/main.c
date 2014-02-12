#define _BSD_SOURCE // CRTSCTS

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#define SERIAL_DEVICE           "/dev/ttyUSB0"

#define SERIAL_OK                0 ///< no error
#define SERIAL_ERR              -1 ///< unknown error
#define SERIAL_ERR_OPEN         -2 ///< error while opening the serial port
#define SERIAL_ERR_READ         -3 ///< reading from port failed
#define SERIAL_ERR_WRITE        -4 ///< could not write to port
#define SERIAL_ERR_INIT         -5 ///< parameter mismatch error
#define SERIAL_ERR_TIMEOUT      -6 ///< read did not complete in time

#define UNKNOWN_COMMAND          1
#define WRITE_ERROR              2
#define READ_TIMEOUT             3
#define OPEN_FAILED              4

#define READ_POWER_STATUS       "CR0\r"
#define READ_INPUT_MODE         "CR1\r"
#define READ_LAMP_HOURS         "CR3\r"
#define READ_TEMP_SENSORS       "CR6\r"

#define POWER_ON                "C00\r"
#define POWER_OFF               "C01\r"

#define SCALE_NORMAL            "C0D\r"
#define SCALE_FULL              "C0E\r"
#define SCALE_ZOOM              "C2C\r"
#define SCALE_WIDE_1            "C2D\r"
#define SCALE_WIDE_2            "C2E\r"
#define SCALE_CAPTION           "C63\r"
#define SCALE_FULL_THROUGH      "C65\r"
#define SCALE_NORMAL_THROUGH    "C66\r"

#define LAMP_AUTO_1             "C72\r"
#define LAMP_AUTO_2             "C73\r"
#define LAMP_NORMAL             "C74\r"
#define LAMP_ECONOMY            "C75\r"

#define INPUT_COMPOSIT          "C23\r"
#define INPUT_SVIDEO            "C24\r"
#define INPUT_COMPONENT_1       "C25\r"
#define INPUT_COMPONENT_2       "C26\r"
#define INPUT_VGA               "C50\r"
#define INPUT_HDMI              "C53\r"

#define COLOR_LIVING            "C39\r"
#define COLOR_CREATIVE          "C3A\r"
#define COLOR_CINEMA            "C3B\r"
#define COLOR_USER_1            "C3C\r"
#define COLOR_USER_2            "C3D\r"
#define COLOR_USER_3            "C3E\r"
#define COLOR_USER_4            "C3F\r"
#define COLOR_VIVID             "C40\r"
#define COLOR_DYNAMIC           "C41\r"
#define COLOR_POWERFUL          "C42\r"
#define COLOR_NATURAL           "C43\r"

static int fd = -1;

static struct termios settings;

static unsigned char response[32];

static char *serial_device = SERIAL_DEVICE;

static int
SerialOpen(const char *device) {
   // open port
   if ((fd = open(device, O_RDWR | O_NOCTTY)) < 0) {
      return (SERIAL_ERR_OPEN);
   }

   return (0);
}

static void
SerialClose(void) {
   close(fd);
}

static int
SerialFlush(void) {
   tcflush(fd, TCIOFLUSH);

   return (SERIAL_OK);
}

static void
SerialSetTimeout(int ms) {
   if (ms < 0) {
      settings.c_cc[VMIN]  = 0;
      settings.c_cc[VTIME] = 0;
   } else {
      settings.c_cc[VMIN]  = (ms) ? 0 : 1;
      settings.c_cc[VTIME] = (ms + 99) / 100; // recalculate from ms to ds
   }

   tcsetattr(fd, TCSANOW, &settings);
}

static int
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

static int
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

static int
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

static int
ProcessCommand(char *cmd) {
   unsigned int len = 1;
   int err = 0;

   if (SerialOpen(serial_device)) {
      return (OPEN_FAILED);
   }

   if (SerialInit(19200, "8N1", 0)) {
      SerialClose();   
      return (OPEN_FAILED);
   }

   if (SerialSendBuffer(cmd, 4)) {
      SerialClose();   
      return (WRITE_ERROR);
   }

   for (int i=0; i<sizeof (response); i++) {
      if (SerialReceiveBuffer(&response[i], &len, 5000)) {
         // timeout, don't try to read more bytes
         err = READ_TIMEOUT; break;
      }

      if (response[i] == '?') {
         // unknown command, set error code
         err = UNKNOWN_COMMAND;
      }

      if (response[i] == 0x06) {
         // received ACK, replace by printable character
         response[i] = '!';
      }

      if (response[i] == '\r') {
         // response complete, end loop
         break;
      }
   }

   SerialClose();   

   return (err);
}

int
main(int argc, char **argv) {
   int err = 0;

   if ((argc==1) || ((argc>1) && (!strcmp(argv[1], "help")))) {
      puts("");
      puts("control client for Sanyo PLV-Z4 projector " VERSION " <clemens@1541.org>");
      puts("");
      puts("USAGE: z4ctrl <command> <argument>");
      puts("");
      puts("POSSIBLE COMMANDS:");
      puts("");
      puts("\tC??     ... send generic 3 byte command to the projector");
      puts("\tstatus  ... read power status, video input, lamp usage or temperature sensors");
      puts("\tpower   ... power the projector on or off");
      puts("\tinput   ... select video source");
      puts("\tscaler  ... set image scaler mode");
      puts("\tlamp    ... set lamp mode");
      puts("\tcolor   ... set color mode");
      puts("");
      puts("RETURN CODES:");
      puts("");
      puts("\t0 ... ok");
      puts("\t1 ... unknown command");
      puts("\t2 ... serial open failed");
      puts("\t3 ... serial write error");
      puts("\t4 ... serial read timeout");
      puts("");
      exit(0);
   }

   if ((argc==2) && (argv[1][0]=='C')) {
      char cmd[5];

      snprintf(cmd, 5, "%3s\r", argv[1]);
      if (!(err = ProcessCommand(cmd))) {
         printf("response: %s\n", response);
         exit(0);
      }
   }

   if (!strcmp(argv[1], "status")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         puts("possible status read arguments are:");
         puts("");
         puts("\tpower ... return current power status");
         puts("\tinput ... return selected input");
         puts("\tlamp  ... return houres of lamp use");
         puts("\ttemp  ... return current temperaure sensor values");
         puts("");
         exit(0);
      } else if (!strcmp(argv[2], "power")) {
         if (!(err = ProcessCommand(READ_POWER_STATUS))) {
            char *status = NULL;

            switch (atoi((char *)response)) {
               case 00: status = "power on";                                             break;
               case 80: status = "stand-by";                                             break;
               case 40: status = "processing countdown";                                 break;
               case 20: status = "processing cooling down";                              break;
               case 10: status = "power failure";                                        break;
               case 28: status = "processing cooling down due to abnormal temperature";  break;
               case 88: status = "stand-by due to abnormal temperature or door failure"; break;
               case 24: status = "processing power save / cooling down";                 break;
               case 04: status = "power save";                                           break;
               case 21: status = "processing cooling down after lamp failure";           break;
               case 81: status = "stand-by after cooling down due to lamp failure";      break;
            }

            puts(status);
            exit(0);
         }
      } else if (!strcmp(argv[2], "input")) {
         if (!(err = ProcessCommand(READ_INPUT_MODE))) {
            char *input = NULL;

            switch (atoi((char *)response)) {
               case 0: input = "composit";    break;
               case 1: input = "s-video";     break;
               case 2: input = "component 1"; break;
               case 3: input = "component 2"; break;
               case 4: input = "hdmi";        break;
               case 5: input = "vga";         break;
               case 6: input = "scart";       break;
            }

            puts(input);
            exit(0);
         }
      } else if (!strcmp(argv[2], "lamp")) {
         if (!(err = ProcessCommand(READ_LAMP_HOURS))) {
            printf("%i\n", atoi((char *)response));
            exit(0);
         }
      } else if (!strcmp(argv[2], "temp")) {
         if (!(err = ProcessCommand(READ_TEMP_SENSORS))) {
            printf("%s\n", response);
            exit(0);
         }
      }
   }

   if (!strcmp(argv[1], "power")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         puts("possible power arguments are:");
         puts("");
         puts("\ton    ... switch projector on");
         puts("\toff   ... switch projector to stand-by");
         puts("");
         exit(0);
      } else if (!strcmp(argv[2], "on")) {
         err = ProcessCommand(POWER_ON);
      } else if (!strcmp(argv[2], "off")) {
         err = ProcessCommand(POWER_OFF);
      }
   }

   if (!strcmp(argv[1], "input")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         puts("possible input sources are:");
         puts("");
         puts("\tvideo   ... composit video");
         puts("\ts-video ... super video");
         puts("\tcomp1   ... component video 1");
         puts("\tcomp2   ... component video 2");
         puts("\tvga     ... vga video");
         puts("\thdmi    ... digital hd video");
         puts("");
         exit(0);
      } else if (!strcmp(argv[2], "video")) {
         err = ProcessCommand(INPUT_COMPOSIT);
      } else if (!strcmp(argv[2], "s-video")) {
         err = ProcessCommand(INPUT_SVIDEO);
      } else if (!strcmp(argv[2], "comp1")) {
         err = ProcessCommand(INPUT_COMPONENT_1);
      } else if (!strcmp(argv[2], "comp2")) {
         err = ProcessCommand(INPUT_COMPONENT_2);
      } else if (!strcmp(argv[2], "vga")) {
         err = ProcessCommand(INPUT_VGA);
      } else if (!strcmp(argv[2], "hdmi")) {
         err = ProcessCommand(INPUT_HDMI);
      }
   }

   if (!strcmp(argv[1], "scaler")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         puts("possible image scaler modes are:");
         puts("");
         puts("\toff       ... scaler off");
         puts("\tnormal    ... scale up 4:3 to 19:9 by adding black borders");
         puts("\tzoom      ... scale up 4:3 to 19:9 by cutting edges");
         puts("\tfull      ... strech 4:3 to 19:9 full screen");
         puts("\tstrech    ... strech 4:3 to 16:9 unscaled");
         puts("\twide1     ... strech 4:3 to 19:9 but keep aspect ratio in the center");
         puts("\twide2     ... like wide1 but strech 16:9 with black borders to 16:9 without");
         puts("\tcaption   ... like zoom but keep subtitles on the bottom visible");
         puts("");
         exit(0);
      } else if (!strcmp(argv[2], "off")) {
         err = ProcessCommand(SCALE_NORMAL_THROUGH);
      } else if (!strcmp(argv[2], "normal")) {
         err = ProcessCommand(SCALE_NORMAL);
      } else if (!strcmp(argv[2], "zoom")) {
         err = ProcessCommand(SCALE_ZOOM);
      } else if (!strcmp(argv[2], "full")) {
         err = ProcessCommand(SCALE_FULL);
      } else if (!strcmp(argv[2], "wide1")) {
         err = ProcessCommand(SCALE_WIDE_1);
      } else if (!strcmp(argv[2], "wide2")) {
         err = ProcessCommand(SCALE_WIDE_2);
      } else if (!strcmp(argv[2], "strech")) {
         err = ProcessCommand(SCALE_FULL_THROUGH);
      } else if (!strcmp(argv[2], "caption")) {
         err = ProcessCommand(SCALE_CAPTION);
      }
   }

   if (!strcmp(argv[1], "lamp")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         puts("possible lamp modes are:");
         puts("");
         puts("\tnormal ... standard brightness");
         puts("\tauto1  ... adjusting brightness to input signal");
         puts("\tauto2  ... like auto1 but less bright");
         puts("\teco    ... lowest brightness and power consumption");
         puts("");
         exit(0);
      } else if (!strcmp(argv[2], "normal")) {
         err = ProcessCommand(LAMP_NORMAL);
      } else if (!strcmp(argv[2], "auto1")) {
         err = ProcessCommand(LAMP_AUTO_1);
      } else if (!strcmp(argv[2], "auto2")) {
         err = ProcessCommand(LAMP_AUTO_2);
      } else if (!strcmp(argv[2], "eco")) {
         err = ProcessCommand(LAMP_ECONOMY);
      }
   }

   if (!strcmp(argv[1], "color")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         puts("possible color modes are:");
         puts("");
         puts("\tcreative ... contrasty 3D images in a dark room");
         puts("\tcinema   ... quiet tones of color in a dark room");
         puts("\tnatural  ... color correction off");
         puts("\tliving   ... sport and TV in a bright room");
         puts("\tdynamic  ... contrasty images in a bright room");
         puts("\tpowerful ... big screen in a bright room");
         puts("\tvivid    ... contrasty images to maximum extent");
         puts("\tuser1    ... user preset 1");
         puts("\tuser2    ... user preset 2");
         puts("\tuser3    ... user preset 3");
         puts("\tuser4    ... user preset 4");
         puts("");
         exit(0);
      } else if (!strcmp(argv[2], "creative")) {
         err = ProcessCommand(COLOR_CREATIVE);
      } else if (!strcmp(argv[2], "cinema")) {
         err = ProcessCommand(COLOR_CINEMA);
      } else if (!strcmp(argv[2], "natural")) {
         err = ProcessCommand(COLOR_NATURAL);
      } else if (!strcmp(argv[2], "living")) {
         err = ProcessCommand(COLOR_LIVING);
      } else if (!strcmp(argv[2], "dynamic")) {
         err = ProcessCommand(COLOR_DYNAMIC);
      } else if (!strcmp(argv[2], "powerful")) {
         err = ProcessCommand(COLOR_POWERFUL);
      } else if (!strcmp(argv[2], "vivid")) {
         err = ProcessCommand(COLOR_VIVID);
      } else if (!strcmp(argv[2], "user1")) {
         err = ProcessCommand(COLOR_USER_1);
      } else if (!strcmp(argv[2], "user2")) {
         err = ProcessCommand(COLOR_USER_2);
      } else if (!strcmp(argv[2], "user3")) {
         err = ProcessCommand(COLOR_USER_3);
      } else if (!strcmp(argv[2], "user4")) {
         err = ProcessCommand(COLOR_USER_4);
      }
   }

   switch (err) {
      case UNKNOWN_COMMAND:
         printf("unknown command!\n");
      break;

      case WRITE_ERROR:
         printf("serial write error!\n");
      break;

      case READ_TIMEOUT:
         printf("serial read timeout!\n");
      break;

      case OPEN_FAILED:
         printf("serial device open failed!\n");
      break;

      default:
         //printf("response: %s\n", response);
      break;
   }

   return (err);
}
