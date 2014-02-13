#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "command.h"
#include "serial.h"
#include "ctrl.h"

char *serial_device = NULL;

static int
ProcessCommand(char ret[], const char *cmd) {
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

   memset(ret, 0, STRING_SIZE);

   for (int i=0; i<STRING_SIZE; i++) {
      if (SerialReceiveBuffer(&ret[i], &len, 3000)) {
         // timeout, don't try to read more bytes
         err = READ_TIMEOUT; break;
      }

      if (ret[i] == '?') {
         // unknown command, set error code
         err = UNKNOWN_COMMAND;
      }

      if (ret[i] == 0x06) {
         // received ACK, replace by printable character
         ret[i] = '!';
      }

      if (ret[i] == '\r') {
         // ret complete, end loop
         break;
      }
   }

   SerialClose();   

   return (err);
}

int
ReadPowerStatus(char ret[]) {
   char *status = NULL;
   int err;

   if (!(err = ProcessCommand(ret, READ_POWER_STATUS))) {
      switch (atoi((char *)ret)) {
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

      strcpy(ret, status);
   }

   return (err);
}

int
ReadInputMode(char ret[]) {
   char *input = NULL;
   int err;

   if (!(err = ProcessCommand(ret, READ_INPUT_MODE))) {
      switch (atoi((char *)ret)) {
         case 0: input = "composit";    break;
         case 1: input = "s-video";     break;
         case 2: input = "component 1"; break;
         case 3: input = "component 2"; break;
         case 4: input = "hdmi";        break;
         case 5: input = "vga";         break;
         case 6: input = "scart";       break;
      }

      strcpy(ret, input);
   }

   return (err);
}

int
ReadLampHours(char ret[]) {
   int err;

   if (!(err = ProcessCommand(ret, READ_LAMP_HOURS))) {
      sprintf(ret, "%i", atoi(ret)); // strip leading zeros
   }

   return (err);
}

int
ReadTempSensors(char ret[]) {
   int err = ProcessCommand(ret, READ_TEMP_SENSORS);

   return (err);
}

int
ReadModelNumber(char ret[]) {
   int err = ProcessCommand(ret, READ_MODEL_NUMBER);

   return (err);
}

int
ExecStatusRead(char ret[], const char *arg) {
   if (!strcmp(arg,    "power")) return (ReadPowerStatus(ret));
   if (!strcmp(arg,    "input")) return (ReadInputMode(ret));
   if (!strcmp(arg,     "lamp")) return (ReadLampHours(ret));
   if (!strcmp(arg,     "temp")) return (ReadTempSensors(ret));

   return (INVALID_ARGUMENT);
}

int
ExecPowerCommand(char ret[], const char *arg) {
   if (!strcmp(arg,       "on")) return (ProcessCommand(ret, POWER_ON));
   if (!strcmp(arg,      "off")) return (ProcessCommand(ret, POWER_OFF));

   return (INVALID_ARGUMENT);
}

int
ExecInputCommand(char ret[], const char *arg) {
   if (!strcmp(arg,    "video")) return (ProcessCommand(ret, INPUT_COMPOSIT));
   if (!strcmp(arg,  "s-video")) return (ProcessCommand(ret, INPUT_SVIDEO));
   if (!strcmp(arg,    "comp1")) return (ProcessCommand(ret, INPUT_COMPONENT_1));
   if (!strcmp(arg,    "comp2")) return (ProcessCommand(ret, INPUT_COMPONENT_2));
   if (!strcmp(arg,      "vga")) return (ProcessCommand(ret, INPUT_VGA));
   if (!strcmp(arg,     "hdmi")) return (ProcessCommand(ret, INPUT_HDMI));

   return (INVALID_ARGUMENT);
}

int
ExecScalerCommand(char ret[], const char *arg) {
   if (!strcmp(arg,      "off")) return (ProcessCommand(ret, SCALE_NORMAL_THROUGH));
   if (!strcmp(arg,   "normal")) return (ProcessCommand(ret, SCALE_NORMAL));
   if (!strcmp(arg,     "zoom")) return (ProcessCommand(ret, SCALE_ZOOM));
   if (!strcmp(arg,     "full")) return (ProcessCommand(ret, SCALE_FULL));
   if (!strcmp(arg,    "wide1")) return (ProcessCommand(ret, SCALE_WIDE_1));
   if (!strcmp(arg,    "wide2")) return (ProcessCommand(ret, SCALE_WIDE_2));
   if (!strcmp(arg,   "strech")) return (ProcessCommand(ret, SCALE_FULL_THROUGH));
   if (!strcmp(arg,  "caption")) return (ProcessCommand(ret, SCALE_CAPTION));

   return (INVALID_ARGUMENT);
}

int
ExecLampCommand(char ret[], const char *arg) {
   if (!strcmp(arg,   "normal")) return (ProcessCommand(ret, LAMP_NORMAL));
   if (!strcmp(arg,    "auto1")) return (ProcessCommand(ret, LAMP_AUTO_1));
   if (!strcmp(arg,    "auto2")) return (ProcessCommand(ret, LAMP_AUTO_2));
   if (!strcmp(arg,      "eco")) return (ProcessCommand(ret, LAMP_ECONOMY));

   return (INVALID_ARGUMENT);
}

int
ExecColorCommand(char ret[], const char *arg) {
   if (!strcmp(arg, "creative")) return (ProcessCommand(ret, COLOR_CREATIVE));
   if (!strcmp(arg,   "cinema")) return (ProcessCommand(ret, COLOR_CINEMA));
   if (!strcmp(arg,  "natural")) return (ProcessCommand(ret, COLOR_NATURAL));
   if (!strcmp(arg,   "living")) return (ProcessCommand(ret, COLOR_LIVING));
   if (!strcmp(arg,  "dynamic")) return (ProcessCommand(ret, COLOR_DYNAMIC));
   if (!strcmp(arg, "powerful")) return (ProcessCommand(ret, COLOR_POWERFUL));
   if (!strcmp(arg,    "vivid")) return (ProcessCommand(ret, COLOR_VIVID));
   if (!strcmp(arg,    "user1")) return (ProcessCommand(ret, COLOR_USER_1));
   if (!strcmp(arg,    "user2")) return (ProcessCommand(ret, COLOR_USER_2));
   if (!strcmp(arg,    "user3")) return (ProcessCommand(ret, COLOR_USER_3));
   if (!strcmp(arg,    "user4")) return (ProcessCommand(ret, COLOR_USER_4));

   return (INVALID_ARGUMENT);
}

int
ExecGenericCommand(char ret[], const char *arg) {
   char cmd[5];

   snprintf(cmd, 5, "%3s\r", arg);

   return (ProcessCommand(ret, cmd));
}
