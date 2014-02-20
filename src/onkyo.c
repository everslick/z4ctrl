#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "command.h"
#include "serial.h"
#include "onkyo.h"

Serial *onkyo_serial = NULL;

static int
ProcessCommand(char ret[], const char *cmd) {
   unsigned int len = 1;
   int err = 0;

   if (!onkyo_serial) return (0);

   if (SerialSendBuffer(onkyo_serial, cmd, strlen(cmd))) {
      SerialClose(onkyo_serial);
      onkyo_serial = NULL;
      return (WRITE_ERROR);
   }

   memset(ret, 0, STRING_SIZE);

   for (int i=0; i<STRING_SIZE; i++) {
      if (SerialReceiveBuffer(onkyo_serial, &ret[i], &len, 2000)) {
         // timeout, don't try to read more bytes
         err = READ_TIMEOUT; break;
      }

      if (ret[i] == '?') {
         // unknown command, set error code
         err = UNKNOWN_COMMAND;
      }

      if (ret[i] == '\r') {
         // received CR, replace by terminating \0
         ret[i] = '\0';
      }

      if (ret[i] == '\n') {
         // received NL, replace by (one more) terminating \0
         ret[i] = '\0';
         // ret complete, end loop
         break;
      }
   }

   return (err);
}

int
OnkyoProbeDevice(const char *device) {
   char ret[STRING_SIZE];

   if ((onkyo_serial = SerialOpen(device)) == NULL) {
      return (OPEN_FAILED);
   }

   if (SerialInit(onkyo_serial, 19200, "8N1", 0)) {
      SerialClose(onkyo_serial);   
      onkyo_serial = NULL;
      return (OPEN_FAILED);
   }

   // wait for arduino bootloader to start sketch
   sleep(1);

   if (OnkyoReadStatus(ret)) {
      SerialClose(onkyo_serial);   
      onkyo_serial = NULL;
      return (NOT_CONNECTED);
   }

   return (0);
}

int
OnkyoReadStatus(char ret[]) {
   int err = ProcessCommand(ret, "status\n");

   return (err);
}

int
OnkyoExecCommand(char ret[], const char *arg) {
   if (!strcmp(arg,   "status")) return (OnkyoReadStatus(ret));

   if (!strcmp(arg,    "power")) return (ProcessCommand(ret,   "power\n"));
   if (!strcmp(arg,     "vol+")) return (ProcessCommand(ret,    "vol+\n"));
   if (!strcmp(arg,     "vol-")) return (ProcessCommand(ret,    "vol-\n"));
   if (!strcmp(arg,     "mute")) return (ProcessCommand(ret,    "mute\n"));
   if (!strcmp(arg,  "speaker")) return (ProcessCommand(ret, "speaker\n"));
   if (!strcmp(arg,    "movie")) return (ProcessCommand(ret,   "movie\n"));
   if (!strcmp(arg,     "game")) return (ProcessCommand(ret,    "game\n"));
   if (!strcmp(arg,    "music")) return (ProcessCommand(ret,   "music\n"));
   if (!strcmp(arg,   "stereo")) return (ProcessCommand(ret,  "stereo\n"));

   return (INVALID_ARGUMENT);
}
