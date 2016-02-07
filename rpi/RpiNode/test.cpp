/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

 03/17/2013 : Charles-Henri Hallard (http://hallard.me)
              Modified to use with Arduipi board http://hallard.me/arduipi
						  Changed to use modified bcm2835 and RF24 library
 */

/**
 * Example RF Radio Ping Pair
 *
 * This is an example of how to use the RF24 class.  Write this sketch to two different nodes,
 * connect the role_pin to ground on one.  The ping node sends the current time to the pong node,
 * which responds by sending the value back.  The ping node can then see how long the whole cycle
 * took.
 */

#include <cstdlib>
#include <iostream>
#include "./RF24.h"

//
// Hardware configuration
//

// CE Pin, CSN Pin, SPI Speed

// Setup for GPIO 22 CE and GPIO 25 CSN with SPI Speed @ 1Mhz
//RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_18, BCM2835_SPI_SPEED_1MHZ);

// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 4Mhz
//RF24 radio(RPI_V2_GPIO_P1_15, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_4MHZ);

// Setup for GPIO 22 CE and CE1 CSN with SPI Speed @ 8Mhz
// CE -> CS0
// CSN -> GPIO 25	(pin 22)
const uint8_t PinCE = RPI_V2_GPIO_P1_24;
const uint8_t PinCSN = RPI_V2_GPIO_P1_22;

RF24 radio(PinCE, PinCSN, BCM2835_SPI_SPEED_8MHZ);

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

int main(int argc, char** argv) {
	printf("RF24/examples/pingtest/\n");

	//
	// Setup and configure rf radio
	//
	radio.begin();
	radio.setRetries(15,15);
	radio.setChannel(0x4c);
	radio.setPALevel(RF24_PA_LOW);
	radio.openWritingPipe(pipes[0]);
	radio.openReadingPipe(1,pipes[1]);
	radio.startListening();
	radio.printDetails();
	//
	// Ping out role.  Repeatedly send the current time
	//

	// forever loop
	while (true) {
		// First, stop listening so we can talk.
		radio.stopListening();

		// Take the time, and send it.  This will block until complete
		unsigned long time = millis();
		printf("Now sending %lu...",time);
		bool ok = radio.write( &time, sizeof(unsigned long) );

		if (ok)
			printf("ok...");
		else
			printf("failed.\n");

		// Now, continue listening
		radio.startListening();

		// Wait here until we get a response, or timeout (250ms)
		unsigned long started_waiting_at = millis();
		bool timeout = false;
		while ( ! radio.available() && ! timeout ) {
				// by bcatalin » Thu Feb 14, 2013 11:26 am
				delay(5); //add a small delay to let radio.available to check payload
			if (millis() - started_waiting_at > 200 )
				timeout = true;
		}


		// Describe the results
		if ( timeout )
		{
			printf("Failed, response timed out.\n");
		}
		else
		{
			// Grab the response, compare, and send to debugging spew
			unsigned long got_time;
			radio.read( &got_time, sizeof(unsigned long) );

			// Spew it
			printf("Got response %lu, round-trip delay: %lu\n",got_time,millis()-got_time);
		}

		// Try again 1s later
		//    delay(1000);
		delay(1000);
	} // forever loop

  return 0;
}

// vim:cin:ai:sts=2 sw=2 ft=cpp
