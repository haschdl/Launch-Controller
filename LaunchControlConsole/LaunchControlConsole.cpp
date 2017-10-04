// LaunchControlConsole.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../LaunchControl/LaunchControl.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <stdlib.h>
#include <crtdbg.h>
#include <algorithm>
#include <string>
#include <iostream>


LaunchControl* launchControl;

int main()
{
	try {

		launchControl = new LaunchControl(true);


		launchControl->init();


		//Setting template to factory 1
		//Template is 00h-07h (0-7) for the 8 user templates, and 08h-0Fh (8-15) for the 8 factory templates
		auto templateNumber = 0x08;

		std::cout << "Reseting LaunchControl...\n";
		launchControl->resetLaunchControl(templateNumber);

		std::cout << "Sending message to set the template.\n";
		launchControl->setTemplate(templateNumber);


		int i = 0;
		// i being a multiple of number of pads makes complete rounds
		// 72 will make each pad flash once with each of 8 colors
		while (i < 16)
		{
			std::vector<unsigned char> message;
			std::cout << "Sending control message...\n";
			launchControl->setPadColor(i % 8,
				//this makes the PAD be either Red(7) or off (0)
				LaunchControl::ColorBrightness[(i / 8) % 8]// *((i / 8) % 2)]
				);
			if(launchControl->PAD_1.On())
			{
				std::cout << "PAD 1 is ON";
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			++i;
		}

		std::cout << "\nReading MIDI input ... press <enter> to quit.\n";

		char input;
		std::cin.get(input);
		std::cin.get(input);
		//std::this_thread::sleep_for(std::chrono::milliseconds(100));





		//Resetting controller (turning off leds...)
		launchControl->resetLaunchControl(templateNumber);


		goto cleanup;

	}
	catch (RtMidiError &error) {
		error.printMessage();
		char input;
		std::cin.get(input);

	}
	catch (const std::exception & ex) {
		std::cerr << ex.what() << std::endl;
		char input;
		std::cin.get(input);

	}

cleanup:

	delete launchControl;
	_CrtDumpMemoryLeaks();
	return 0;
}

