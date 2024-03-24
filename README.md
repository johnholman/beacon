Beacon - firmware for an IR emitting beacon that supports distance estimation by the Vishay TSSP58P38 IR proximity sensor

Version 0.7, 24 Mar 24
 
 At startup, the beacon connects to the broker and publishes with topic beacon/announce and payload its flash memory serial number.

 A beacon command is communicated as the payload of an MQTT message on topic beacon/\<s> where s is either the beacon's flash
 memory serial number or the "public id" given as the parameter of a previous "i" command.
 (For previous firmware s was the decimal value of the last octet of the IP address).
 The payload is the command, a single character, optionally followed by a parameter.
 
 The following commands are supported:

 i\<id> - set public id to \<id>. The beacon subscribes to topic <id> where is is a (shortish) string

 n - continuous mode off.  The beacon transmits only in response to the f command (default)

 c - continuous mode on. The beacon transmits a continuous series of flashes

 f\<n> - send a single flash accompanied by data \<n>, where n is the number of short bursts in the flash

 d\<n> - set the data to be sent when in continuous mode  Default is 0. Overridden by any subsequent f
 command.

Each transmission ("flash") consists of a sequence of bursts of 38 kHz carrier frequency output on the output GPIO. The sequence starts with a series of short bursts, each of duration about 3 ms with 5 ms inter-burst gap whose count constitutes the data associated with the flash. The last short burst is followed by a recovery period of about 100 ms. A 180ms burst follows to allow the TSP58P38 IR sensor to estimate irradiance and therefore distance. The sequence finishes with a recovery period of 500 ms.
  
One state machine runs the "burst" PIO program responsible for processing commands sent from the main loop via its TX FIFO and generating the appropriate burst sequence. It communicates with a second state machine responsible for generating the 38Hz carrier by setting a GPIO, the burst pin. This is not ideal but it seems impossible to use interrupt flags as you cannot read them only set them or wait upon them. 
  
The second state machine runs the "carrier" PIO program. It outputs a 38 kHz square wave on the output pin whenever the burst pin is set to 1
   
The burst state machine's clock is slowed to 3200 Hz, so each PIO cycle lasts 5/16 ms and delay is implemented by looping round
nop [31] instructions each taking 5/16 * 32 ms = 10 ms. Exact timing isn't required so time taken by instructions other
than nop [31] are ignored here.
  
The carrier state machine's clock is set so that a single period of the 38 kHz carrier corresponds to 32 system clock cycles.
 