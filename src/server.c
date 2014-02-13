#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

#include "ctrl.h"
#include "snl.h"

static int shutdown = 0;

static char *
ipaddr(unsigned int host, unsigned short port) {
   unsigned char *ip = (unsigned char *)&host;
   static char buf[32];

   sprintf(buf, "%i.%i.%i.%i:%i", ip[3], ip[2], ip[1], ip[0], port);

   return (buf);
}

static void
quit(int sig) {
   if (!shutdown) {
      shutdown = 1;
   }
}

static void
event_callback(snl_socket_t *skt) {
   char *data, cmd[32], arg[32], ret[STRING_SIZE];
   const char *info, *err;

   switch (skt->event_code) {
      case SNL_EVENT_ERROR:
         err = snl_error_string(skt->error_code);

         printf("client error: %i (%s)\n", skt->error_code, err);
      break;

      case SNL_EVENT_RECEIVE:
         info = ipaddr(skt->client_ip, skt->client_port);
         data = (char *)skt->data_buffer;

         // FIXME potential buffer overflow
         sscanf(data, "%s %s", cmd, arg);

         printf("exec z4ctrl %s %s\n", cmd, arg);

         if (!strcmp(cmd,  "power")) ExecPowerCommand(ret, arg);
         if (!strcmp(cmd,  "input")) ExecInputCommand(ret, arg);
         if (!strcmp(cmd, "scaler")) ExecScalerCommand(ret, arg);
         if (!strcmp(cmd,   "lamp")) ExecLampCommand(ret, arg);
         if (!strcmp(cmd,  "color")) ExecColorCommand(ret, arg);
         if (!strcmp(cmd, "status")) ExecStatusRead(ret, arg);

			printf("response: %s\n", ret);
      break;
   }
}

int
ServerNetworkStart(void) {
   snl_socket_t *server = NULL;

   snl_init();

   signal(SIGINT,  quit);
   signal(SIGQUIT, quit);
   signal(SIGHUP,  quit);

   printf("starting UDP listener on port 1541.\n");

   server = snl_socket_new(SNL_PROTO_UDP, event_callback, NULL);

   if (snl_listen(server, 1541)) {
      printf("could not start listener, exiting.\n");

      return (1);
   }

   while (!shutdown) {
      sleep(1);
   }

   snl_disconnect(server);

   snl_socket_delete(server);

   return (0);
}
