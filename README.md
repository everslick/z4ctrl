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
	model   ... read model number
	probe   ... probe serial devices for connected projector and exit
	server  ... start in background and keep running as a network service


RETURN CODES:

	0 ... ok
	1 ... unknown command
	2 ... invalid argument
	3 ... serial open failed
	4 ... serial write error
	5 ... serial read timeout
	6 ... projector not connected


Setting *argument* to *help* or omitting it will print a list of possible
arguments to the given *command*. When started in *server* mode z4ctrl will
fork to the background an wait for UDP packets on port 1541 in the form
"command argument" (without quotes).
