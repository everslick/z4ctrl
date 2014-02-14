#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#include "ctrl.h"
#include "snl.h"

static int shutdown = 0;

static void
quit(int sig) {
   if (!shutdown) {
      shutdown = 1;
   }
}

static void
event_callback(snl_socket_t *skt) {
   char *data, cmd[32], arg[32], ret[STRING_SIZE];
   int err = UNKNOWN_COMMAND;

   if (skt->event_code == SNL_EVENT_RECEIVE) {
      data = (char *)skt->data_buffer;
      data[skt->data_length] = '\0';

      // FIXME potential buffer overflow
      sscanf(data, "%s %s", cmd, arg);

      syslog(LOG_DEBUG, "received: %s %s", cmd, arg);

      if (!strcmp(cmd,  "power")) err = ExecPowerCommand(ret, arg);
      if (!strcmp(cmd,  "input")) err = ExecInputCommand(ret, arg);
      if (!strcmp(cmd, "scaler")) err = ExecScalerCommand(ret, arg);
      if (!strcmp(cmd,   "lamp")) err = ExecLampCommand(ret, arg);
      if (!strcmp(cmd,  "color")) err = ExecColorCommand(ret, arg);
      if (!strcmp(cmd, "status")) err = ExecStatusRead(ret, arg);
      if (!strcmp(cmd,  "model")) err = ReadModelNumber(ret);

      switch (err) {
         case UNKNOWN_COMMAND:
            syslog(LOG_ERR, "unknown command");
         break;

         case INVALID_ARGUMENT:
            syslog(LOG_ERR, "invalid argument");
         break;

         case WRITE_ERROR:
            syslog(LOG_ERR, "serial write error");
         break;

         case READ_TIMEOUT:
            syslog(LOG_ERR, "serial read timeout");
         break;

         case OPEN_FAILED:
            syslog(LOG_ERR, "serial device open failed");
         break;

         default:
            syslog(LOG_DEBUG, "response: %s", ret);
         break;
      }
   }
}

int
ServerNetworkStart(void) {
   snl_socket_t *server = NULL;

   snl_init();

   openlog(NULL, LOG_PID|LOG_NDELAY, LOG_USER);

   signal(SIGINT,  quit);
   signal(SIGQUIT, quit);
   signal(SIGHUP,  quit);

   server = snl_socket_new(SNL_PROTO_UDP, event_callback, NULL);

   if (snl_listen(server, 1541)) {
      syslog(LOG_ERR, "failed to start server");

      goto cleanup;
   }

   syslog(LOG_INFO, "UDP server started on port 1541");

   while (!shutdown) {
      sleep(1);
   }

cleanup:

   snl_disconnect(server);
   snl_socket_delete(server);

   syslog(LOG_INFO, "terminating");
   closelog();

   return (0);
}
