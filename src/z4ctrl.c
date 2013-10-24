#define _BSD_SOURCE // CRTSCTS

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#define SERIAL_DEVICE           "/dev/ttyusb0"

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
#define READ_LAMP_TIME          "CR3\r"
#define READ_TEMP_SENSORS       "CR6\r"

#define POWER_ON                "C00\r"
#define POWER_OFF               "C01\r"

#define SIZE_NORMAL             "C0D\r"
#define SIZE_FULL               "C0E\r"
#define SIZE_ZOOM               "C2C\r"
#define SIZE_WIDE1              "C2D\r"
#define SIZE_WIDE2              "C2E\r"
#define SIZE_FULL_THRU          "C65\r"
#define SIZE_NORMAL_THRU        "C66\r"

#define LAMP_AUTO1              "C72\r"
#define LAMP_AUTO2              "C73\r"
#define LAMP_NORMAL             "C74\r"
#define LAMP_ECO                "C75\r"

#define INPUT_COMPOSIT          "C23\r"
#define INPUT_SVIDEO            "C24\r"
#define INPUT_COMPONENT1        "C25\r"
#define INPUT_COMPONENT2        "C26\r"
#define INPUT_VGA               "C50\r"
#define INPUT_HDMI              "C53\r"

#define COLOR_LIVING            "C39\r"
#define COLOR_CREATIVE_CINEMA   "C3A\r"
#define COLOR_PURE_CINEMA       "C3B\r"
#define COLOR_USER1             "C3C\r"
#define COLOR_USER2             "C3D\r"
#define COLOR_USER3             "C3E\r"
#define COLOR_USER4             "C3F\r"
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

   if ((argc==1) || ((argc>1) && (!strcmp(argv[1], "--help")))) {
		puts("");
		puts("control client for Sanyo PLV-Z4 projector " VERSION " <clemens@1541.org>");
		puts("");
		puts("USAGE: z4ctrl <command> <argument>");
		puts("");
		puts("POSSIBLE COMMANDS:");
		puts("");
		puts("\tC??     ... send generic 3 byte command to the projector");
		puts("\tstatus  ... read <power>, <input>, <lamp> or <input> status");
		puts("\tpower   ... power the projector <on> or <off>");
		puts("\tinput   ... set video source to <video>, <s-video>, <comp1>, <comp2>, <vga> or <hdmi>");
		puts("\tscreen  ...                                  ");
		puts("\tlamp    ...                                  ");
		puts("\tcolor   ...                                  ");
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
		err = ProcessCommand(cmd);
	}

   if (!strcmp(argv[1], "status")) {
		if ((argc<3) || (!strcmp(argv[2], "help"))) {
			puts("possible status read commands are:");
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
					case 00: status = "Power ON";                                              break;
					case 80: status = "Standby";                                               break;
					case 40: status = "Processing countdown";                                  break;
					case 20: status = "Processing cooling down";                               break;
					case 10: status = "Power failure";                                         break;
					case 28: status = "Processing cooling down due to abnormal temperature";   break;
					case 88: status = "Standby due to abnormal temperature or door failure";   break;
					case 24: status = "Processing power save / Cooling down";                  break;
					case 04: status = "Power save";                                            break;
					case 21: status = "Processing cooling down after OFF due to lamp failure"; break;
					case 81: status = "Standby after cooling down due to lamp failure";        break;
				}

				puts(status);
				exit(0);
			}
		} else if (!strcmp(argv[2], "input")) {
			if (!(err = ProcessCommand(READ_INPUT_MODE))) {
				char *input = NULL;

				switch (atoi((char *)response)) {
					case 0: input = "Video";                 break;
					case 1: input = "S-Video";               break;
					case 2: input = "Component1";            break;
					case 3: input = "Component2 (D4-Video)"; break;
					case 4: input = "HDMI";                  break;
					case 5: input = "Computer (Analog)";     break;
					case 6: input = "Computer (Scart)";      break;
				}

				puts(input);
				exit(0);
			}
		} else if (!strcmp(argv[2], "lamp")) {
			err = ProcessCommand(READ_LAMP_TIME);
		} else if (!strcmp(argv[2], "temp")) {
			err = ProcessCommand(READ_TEMP_SENSORS);
		}
	}

   if (!strcmp(argv[1], "power")) {
		if ((argc<3) || (!strcmp(argv[2], "help"))) {
			puts("possible power commands are:");
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
			printf("response: %s\n", response);
		break;
	}

	return (err);
}
