z4ctrl
======

A command line tool to control Sanyo projectors via their serial port.

USAGE: z4ctrl *command* *argument*

POSSIBLE COMMANDS:

	C??     ... send generic 3 byte command to the projector
	status  ... read power status, video input, lamp usage or temperature sensors
	power   ... power the projector on or off
	input   ... select video source
	scaler  ... set image scaler mode
	lamp    ... set lamp mode
	color   ... set color mode

RETURN CODES:

	0 ... ok
	1 ... unknown command
	2 ... serial open failed
	3 ... serial write error
	4 ... serial read timeout

Omitting *argument* will print a list of possible arguments to the given
*command*. Keep in mind that there is no command line option for the used
serial port. To specify a different one, you have to change the #define
SERIAL_DEVICE "..." in z4ctrl.c and recompile.
