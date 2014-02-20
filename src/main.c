#define _BSD_SOURCE // daemon()

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "server.h"
#include "serial.h"
#include "sanyo.h"
#include "onkyo.h"

static void
HelpUsage(void) {
   puts("");
   puts("sanyo projector control server " VERSION " <clemens@1541.org>");
   puts("");
   puts("USAGE: z4ctrl <command> <argument>");
   puts("");
   puts("POSSIBLE COMMANDS:");
   puts("");
   puts("\tC??    ... send generic 3 byte command to the projector");
   puts("\tstatus ... read current status from projector");
   puts("\tpower  ... power the projector on or off");
   puts("\tinput  ... select video source");
   puts("\tscaler ... set image scaler mode");
   puts("\tlamp   ... set lamp brightness");
   puts("\tcolor  ... set color mode");
   puts("\tmute   ... mute picture");
   puts("\tlogo   ... select startup logo");
   puts("\tmenu   ... switch OSD menu on or off");
   puts("\tpress  ... emulate menu navigation buttons");
   puts("\tmodel  ... read model number");
   puts("\tprobe  ... probe serial devices for connected devices and exit");
   puts("\tserver ... fork to background and keep running as network service");
   puts("");
   puts("RETURN CODES:");
   puts("");
   puts("\t0      ... ok");
   puts("\t1      ... unknown command");
   puts("\t2      ... invalid argument");
   puts("\t3      ... serial open failed");
   puts("\t4      ... serial write error");
   puts("\t5      ... serial read timeout");
   puts("\t6      ... no projector connected");
   puts("");

   exit(0);
}

static void
HelpStatusRead(void) {
   puts("possible status read arguments are:");
   puts("");
   puts("\tpower    ... return current power status");
   puts("\tinput    ... return selected video input");
   puts("\tlamp     ... return houres of lamp use");
   puts("\ttemp     ... return current temperaure sensor values");
   puts("");

   exit(0);
}

static void
HelpLampMode(void) {
   puts("possible lamp modes are:");
   puts("");
   puts("\tnormal   ... standard brightness");
   puts("\tauto1    ... adjusting brightness to input signal");
   puts("\tauto2    ... like auto1 but less bright");
   puts("\teco      ... lowest brightness and power consumption");
   puts("");

   exit(0);
}

static void
HelpPowerOnOff(void) {
   puts("possible power arguments are:");
   puts("");
   puts("\ton       ... switch projector on");
   puts("\toff      ... switch projector to stand-by");
   puts("\task      ... ask for confirmation before switching off");
   puts("");

   exit(0);
}

static void
HelpInputSource(void) {
   puts("possible input sources are:");
   puts("");
   puts("\tvideo    ... composit video");
   puts("\ts-video  ... super video");
   puts("\tcomp1    ... component video 1");
   puts("\tcomp2    ... component video 2");
   puts("\tvga      ... vga video");
   puts("\thdmi     ... digital hd video");
   puts("");

   exit(0);
}

static void
HelpScalerMode(void) {
   puts("possible image scaler modes are:");
   puts("");
   puts("\toff      ... scaler off");
   puts("\tnormal   ... scale up 4:3 to 19:9 by adding black borders");
   puts("\tzoom     ... scale up 4:3 to 19:9 by cutting edges");
   puts("\tfull     ... strech 4:3 to 19:9 full screen");
   puts("\tstrech   ... strech 4:3 to 16:9 unscaled");
   puts("\twide1    ... strech 4:3 to 19:9 but keep aspect ratio in the center");
   puts("\twide2    ... strech 16:9 with black borders to 16:9 without");
   puts("\tcaption  ... keep subtitles on the bottom visible");
   puts("");

   exit(0);
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

   exit(0);
}

static void
HelpOsdMenu(void) {
   puts("possible menu commands are:");
   puts("");
   puts("\ton       ... display OSD menu");
   puts("\toff      ... close OSD menu");
   puts("\tclear    ... unconditionally clear OSD");
   puts("");

   exit(0);
}

static void
HelpStartLogo(void) {
   puts("possible logo commands are:");
   puts("");
   puts("\toff      ... don't show any logo at startup");
   puts("\tdefault  ... use default startup logo");
   puts("\tuser     ... show captured logo at startup");
   puts("\tcapture  ... capture current image as startup logo");
   puts("");

   exit(0);
}

static void
HelpButtonPress(void) {
   puts("possible press commands are:");
   puts("");
   puts("\tright    ... move pointer of OSD menu to the right");
   puts("\tleft     ... move pointer of OSD to the left");
   puts("\tup       ... move up OSD pointer");
   puts("\tdown     ... move pointer down");
   puts("\tenter    ... select highlighted OSD item");
   puts("");

   exit(0);
}

static void
HelpVideoMute(void) {
   puts("possible mute commands are:");
   puts("");
   puts("\ton       ... black out the image");
   puts("\toff      ... restore image");
   puts("");

   exit(0);
}

static void
HelpOnkyoCommands(void) {
   puts("possible onkyo commands are:");
   puts("");
   puts("\tpower    ... switch receiver on or off");
   puts("\tmute     ... mute speaker");
   puts("");

   exit(0);
}

int
main(int argc, char **argv) {
   char ret[STRING_SIZE];
   int err = UNKNOWN_COMMAND;
   unsigned int dev_number = 32;
   char *dev_node[32];

   if ((argc == 1) || ((argc == 2) && (!strcmp(argv[1], "help")))) {
      HelpUsage();
   }

   if ((argc > 2) && (!strcmp(argv[1], "help"))) {
      if (!strcmp(argv[2], "status")) HelpStatusRead();
      if (!strcmp(argv[2],  "power")) HelpPowerOnOff();
      if (!strcmp(argv[2],  "input")) HelpInputSource();
      if (!strcmp(argv[2], "scaler")) HelpScalerMode();
      if (!strcmp(argv[2],   "lamp")) HelpLampMode();
      if (!strcmp(argv[2],  "color")) HelpColorMode();
      if (!strcmp(argv[2],   "mute")) HelpVideoMute();
      if (!strcmp(argv[2],   "logo")) HelpStartLogo();
      if (!strcmp(argv[2],   "menu")) HelpOsdMenu();
      if (!strcmp(argv[2],  "press")) HelpButtonPress();
   }

   SerialListDevices(dev_node, &dev_number);

   for (int i=0; i<dev_number; i++) {
      if (!SanyoProbeDevice(dev_node[i])) break;
   }

   for (int i=0; i<dev_number; i++) {
      if (!OnkyoProbeDevice(dev_node[i])) break;
   }

   if (sanyo_serial) {
      printf("found Sanyo projector on %s\n", sanyo_serial->device);
   }

   if (onkyo_serial) {
      printf("found Onkyo receiver on %s\n", onkyo_serial->device);
   }

   if (!strcmp(argv[1], "probe")) {
      exit(err);
   }

   if (!strcmp(argv[1], "server")) {
      puts("starting network service ...");

      if (daemon(0, 0)) {
         puts("couldn't fork, keep running in foreground.");
      }

      ServerNetworkStart();
 
      exit(0);
   }

   if (argv[1][0] == 'C') {
      err = ExecGenericCommand(ret, argv[1]);
   }

   if (!strcmp(argv[1], "model")) {
      err = ReadModelNumber(ret);
   }

   if (!strcmp(argv[1], "status")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpStatusRead();
      } else {
         err = ExecStatusRead(ret, argv[2]);
      }
   }

   if (!strcmp(argv[1], "power")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpPowerOnOff();
      } else {
         err = ExecPowerCommand(ret, argv[2]);
      }
   }

   if (!strcmp(argv[1], "input")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpInputSource();
      } else {
         err = ExecInputCommand(ret, argv[2]);
      }
   }

   if (!strcmp(argv[1], "scaler")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpScalerMode();
      } else {
         err = ExecScalerCommand(ret, argv[2]);
      }   
   }

   if (!strcmp(argv[1], "lamp")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpLampMode();
      } else {
         err = ExecLampCommand(ret, argv[2]);
      }   
   }

   if (!strcmp(argv[1], "color")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpColorMode();
      } else {
         err = ExecColorCommand(ret, argv[2]);
      }   
   }

   if (!strcmp(argv[1], "mute")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpVideoMute();
      } else {
         err = ExecMuteCommand(ret, argv[2]);
      }   
   }

   if (!strcmp(argv[1], "logo")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpStartLogo();
      } else {
         err = ExecLogoCommand(ret, argv[2]);
      }   
   }

   if (!strcmp(argv[1], "menu")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpOsdMenu();
      } else {
         err = ExecMenuCommand(ret, argv[2]);
      }   
   }

   if (!strcmp(argv[1], "press")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpButtonPress();
      } else {
         err = ExecPressCommand(ret, argv[2]);
      }   
   }

   if (!strcmp(argv[1], "onkyo")) {
      if ((argc<3) || (!strcmp(argv[2], "help"))) {
         HelpOnkyoCommands();
      } else {
         err = OnkyoExecCommand(ret, argv[2]);
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

      case NOT_CONNECTED:
         printf("device not connected!\n");
      break;

      default:
         puts(ret);
      break;
   }

   return (err);
}
