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
 * Channel scanner
 *
 * Example to detect interference on the various channels available.
 * This is a good diagnostic tool to check whether you're picking a
 * good channel for your application.
 *
 * Inspired by cpixip.
 * See http://arduino.cc/forum/index.php/topic,54795.0.html
 */

#include <cstdlib>
#include <iostream>
#include <librf24-bcm.h>

using namespace std;

//
// Hardware configuration
//

// CE Pin, CSN Pin, SPI Speed

// Setup for GPIO 22 CE and GPIO 25 CSN with SPI Speed @ 1Mhz
//RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_18, BCM2835_SPI_SPEED_1MHZ);

// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 4Mhz
//RF24 radio(RPI_V2_GPIO_P1_15, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_4MHZ);

// Setup for GPIO 22 CE and CE1 CSN with SPI Speed @ 8Mhz
RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_26, BCM2835_SPI_SPEED_8MHZ);


//
// Channel info
//
//const uint8_t num_channels = 128;
const uint8_t num_channels = 120;
uint8_t values[num_channels];


const int num_reps = 100;
int reset_array=0;


int main(int argc, char** argv)
{
  //
  // Print preamble
  //

  //Serial.begin(57600);
  //printf_begin();
  printf("RF24/examples/scanner/\n");

  //
  // Setup and configure rf radio
  //
  radio.begin();

  printf("Started radio ...\n");

  radio.setAutoAck(false);

  printf("Disabled auto ACK ...\n");

  // Get into standby mode
  radio.startListening();
  radio.stopListening();

  radio.printDetails();

  // Print out header, high then low digit
  int i = 0;

  while ( i < num_channels )
  {
    printf("%x",i>>4);
    ++i;
  }
  printf("\n");

  i = 0;
  while ( i < num_channels )
  {
    printf("%x",i&0xf);
    ++i;
  }
  printf("\n");

	// forever loop
  while(1)
	{
		if ( reset_array == 1 )
		{
			// Clear measurement values
			memset(values,0,sizeof(values));
			printf("\n");
		}

		// Scan all channels num_reps times
		int i = num_channels;
		while (i--)
		{
			// Select this channel
			radio.setChannel(i);

			// Listen for a little
			radio.startListening();
			delayMicroseconds(128);
			radio.stopListening();

			// Did we get a carrier?
			if ( radio.testCarrier() )
					++values[i];
			if ( values[i] == 0xf )
			{
				reset_array = 2;
			}
		}

		// Print out channel measurements, clamped to a single hex digit
		i = 0;
		while ( i < num_channels )
		{
			printf("%x",min(0xf,(values[i]&0xf)));
			++i;
		}

		printf("\n");
	}

  return 0;
}

// vim:ai:cin:sts=2 sw=2 ft=cpp
