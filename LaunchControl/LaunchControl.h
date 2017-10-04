#pragma once
#include <vector>
#include <memory>
#include "RtMidi.h"

void midiInCallback(double deltatime, std::vector< unsigned char > *message, void *);

class LaunchControl
{
public:
	enum LogMode : unsigned int
	{
		DEBUG = 0,
		ERR = 1
	};

	struct Pad
	{
		unsigned char data[3];
		const unsigned char& operator[](size_t i) const {
			return data[i];
		}
		unsigned char& operator[](size_t i) {
			return data[i];
		}

		bool On() const
		{
			return (data[2] == 127);
		}
	};
	struct Knob
	{
		unsigned char data[3];
		const unsigned char& operator[](size_t i) const {
			return data[i];
		}
		unsigned char& operator[](size_t i) {
			return data[i];
		}

		int Value()
		{
			return data[2];
		}
	};
private:
	//SYSSEX must start with 0F and end with F7
	static const char SYSSEX_HEAD = 0xF0;
	static const char SYSSEX_TAIL = 0xF7;
	static const std::string ERROR_DEVICE_NOT_FOUND;

	//if true, it will always turn the pad light on and off
	bool forceToggleMode;
	LogMode logMode;


	RtMidiIn *midiin;
	RtMidiOut *midiout;

public:
	//initializing arrays in construnctors
	static const std::string DEVICE_NAME;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Constructor. </summary>
	///
	/// <remarks>	Scheihal, 22/04/2016. </remarks>
	///	
	/// <param name="toggleMode">	  	true to enable toggle mode, false to disable it. In toggle mode,
	/// 								each pad LED will turn on and off after pressing.</param>
	////////////////////////////////////////////////////////////////////////////////////////////////////
	LaunchControl(bool toggleMode, LaunchControl::LogMode logMode = LogMode::ERR);
	void init();
	~LaunchControl();
	bool openLaunchControlMidiPorts(RtMidiIn *midiIn, RtMidiOut *midiOut);
	
	void printMessage(double deltaTime, std::vector<unsigned char>& message);
	void sendMessage(std::vector<unsigned char>* message);
	std::vector< unsigned char > currentMessage;

	std::vector<unsigned char> GetSysExMessage(std::vector<unsigned char> * dataBytes);

	//MIDI ID for Focusrite/Novation = 0x00, 0x00, 0x29 = (0,32,41)
	//This can be seen from MIDI.org but also when send the non-standard buttons from the device, 
	//such as Templates User and Factory
	unsigned char SYSEX_ID[3];
	Knob KNOB_1_UPPER;
	Knob KNOB_2_UPPER;
	Knob KNOB_3_UPPER;
	Knob KNOB_4_UPPER;
	Knob KNOB_5_UPPER;
	Knob KNOB_6_UPPER;
	Knob KNOB_7_UPPER;
	Knob KNOB_8_UPPER;

	Knob KNOB_1_LOW;
	Knob KNOB_2_LOW;
	Knob KNOB_3_LOW;
	Knob KNOB_4_LOW;
	Knob KNOB_5_LOW;
	Knob KNOB_6_LOW;
	Knob KNOB_7_LOW;
	Knob KNOB_8_LOW;


	//These mean "pressed" = MIDI, NOTE ON = 0
	//Channel: in the Factory template, LaunchControl will have assigned channel 1 assigned to all knobs and pads.
	//MIDI channels go from 1 to 16, and channel 1 is represented by 0x0 in hexadecimal.

	//First byte of MIDI message is Status)
	//Pads 1 to 8 will send "note on" when pressed and "note off" when released.
	// "Note on" is 8n, where n is channel.For channel 1 : 0x80 = 128
	// "note off" is 9n, where n is channel. for channel 1 : 0x90 = 144
	// Third byte for "Note on" is velocity. It seems LaunchControl sends note on with velocity 0 (and Note Off with velocity 127).
	;
	Pad PAD_1;
	Pad PAD_2;
	Pad PAD_3;
	Pad PAD_4;
	Pad PAD_5;
	Pad PAD_6;
	Pad PAD_7;
	Pad PAD_8; 

	/*
	Hex Decimal Colour Brightness, according to Novation LaunchControl documentation.
	0Ch 12 Off Off
	0Dh 13 Red Low
	0Fh 15 Red Full
	1Dh 29 Amber Low
	3Fh 63 Amber Full
	3Eh 62 Yellow Full
	1Ch 28 Green Low
	3Ch 60 Green Full
	*/
	enum ColorBrightnessEnum : unsigned char
	{
		Off = 0x0C,
		RedLow = 0x0D,
		RedFull = 0x0F,
		AmberLow = 0x1D,
		AmberFull = 0x3F,
		YellowFull = 0x3E,
		GreenLow = 0x1C,
		GreenFull = 0x3C
	};
	static ColorBrightnessEnum ColorBrightness[8];

	//The ID of controls in the devices. The order matches the codes expected in SysEx messages.
	enum Controls : unsigned char
	{
		PAD1 = 0x00,
		PAD2 = 0x01,
		PAD3 = 0x02,
		PAD4 = 0x03,
		PAD5 = 0x04,
		PAD6 = 0x05,
		PAD7 = 0x06,
		PAD8 = 0x07,
		KNOB_1 = 0x08,
		KNOB_2 = 0x09,
		KNOB_3 = 0x0A,
		KNOB_4 = 0x0B,
		KNOB_5 = 0x0C,
		KNOB_6 = 0x0D,
		KNOB_7 = 0x0E,
		KNOB_8 = 0x0F,
		
		KNOB_9 = 0x10,
		KNOB_10 = 0x11,
		KNOB_11 = 0x12,
		KNOB_12 = 0x13,
		KNOB_13 = 0x14,
		KNOB_14 = 0x15,
		KNOB_15 = 0x16,
		KNOB_16 = 0x17,

		UNKNOWN = 0x18
	};

	Controls messageToControl(std::vector<unsigned char>& message);
	//Returns a label for the control, such as "PAD 1" or "KNOB 1". Non-standard, just to identify which control sent the message.
	std::string  LaunchControl::getControlName(std::vector<unsigned char>& message);
	unsigned char* controlValues(LaunchControl::Controls& control);
	static bool isPad(LaunchControl::Controls& control);
	void LaunchControl::setPadColor(int pad, ColorBrightnessEnum color);
	void LaunchControl::setTemplate(unsigned char templateNumber);
	void LaunchControl::resetLaunchControl(unsigned char templateNumber);

	//Not working
	void LaunchControl::midiInCallback(double deltatime, std::vector< unsigned char > *message, void *);
	


	//Must be called regurlarly to update the controls values with the value of the last MIDI message.
	void update();


};

