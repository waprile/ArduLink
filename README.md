ArduLink
========

A library (for now just a chunk of code) to send and receive data over an optical link. It uses Manchester Differential Encoding and one interrupt for receiving. Sending uses no interrupts but only delay.

The code has been tested on Arduino UNO and ATTiny45 microcontrollers. On the ATTiny, you have to undefine the RECEIVE symbol because the code gets too big, and interrupts are also wrong. 

Possible future work: 
 other types of line code
	UWB like
	OOK with start and stop bits

Necessary future work:
	conversion to library
	packet sending with ack from the other side
		(this is the way one ends up rewriting TCP/IP, but buggier)

