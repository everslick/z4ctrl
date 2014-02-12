#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ctrl.h"

static void
HelpUsage(void) {
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
   puts("\t2 ... invalid argument");
   puts("\t3 ... serial open failed");
   puts("\t4 ... serial write error");
   puts("\t5 ... serial read timeout");
   puts("");
}

static void
HelpStatusRead(void) {
   puts("possible status read arguments are:");
   puts("");
   puts("\tpower ... return current power status");
   puts("\tinput ... return selected input");
   puts("\tlamp  ... return houres of lamp use");
   puts("\ttemp  ... return current temperaure sensor values");
   puts("");
}

static void
HelpLampModes(void) {
   puts("possible lamp modes are:");
   puts("");
   puts("\tnormal ... standard brightness");
   puts("\tauto1  ... adjusting brightness to input signal");
   puts("\tauto2  ... like auto1 but less bright");
   puts("\teco    ... lowest brightness and power consumption");
   puts("");
}

static void
HelpPowerOnOff(void) {
   puts("possible power arguments are:");
   puts("");
   puts("\ton    ... switch projector on");
   puts("\toff   ... switch projector to stand-by");
   puts("");
}

static void
HelpInputSource(void) {
   puts("possible input sources are:");
   puts("");
   puts("\tvideo   ... composit video");
   puts("\ts-video ... super video");
   puts("\tcomp1   ... component video 1");
   puts("\tcomp2   ... component video 2");
   puts("\tvga     ... vga video");
   puts("\thdmi    ... digital hd video");
   puts("");
}

static void
HelpScalerMode(void) {
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
}

static void
HelpColorMode(void) {
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
}

int
main(int argc, char **argv) {
   int err = UNKNOWN_COMMAND;
   char ret[64];

   if ((argc==1) || ((argc>1) && (!strcmp(argv[1], "help")))) {
      HelpUsage();
      exit(0);
   }

   if ((argc==2) && (argv[1][0]=='C')) {
      err = ExecGenericCommand(ret, argv[1]);
   }

   if (!strcmp(argv[1], "status")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpStatusRead();
         exit(0);
      } else {
         err = ExecStatusRead(ret, argv[2]);
      }
   }

   if (!strcmp(argv[1], "power")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpPowerOnOff();
         exit(0);
      } else {
         err = ExecPowerCommand(ret, argv[2]);
      }
   }

   if (!strcmp(argv[1], "input")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpInputSource();
         exit(0);
      } else {
         err = ExecInputCommand(ret, argv[2]);
      }
   }

   if (!strcmp(argv[1], "scaler")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpScalerMode();
         exit(0);
      } else {
         err = ExecScalerCommand(ret, argv[2]);
      }   
   }

   if (!strcmp(argv[1], "lamp")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpLampModes();
         exit(0);
      } else {
         err = ExecLampCommand(ret, argv[2]);
      }   
   }

   if (!strcmp(argv[1], "color")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpColorMode();
         exit(0);
      } else {
         err = ExecColorCommand(ret, argv[2]);
      }   
   }

   switch (err) {
      case UNKNOWN_COMMAND:
         printf("unknown command!\n");
      break;

      case INVALID_ARGUMENT:
         printf("invalid argument!\n");
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
         puts(ret);
         //printf("response: %s\n", response);
      break;
   }

   return (err);
}
