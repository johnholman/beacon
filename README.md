* Beacon - firmware for an IR emitting beacon that supports distance estimation by the Vishay TSSP58P38 IR proximity sensor
 * 
 * A beacon command is communicated as the payload of an MQTT message on topic beacon/<n> where n is the decimal value of the last octet of
 * the IP address for the beach. The payload must be an integer in [0, 9]. 0 means switch off the beacon. Otherwise a repeating sequence
 * of bursts of carrier at 38 Hz are generated on the output GPIO. The sequence starts with a 180ms burst to allow the IR receiver to
 * irradiance and therefore distance. c-1 short bursts (each 10 ms off 10 ms on) follow, where c is the command number. It finishes with a 
 * recovery period of 500 ms during which the beacon is not lit.
 * 
 * One state machine running the "burst" PIO program is responsible for processing commands sent via its RX FIFO and generating the appropriate
 * burst sequence. This burst SM checks the RX FIFO for a new command at the beginning of each burst sequence. It signals to the second state machine
 * responsible for generating the 38Hz carrier by setting a GPIO, the burst GPIO. This is not ideal but it seems impossible to use interrupt flags
 * as you cannot read them only set them or wait upon them. 
 * 
 * The second state machine runs the "carrier" PIO program. It outputs a 38 kHz square wave on the output pin whenever the burst pin is set to 1
 *  
 * The burst SM clock is slowed to 3200 Hz, so each PIO cycle lasts 5/16 ms and delay is implemented by looping round
 * nop [31] instructions each taking 5/16 * 32 ms = 10 ms. Exact timing isn't required so time take by instructions other
 * than nop [31] are ignored here.
 * 
 * The carrier SM clock is set so that a single period of the 38 kHz carrier corresponds to 32 system clock cycles.
 