Beacon - firmware for an IR emitting beacon that supports distance estimation by the Vishay TSSP58P38 IR proximity sensor
 
 A beacon command is communicated as the payload of an MQTT message on topic beacon/<n> where n is the decimal value of the last octet of
 the IP address for the beach and the payload, a single character string, is the command.
 
 The following commands are supported:

 i<id> - set public id to <id>. The beacon subscribes to topic <id> where is is a (shortish) string

 c - continuous mode on (default). The beacon transmits a continuous series of flashes

 n - continuous mode off.  The beacon transmits only in response to the f command

 f - send a single flash. This is ignored if the beacon is in continuous mode.

 d<n> - set the data to be sent - i.e. the number short flashes in a burst. Default is 0.

Each transmission ("flash") consists of a sequence of bursts of 38 kHz carrier frequency output on the output GPIO. The sequence starts with a series of short bursts, each of duration about 3 ms with 5 ms inter-burst gap, that communicate the data (a number between and 9). The last short burst is followed by a recovery period of about 100 ms. A 180ms burst follows to allow the TSP58P38 IR sensor to estimate irradiance and therefore distance. The sequence finishes with a 
 recovery period of 500 ms.
  
One state machine runs the "burst" PIO program responsible for processing commands sent from the main loop via its TX FIFO and generating the appropriate
burst sequence. It communicates with a second state machine responsible for generating the 38Hz carrier by setting a GPIO, the burst GPIO. This is not ideal but it seems impossible to use interrupt flags as you cannot read them only set them or wait upon them. 
  
The second state machine runs the "carrier" PIO program. It outputs a 38 kHz square wave on the output pin whenever the burst pin is set to 1
   
The burst SM clock is slowed to 3200 Hz, so each PIO cycle lasts 5/16 ms and delay is implemented by looping round
nop [31] instructions each taking 5/16 * 32 ms = 10 ms. Exact timing isn't required so time taken by instructions other
than nop [31] are ignored here.
  
The carrier SM clock is set so that a single period of the 38 kHz carrier corresponds to 32 system clock cycles.
 