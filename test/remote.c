#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

static void
PrintUsage(void) {
   puts("");
   puts("z4remote " VERSION " <clemens@1541.org>");
   puts("");
   puts("USAGE: z4remote <cmd> [arg]");
   puts("");
   puts("\tcmd ... command");
   puts("\targ ... argument");
   puts("");

   exit(0);
}

static int
OpenUdpSocket(const char *host, unsigned short int port) {
   char addrstr[INET_ADDRSTRLEN];
   struct hostent *hent = NULL;
   struct sockaddr_in addr;
   int fd = -1, flg = 1;

   if (!port) {
      goto error;
   }

   if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
      goto error;
   }

   memset(&addr, 0, sizeof (addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons((int)port);

   if (!host) {
      setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &flg, sizeof (flg));
      addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
   } else {
      if (!(hent = gethostbyname(host))) {
         goto error;
      }
      if (!inet_ntop(AF_INET, hent->h_addr, addrstr, sizeof (addrstr))) {
         goto error;
      }
      if (!inet_pton(AF_INET, addrstr, &addr.sin_addr)) {
         goto error;
      }
   }

   if (connect(fd, (struct sockaddr *)&addr, sizeof (addr))) {
      goto error;
   }

   return (fd);

error:

   if (fd >= 0) {
      close(fd);
   }

   return (-1);
}

int
main(int argc, char **argv) {
   const char *cmd = "";
   const char *arg = "";
   unsigned int size;
   char buffer[64];
   int skt = NULL;

   for (int i=1; i<argc; i++) {
      if (!strcmp(argv[i], "--help")) {
         PrintUsage();
      }
   }

   if ((argc < 2) || (argc > 3)) {
      PrintUsage();
   }
   if (argc == 2) {
      cmd = argv[argc - 1];
   }
   if (argc == 3) {
      cmd = argv[argc - 2];
      arg = argv[argc - 1];
   }

   if ((skt = OpenUdpSocket(NULL, 1541)) >= 0) {
      size = snprintf(buffer, 64, "%s %s", cmd, arg);

      if (send(skt, buffer, size, 0) < 0) {
         printf("error while sending data to server\n");
      }

      close(skt);
   } else {
      puts("could not connect to server, exiting.");
      exit(-1);
   }

   return (0);
}
