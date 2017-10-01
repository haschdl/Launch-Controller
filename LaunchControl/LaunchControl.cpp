#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "stdafx.h"
#include "LaunchControl.h"
#include <algorithm>
#include <string>
#include <iostream>


const std::string LaunchControl::DEVICE_NAME = "Launch Control";
const std::string LaunchControl::ERROR_DEVICE_NOT_FOUND = "It was not possible to locate a MIDI controller containing the name '" + LaunchControl::DEVICE_NAME + "'.\nPlease make sure LaunchControl is connected.";

//This is a pointer which allows using the midiInCallBack not as a static object.
void* ptMidiCallback;
//Wrapper to the callback.
void midiInCallbackWrapper(double deltatime, std::vector< unsigned char > *message, void *);


//A list of all possible color brightness. It can be used to iterate over possible colors
//or randomly pick one value for animation effects.
LaunchControl::ColorBrightnessEnum LaunchControl::ColorBrightness[] = { Off,GreenLow,GreenFull,AmberLow,AmberFull,YellowFull,RedLow,RedFull };


/*
Constructor.
Note: the values below correspond to Factory Template number 1.
If you are not sure which template is currently set, hold the Factory button.
*/
LaunchControl::LaunchControl(bool throwIfNotFound, bool toggleMode, LaunchControl::LogMode logMode) : midiin(nullptr), midiout(nullptr),
SYSEX_ID{ 0x00, 0x20, 0x29 },
KNOB_1_LOW{ 184, 41, 0 },
KNOB_2_LOW{ 184, 42, 0 },
KNOB_3_LOW{ 184, 43, 0 },
KNOB_4_LOW{ 184, 44, 0 },
KNOB_5_LOW{ 184, 45, 0 },
KNOB_6_LOW{ 184, 46, 0 },
KNOB_7_LOW{ 184, 47, 0 },
KNOB_8_LOW{ 184, 48, 0 },
KNOB_1_UPPER{ 184, 21, 0 },
KNOB_2_UPPER{ 184, 22, 0 },
KNOB_3_UPPER{ 184, 23, 0 },
KNOB_4_UPPER{ 184, 24, 0 },
KNOB_5_UPPER{ 184, 25, 0 },
KNOB_6_UPPER{ 184, 26, 0 },
KNOB_7_UPPER{ 184, 27, 0 },
KNOB_8_UPPER{ 184, 28, 0 },
PAD_1{ 152,  9, 127 },
PAD_2{ 152, 10, 127 },
PAD_3{ 152, 11, 127 },
PAD_4{ 152, 12, 127 },
PAD_5{ 152, 25, 127 },
PAD_6{ 152, 26, 127 },
PAD_7{ 152, 27, 127 },
PAD_8{ 152, 28, 127 }
{
	//Setting up the controls values
	//TODO The knobs might have a different value at start up,
	//therefore the actual values can only be known after the knob is modified
	//For this reason is recommend to assume that the knobs are always at 0 
	//during start up.

	ptMidiCallback = this;
	forceToggleMode = toggleMode;

	try {
		// RtMidiIn constructor
		midiin = new RtMidiIn();
		midiout = new RtMidiOut();

		openLaunchControlMidiPorts(midiin, midiout);

		// Set our callback function.  This should be done immediately after
		// opening the port to avoid having incoming messages written to the
		// queue instead of sent to the callback function.
		midiin->setCallback(&midiInCallbackWrapper, 0);

		// Don't ignore sysex, timing, or active sensing messages.
		midiin->ignoreTypes(false, false, false);
	}
	catch (RtMidiError)
	{
		if (throwIfNotFound)
			throw;
	}
}



LaunchControl::~LaunchControl()
{

	delete midiin;
	delete midiout;
	_CrtDumpMemoryLeaks();

}

void LaunchControl::midiInCallback(double deltatime, std::vector< unsigned char > *message, void *)
{
#if DEBUG
	printMessage(deltatime, *message);
#endif


	int nBytes = message->size();
	if (nBytes <= 3) {
		LaunchControl::Controls launchPadControl = messageToControl(*message);


		int launchPadControlValue = (int)message->at(2);


		if (forceToggleMode)
		{
			//lighting up LED according to last value
			std::string controlName = getControlName(*message);
			auto controlValue = controlValues(launchPadControl);
			if (controlValue != nullptr) {
				if (controlValue[2] == 127)
					setPadColor(launchPadControl, ColorBrightnessEnum::RedFull);
				if (controlValue[2] == 0)
					setPadColor(launchPadControl, ColorBrightnessEnum::Off);
			}

		}
		if (logMode == LogMode::DEBUG) {
			std::string controlName = getControlName(*message);
			std::cout << "[" << (int)message->at(0) << "," << (int)message->at(1) << "," << (int)message->at(2) << "]";
			std::cout << " LaunchControl  = " << launchPadControl << ", " << controlName;
			std::cout << " Control value= " << launchPadControlValue << "\n";
		}
	}

}


void midiInCallbackWrapper(double deltatime, std::vector< unsigned char > *message, void *)
{
	//this wrapper is a solution for the fact that midin.setCallback requires a static
	//pointer

	//this class
	LaunchControl* launchControl = (LaunchControl*)ptMidiCallback;
	launchControl->midiInCallback(deltatime, message, 0);

}

void LaunchControl::update()
{

}
std::vector<unsigned char> LaunchControl::GetSysExMessage(std::vector<unsigned char> * dataBytes)
{
	//Header (1 byte) + Manufacturer ID (3 bytes) + [data] + Tail (1 bytes) = 4 + size(dataBytes)
	int sizeInByte = dataBytes->size();
	std::vector<unsigned char> sysexMsg;

	sysexMsg.push_back(SYSSEX_HEAD);

	//Asumes that SysexID will always have 3 bytes.
	sysexMsg.push_back(SYSEX_ID[0]);
	sysexMsg.push_back(SYSEX_ID[1]);
	sysexMsg.push_back(SYSEX_ID[2]);

	for (size_t i = 0; i < sizeInByte; i++)
	{
		sysexMsg.push_back(dataBytes->at(i));
	}
	sysexMsg.push_back(SYSSEX_TAIL);
	return sysexMsg;

}

/*
Set pad/button LEDs
System Exclusive messages can be used to set the red and green LED values for any pad in any
template, regardless of which template is currently selected. This is done using the following
message:

<-  ID   -> <-FIXED? ->
Hex version F0h 00h 20h 29h 02h 0Ah 78h Template LED Value F7h
Dec version 240 0	32	 41	  2 10  120 Template LED Value 247

Where Template is 00h-07h (0-7) for the 8 user templates, and 08h-0Fh (8-15) for the 8 factory
templates;
LED is the index of the pad/button (00h-07h (0-7) for pads, 08h-0Bh (8-11) for buttons);

and Value is the velocity byte that defines the brightness values of both the red and green LEDs.

*/
void LaunchControl::setPadColor(int padNumber, ColorBrightnessEnum color)
{
	//set LED color is fixed, always has these 3 bytes
	std::vector<unsigned char> setColorFixed;
	setColorFixed.push_back(0x02);
	setColorFixed.push_back(0x0A);
	setColorFixed.push_back(0x78);

	//Template. Factory 1
	setColorFixed.push_back(0x08);

	//index of the pad / button(00h - 07h(0 - 7) for pads, 08h-0Bh(8 - 11) for buttons);
	setColorFixed.push_back(padNumber);

	//Velocity byte. For LED operations, velocity has the brightness and color of the LED.
	setColorFixed.push_back(color);
	midiout->sendMessage(&GetSysExMessage(&setColorFixed));
}

void LaunchControl::setTemplate(unsigned char templateNumber)
{
	//Fixed sequence for template
	std::vector<unsigned char> setTemplateMessage;
	setTemplateMessage.push_back(0x02);
	setTemplateMessage.push_back(0x0A);
	setTemplateMessage.push_back(0x77);

	//Template. Factory 1
	setTemplateMessage.push_back(templateNumber);

	midiout->sendMessage(&GetSysExMessage(&setTemplateMessage));
}
/*
Reset Launch Control. Hex version Bnh, 00h, 00h. Dec version 176+n, 0, 0
All LEDs are turned off, and the buffer settings and duty cycle are reset to their default values. The
MIDI channel n defines the template for which this message is intended (00h-07h (0-7) for the 8
user templates, and 08h-0Fh (8-15) for the 8 factory templates).
*/
void LaunchControl::resetLaunchControl(unsigned char templateNumber)
{
	auto message = std::vector<unsigned char>({ (unsigned char)(176 + templateNumber), 0x00, 0x00 });
	midiout->sendMessage(&message);
}



std::string  LaunchControl::getControlName(std::vector<unsigned char>& message)
{
	std::string result = "";
	bool pad = true;
	auto controlEnum = messageToControl(message);
	switch (controlEnum)
	{
		//TODO right now I "force" a switch always
	case Controls::PAD1:
		break;
	case Controls::PAD2:
		break;
	case Controls::PAD3:
		break;
	case Controls::PAD4:
		break;
	case Controls::PAD5:
		break;
	case Controls::PAD6:
		break;
	case Controls::PAD7:
		break;
	case Controls::PAD8:
		break;
	default:
		pad = false;
		break;
	}

	if (pad) {
		result = "PAD " + std::to_string(controlEnum + 1);
		return result;
	}

	//handling the knobs
	switch (controlEnum) {
		//For knows, we also update the value of the control
	case Controls::KNOB_1:

		break;
	case Controls::KNOB_2:
		break;
	case Controls::KNOB_3:
		break;
	case Controls::KNOB_4:
		break;
	case Controls::KNOB_5:
		break;
	case Controls::KNOB_6:
		break;
	case Controls::KNOB_7:
		break;
	case Controls::KNOB_8:
		break;
	case Controls::KNOB_9:
		break;
	case Controls::KNOB_10:
		break;
	case Controls::KNOB_11:
		break;
	case Controls::KNOB_12:
		break;
	case Controls::KNOB_13:
		break;
	case Controls::KNOB_14:
		break;
	case Controls::KNOB_15:
		break;
	case Controls::KNOB_16:
		break;
	case Controls::UNKNOWN:
		return "UNKNOWN";
		break;
	}
	if (!pad)
		result = "KNOB " + std::to_string(controlEnum - 7);
	return result;
}

unsigned char* LaunchControl::controlValues(LaunchControl::Controls &control)
{
	if (control == LaunchControl::Controls::PAD1)
		return &PAD_1[0];
	if (control == LaunchControl::Controls::PAD2)
		return &PAD_2[0];
	if (control == LaunchControl::Controls::PAD3)
		return &PAD_3[0];
	if (control == LaunchControl::Controls::PAD4)
		return &PAD_4[0];
	if (control == LaunchControl::Controls::PAD5)
		return &PAD_5[0];
	if (control == LaunchControl::Controls::PAD6)
		return &PAD_6[0];
	if (control == LaunchControl::Controls::PAD7)
		return &PAD_7[0];
	if (control == LaunchControl::Controls::PAD8)
		return &PAD_8[0];

	return nullptr;
}

bool LaunchControl::isPad(LaunchControl::Controls &control)
{
	switch (control) {
	case Controls::PAD1:
		return true;
		break;
	case Controls::PAD2:
		return true;
		break;
	case Controls::PAD3:
		return true;
		break;
	case Controls::PAD4:
		return true;
		break;
	case Controls::PAD5:
		return true;
		break;
	case Controls::PAD6:
		return true;
		break;
	case Controls::PAD7:
		return true;
		break;
	case Controls::PAD8:
		return true;
		break;
	default:
		return false;
		break;
	}
	return false;
}

LaunchControl::Controls LaunchControl::messageToControl(std::vector<unsigned char>& message)
{
	int controlValue = message[2];

	//Using memcmp with vector and array will not give
	//the expected results, even if the corresponding items
	//have the same value.
	unsigned char messageToCompare[] = { message.at(0),message.at(1),message.at(2) };
	LaunchControl::Controls result = Controls::UNKNOWN;
	if (memcmp(messageToCompare, PAD_1.data, 2) == 0) {
		PAD_1[2] = abs(PAD_1[2] - controlValue);
		result = Controls::PAD1;
	}
	else if (memcmp(messageToCompare, PAD_2.data, 2) == 0) {
		PAD_2[2] = abs(PAD_2[2] - controlValue);
		result = Controls::PAD2;
	}
	else if (memcmp(messageToCompare, PAD_3.data, 2) == 0) {
		PAD_3[2] = abs(PAD_3[2] - controlValue);
		result = Controls::PAD3;
	}
	else if (memcmp(messageToCompare, PAD_4.data, 2) == 0) {
		PAD_4[2] = abs(PAD_4[2] - controlValue);
		result = Controls::PAD4;
	}
	else if (memcmp(messageToCompare, PAD_5.data, 2) == 0) {
		PAD_5[2] = abs(PAD_5[2] - controlValue);
		result = Controls::PAD5;
	}
	else if (memcmp(messageToCompare, PAD_6.data, 2) == 0) {
		PAD_6[2] = abs(PAD_6[2] - controlValue);
		result = Controls::PAD6;
	}
	else if (memcmp(messageToCompare, PAD_7.data, 2) == 0) {
		PAD_7[2] = abs(PAD_7[2] - controlValue);
		result = Controls::PAD7;
	}
	else if (memcmp(messageToCompare, PAD_8.data, 2) == 0) {
		PAD_8[2] = abs(PAD_8[2] - controlValue);
		result = Controls::PAD8;
	}
	//KNOBS
	else if (memcmp(messageToCompare, KNOB_1_LOW.data, 2) == 0) {
		KNOB_1_LOW[2] = controlValue;
		result = Controls::KNOB_1;
	}
	else if (memcmp(messageToCompare, KNOB_2_LOW.data, 2) == 0) {
		KNOB_2_LOW[2] = controlValue;
		result = Controls::KNOB_2;
	}
	else if (memcmp(messageToCompare, KNOB_3_LOW.data, 2) == 0) {
		KNOB_3_LOW[2] = controlValue;
		result = Controls::KNOB_3;
	}
	else if (memcmp(messageToCompare, KNOB_4_LOW.data, 2) == 0) {
		KNOB_4_LOW[2] = controlValue;
		result = Controls::KNOB_4;
	}
	else if (memcmp(messageToCompare, KNOB_5_LOW.data, 2) == 0) {
		KNOB_5_LOW[2] = controlValue;
		result = Controls::KNOB_5;
	}
	else if (memcmp(messageToCompare, KNOB_6_LOW.data, 2) == 0) {
		KNOB_6_LOW[2] = controlValue;
		result = Controls::KNOB_6;
	}
	else if (memcmp(messageToCompare, KNOB_7_LOW.data, 2) == 0) {
		KNOB_7_LOW[2] = controlValue;
		result = Controls::KNOB_7;
	}
	else if (memcmp(messageToCompare, KNOB_8_LOW.data, 2) == 0) {
		KNOB_8_LOW[2] = controlValue;
		result = Controls::KNOB_8;

	}
	else if (memcmp(messageToCompare, KNOB_1_UPPER.data, 2) == 0) {
		KNOB_1_UPPER[2] = controlValue;
		result = Controls::KNOB_9;
	}
	else if (memcmp(messageToCompare, KNOB_2_UPPER.data, 2) == 0) {
		KNOB_2_UPPER[2] = controlValue;
		result = Controls::KNOB_10;
	}
	else if (memcmp(messageToCompare, KNOB_3_UPPER.data, 2) == 0) {
		KNOB_3_UPPER[2] = controlValue;
		result = Controls::KNOB_11;
	}
	else if (memcmp(messageToCompare, KNOB_4_UPPER.data, 2) == 0) {
		KNOB_4_UPPER[2] = controlValue;
		result = Controls::KNOB_12;
	}
	else if (memcmp(messageToCompare, KNOB_5_UPPER.data, 2) == 0) {
		KNOB_5_UPPER[2] = controlValue;
		result = Controls::KNOB_13;
	}
	else if (memcmp(messageToCompare, KNOB_6_UPPER.data, 2) == 0) {
		KNOB_6_UPPER[2] = controlValue;
		result = Controls::KNOB_14;
	}
	else if (memcmp(messageToCompare, KNOB_7_UPPER.data, 2) == 0) {
		KNOB_7_UPPER[2] = controlValue;
		result = Controls::KNOB_15;
	}
	else if (memcmp(messageToCompare, KNOB_8_UPPER.data, 2) == 0) {
		KNOB_8_UPPER[2] = controlValue;
		result = Controls::KNOB_16;
	}

	return result;
}


void LaunchControl::printMessage(double deltatime, std::vector< unsigned char >& message)
{
	unsigned int nBytes = message.size();
	std::cout << "Bytes[ " << nBytes << "]: ";
	for (unsigned int i = 0; i < nBytes; i++)
		std::cout << (int)message.at(i) << ", ";
	if (nBytes > 0)
		std::cout << " Timestamp = " << deltatime;

	if (nBytes <= 3) {
		LaunchControl::Controls launchPadControl = LaunchControl::messageToControl(message);
		int launchPadControlValue = (int)message.at(2);

		std::string controlName = getControlName(message);
		std::cout << " LaunchControl  = " << launchPadControl << ", " << controlName;
		std::cout << " Control value= " << launchPadControlValue;
	}
	std::cout << std::endl;

}

void LaunchControl::sendMessage(std::vector<unsigned char>* message)
{
	midiout->sendMessage(message);
}

bool LaunchControl::openLaunchControlMidiPorts(RtMidiIn *midiIn, RtMidiOut *midiOut)
{

	bool launchControlFound = false;

	// Check inputs.
	unsigned int nPorts = midiIn->getPortCount();

#if _DEBUG
	std::cout << "There is(are) " << nPorts << " MIDI input source(s) available.\n";
#endif


	if (nPorts == 0) {
		std::cout << "No input ports available!" << std::endl;
		return false;
	}
	std::string portName;
	for (unsigned int i = 0; i < nPorts; i++) {
		portName = midiIn->getPortName(i);
		//Using LaunchControl in this case
		if (portName.find(DEVICE_NAME) != std::string::npos)
		{
			std::cout << "Opening input port " << portName << std::endl;
			midiIn->openPort(i);
			launchControlFound = true;
			break;
		}
	} //end input ports
	if (!launchControlFound)
	{
		throw RtMidiError(ERROR_DEVICE_NOT_FOUND, RtMidiError::INVALID_DEVICE);
	}

	//Output ports
	launchControlFound = false;
	nPorts = midiOut->getPortCount();

	if (nPorts == 0) {
		std::cout << "No output ports available!" << std::endl;
		return false;
	}
#if _DEBUG
	std::cout << "\nThere is(are) " << nPorts << " MIDI output source(s) available.\n";
#endif

	for (int i = 0; i < nPorts; i++) {
		portName = midiOut->getPortName(i);
#if _DEBUG
		std::cout << "  Output port #" << i << ": " << portName << '\n';
#endif
		//Using LaunchControl in this case
		if (portName.find(DEVICE_NAME) != std::string::npos)
		{
			std::cout << "Opening output port " << portName << std::endl;
			midiOut->openPort(i);
			return true;
			launchControlFound = true;
		}

	}
	if (!launchControlFound)
	{

		throw RtMidiError(ERROR_DEVICE_NOT_FOUND, RtMidiError::INVALID_DEVICE);
	}

}
