#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

/**********************************************************************/
/*! \class RtMidi
\brief An abstract base class for realtime MIDI input/output.

This class implements some common functionality for the realtime
MIDI input/output subclasses RtMidiIn and RtMidiOut.

RtMidi WWW site: http://music.mcgill.ca/~gary/rtmidi/

RtMidi: realtime MIDI i/o C++ classes
Copyright (c) 2003-2016 Gary P. Scavone

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation files
(the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

Any person wishing to distribute modifications to the Software is
asked to send the modifications to the original developer so that
they can be incorporated into the canonical version.  This is,
however, not a binding provision of this license.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/**********************************************************************/

#include "RtMidi.h"
#include <sstream>

#if defined(__MACOSX_CORE__)
#if TARGET_OS_IPHONE
#define AudioGetCurrentHostTime CAHostTimeBase::GetCurrentTime
#define AudioConvertHostTimeToNanos CAHostTimeBase::ConvertToNanos
#endif
#endif

//*********************************************************************//
//  RtMidi Definitions
//*********************************************************************//

RtMidi::RtMidi()
	: rtapi_(0)
{
}

RtMidi :: ~RtMidi()
{
	if (rtapi_)
		delete rtapi_;
	rtapi_ = 0;
}

std::string RtMidi::getVersion(void) throw()
{
	return std::string(RTMIDI_VERSION);
}

void RtMidi::getCompiledApi(std::vector<RtMidi::Api> &apis) throw()
{
	apis.clear();

	// The order here will control the order of RtMidi's API search in
	// the constructor.
#if defined(__MACOSX_CORE__)
	apis.push_back(MACOSX_CORE);
#endif
#if defined(__LINUX_ALSA__)
	apis.push_back(LINUX_ALSA);
#endif
#if defined(__UNIX_JACK__)
	apis.push_back(UNIX_JACK);
#endif
#if defined(__WINDOWS_MM__)
	apis.push_back(WINDOWS_MM);
#endif
#if defined(__RTMIDI_DUMMY__)
	apis.push_back(RTMIDI_DUMMY);
#endif
}

//*********************************************************************//
//  RtMidiIn Definitions
//*********************************************************************//

void RtMidiIn::openMidiApi(RtMidi::Api api, const std::string clientName, unsigned int queueSizeLimit)
{
	if (rtapi_)
		delete rtapi_;
	rtapi_ = 0;

#if defined(__UNIX_JACK__)
	if (api == UNIX_JACK)
		rtapi_ = new MidiInJack(clientName, queueSizeLimit);
#endif
#if defined(__LINUX_ALSA__)
	if (api == LINUX_ALSA)
		rtapi_ = new MidiInAlsa(clientName, queueSizeLimit);
#endif
#if defined(__WINDOWS_MM__)
	if (api == WINDOWS_MM)
		rtapi_ = new MidiInWinMM(clientName, queueSizeLimit);
#endif
#if defined(__MACOSX_CORE__)
	if (api == MACOSX_CORE)
		rtapi_ = new MidiInCore(clientName, queueSizeLimit);
#endif
#if defined(__RTMIDI_DUMMY__)
	if (api == RTMIDI_DUMMY)
		rtapi_ = new MidiInDummy(clientName, queueSizeLimit);
#endif
}

RtMidiIn::RtMidiIn(RtMidi::Api api, const std::string clientName, unsigned int queueSizeLimit)
	: RtMidi()
{
	if (api != UNSPECIFIED) {
		// Attempt to open the specified API.
		openMidiApi(api, clientName, queueSizeLimit);
		if (rtapi_) return;

		// No compiled support for specified API value.  Issue a warning
		// and continue as if no API was specified.
		std::cerr << "\nRtMidiIn: no compiled support for specified API argument!\n\n" << std::endl;
	}

	// Iterate through the compiled APIs and return as soon as we find
	// one with at least one port or we reach the end of the list.
	std::vector< RtMidi::Api > apis;
	getCompiledApi(apis);
	for (unsigned int i = 0; i<apis.size(); i++) {
		openMidiApi(apis[i], clientName, queueSizeLimit);
		if (rtapi_->getPortCount()) break;
	}

	if (rtapi_) return;

	// It should not be possible to get here because the preprocessor
	// definition __RTMIDI_DUMMY__ is automatically defined if no
	// API-specific definitions are passed to the compiler. But just in
	// case something weird happens, we'll throw an error.
	std::string errorText = "RtMidiIn: no compiled API support found ... critical error!!";
	throw(RtMidiError(errorText, RtMidiError::UNSPECIFIED));
}

RtMidiIn :: ~RtMidiIn() throw()
{
}


//*********************************************************************//
//  RtMidiOut Definitions
//*********************************************************************//

void RtMidiOut::openMidiApi(RtMidi::Api api, const std::string clientName)
{
	if (rtapi_)
		delete rtapi_;
	rtapi_ = 0;

#if defined(__UNIX_JACK__)
	if (api == UNIX_JACK)
		rtapi_ = new MidiOutJack(clientName);
#endif
#if defined(__LINUX_ALSA__)
	if (api == LINUX_ALSA)
		rtapi_ = new MidiOutAlsa(clientName);
#endif
#if defined(__WINDOWS_MM__)
	if (api == WINDOWS_MM)
		rtapi_ = new MidiOutWinMM(clientName);
#endif
#if defined(__MACOSX_CORE__)
	if (api == MACOSX_CORE)
		rtapi_ = new MidiOutCore(clientName);
#endif
#if defined(__RTMIDI_DUMMY__)
	if (api == RTMIDI_DUMMY)
		rtapi_ = new MidiOutDummy(clientName);
#endif
}

RtMidiOut::RtMidiOut(RtMidi::Api api, const std::string clientName)
{
	if (api != UNSPECIFIED) {
		// Attempt to open the specified API.
		openMidiApi(api, clientName);
		if (rtapi_) return;

		// No compiled support for specified API value.  Issue a warning
		// and continue as if no API was specified.
		std::cerr << "\nRtMidiOut: no compiled support for specified API argument!\n\n" << std::endl;
	}

	// Iterate through the compiled APIs and return as soon as we find
	// one with at least one port or we reach the end of the list.
	std::vector< RtMidi::Api > apis;
	getCompiledApi(apis);
	for (unsigned int i = 0; i<apis.size(); i++) {
		openMidiApi(apis[i], clientName);
		if (rtapi_->getPortCount()) break;
	}

	if (rtapi_) return;

	// It should not be possible to get here because the preprocessor
	// definition __RTMIDI_DUMMY__ is automatically defined if no
	// API-specific definitions are passed to the compiler. But just in
	// case something weird happens, we'll thrown an error.
	std::string errorText = "RtMidiOut: no compiled API support found ... critical error!!";
	throw(RtMidiError(errorText, RtMidiError::UNSPECIFIED));
}

RtMidiOut :: ~RtMidiOut() throw()
{
}

//*********************************************************************//
//  Common MidiApi Definitions
//*********************************************************************//

MidiApi::MidiApi(void)
	: apiData_(0), connected_(false), errorCallback_(0), errorCallbackUserData_(0)
{
}

MidiApi :: ~MidiApi(void)
{
}

void MidiApi::setErrorCallback(RtMidiErrorCallback errorCallback, void *userData = 0)
{
	errorCallback_ = errorCallback;
	errorCallbackUserData_ = userData;
}

void MidiApi::error(RtMidiError::Type type, std::string errorString)
{
	if (errorCallback_) {

		if (firstErrorOccurred_)
			return;

		firstErrorOccurred_ = true;
		const std::string errorMessage = errorString;

		errorCallback_(type, errorMessage, errorCallbackUserData_);
		firstErrorOccurred_ = false;
		return;
	}

	if (type == RtMidiError::WARNING) {
		std::cerr << '\n' << errorString << "\n\n";
	}
	else if (type == RtMidiError::DEBUG_WARNING) {
		//#if defined(__RTMIDI_DEBUG__)
		std::cerr << '\n' << errorString << "\n\n";
		//#endif
	}
	else {
		std::cerr << '\n' << errorString << "\n\n";
		throw RtMidiError(errorString, type);
	}
}

//*********************************************************************//
//  Common MidiInApi Definitions
//*********************************************************************//

MidiInApi::MidiInApi(unsigned int queueSizeLimit)
	: MidiApi()
{
	// Allocate the MIDI queue.
	inputData_.queue.ringSize = queueSizeLimit;
	if (inputData_.queue.ringSize > 0)
		inputData_.queue.ring = new MidiMessage[inputData_.queue.ringSize];
}

MidiInApi :: ~MidiInApi(void)
{
	// Delete the MIDI queue.
	if (inputData_.queue.ringSize > 0) delete[] inputData_.queue.ring;
}

void MidiInApi::setCallback(RtMidiCallback callback, void *userData)
{
	if (inputData_.usingCallback) {
		errorString_ = "MidiInApi::setCallback: a callback function is already set!";
		error(RtMidiError::WARNING, errorString_);
		return;
	}

	if (!callback) {
		errorString_ = "RtMidiIn::setCallback: callback function value is invalid!";
		error(RtMidiError::WARNING, errorString_);
		return;
	}

	inputData_.userCallback = callback;
	inputData_.userData = userData;
	inputData_.usingCallback = true;
}

void MidiInApi::cancelCallback()
{
	if (!inputData_.usingCallback) {
		errorString_ = "RtMidiIn::cancelCallback: no callback function was set!";
		error(RtMidiError::WARNING, errorString_);
		return;
	}

	inputData_.userCallback = 0;
	inputData_.userData = 0;
	inputData_.usingCallback = false;
}

void MidiInApi::ignoreTypes(bool midiSysex, bool midiTime, bool midiSense)
{
	inputData_.ignoreFlags = 0;
	if (midiSysex) inputData_.ignoreFlags = 0x01;
	if (midiTime) inputData_.ignoreFlags |= 0x02;
	if (midiSense) inputData_.ignoreFlags |= 0x04;
}

double MidiInApi::getMessage(std::vector<unsigned char> *message)
{
	message->clear();

	if (inputData_.usingCallback) {
		errorString_ = "RtMidiIn::getNextMessage: a user callback is currently set for this port.";
		error(RtMidiError::WARNING, errorString_);
		return 0.0;
	}

	if (inputData_.queue.size == 0) return 0.0;

	// Copy queued message to the vector pointer argument and then "pop" it.
	std::vector<unsigned char> *bytes = &(inputData_.queue.ring[inputData_.queue.front].bytes);
	message->assign(bytes->begin(), bytes->end());
	double deltaTime = inputData_.queue.ring[inputData_.queue.front].timeStamp;
	inputData_.queue.size--;
	inputData_.queue.front++;
	if (inputData_.queue.front == inputData_.queue.ringSize)
		inputData_.queue.front = 0;

	return deltaTime;
}

//*********************************************************************//
//  Common MidiOutApi Definitions
//*********************************************************************//

MidiOutApi::MidiOutApi(void)
	: MidiApi()
{
}

MidiOutApi :: ~MidiOutApi(void)
{
}

// *************************************************** //
//
// OS/API-specific methods.
//
// *************************************************** //



//*********************************************************************//
//  API: Windows Multimedia Library (MM)
//*********************************************************************//

// API information deciphered from:
//  - http://msdn.microsoft.com/library/default.asp?url=/library/en-us/multimed/htm/_win32_midi_reference.asp

// Thanks to Jean-Baptiste Berruchon for the sysex code.



// The Windows MM API is based on the use of a callback function for
// MIDI input.  We convert the system specific time stamps to delta
// time values.

// Windows MM MIDI header files.
#include <windows.h>
#include <mmsystem.h>

#define  RT_SYSEX_BUFFER_SIZE 1024
#define  RT_SYSEX_BUFFER_COUNT 4

// A structure to hold variables related to the CoreMIDI API
// implementation.
struct WinMidiData {
	HMIDIIN inHandle;    // Handle to Midi Input Device
	HMIDIOUT outHandle;  // Handle to Midi Output Device
	DWORD lastTime;
	MidiInApi::MidiMessage message;
	LPMIDIHDR sysexBuffer[RT_SYSEX_BUFFER_COUNT];
	CRITICAL_SECTION _mutex; // [Patrice] see https://groups.google.com/forum/#!topic/mididev/6OUjHutMpEo
};

//*********************************************************************//
//  API: Windows MM
//  Class Definitions: MidiInWinMM
//*********************************************************************//

static void CALLBACK midiInputCallback(HMIDIIN /*hmin*/,
	UINT inputStatus,
	DWORD_PTR instancePtr,
	DWORD_PTR midiMessage,
	DWORD timestamp)
{
	if (inputStatus != MIM_DATA && inputStatus != MIM_LONGDATA && inputStatus != MIM_LONGERROR) return;

	//MidiInApi::RtMidiInData *data = static_cast<MidiInApi::RtMidiInData *> (instancePtr);
	MidiInApi::RtMidiInData *data = (MidiInApi::RtMidiInData *)instancePtr;
	WinMidiData *apiData = static_cast<WinMidiData *> (data->apiData);

	// Calculate time stamp.
	if (data->firstMessage == true) {
		apiData->message.timeStamp = 0.0;
		data->firstMessage = false;
	}
	else apiData->message.timeStamp = (double)(timestamp - apiData->lastTime) * 0.001;
	apiData->lastTime = timestamp;

	if (inputStatus == MIM_DATA) { // Channel or system message

								   // Make sure the first byte is a status byte.
		unsigned char status = (unsigned char)(midiMessage & 0x000000FF);
		if (!(status & 0x80)) return;

		// Determine the number of bytes in the MIDI message.
		unsigned short nBytes = 1;
		if (status < 0xC0) nBytes = 3;
		else if (status < 0xE0) nBytes = 2;
		else if (status < 0xF0) nBytes = 3;
		else if (status == 0xF1) {
			if (data->ignoreFlags & 0x02) return;
			else nBytes = 2;
		}
		else if (status == 0xF2) nBytes = 3;
		else if (status == 0xF3) nBytes = 2;
		else if (status == 0xF8 && (data->ignoreFlags & 0x02)) {
			// A MIDI timing tick message and we're ignoring it.
			return;
		}
		else if (status == 0xFE && (data->ignoreFlags & 0x04)) {
			// A MIDI active sensing message and we're ignoring it.
			return;
		}

		// Copy bytes to our MIDI message.
		unsigned char *ptr = (unsigned char *)&midiMessage;
		for (int i = 0; i<nBytes; ++i) apiData->message.bytes.push_back(*ptr++);
	}
	else { // Sysex message ( MIM_LONGDATA or MIM_LONGERROR )
		MIDIHDR *sysex = (MIDIHDR *)midiMessage;
		if (!(data->ignoreFlags & 0x01) && inputStatus != MIM_LONGERROR) {
			// Sysex message and we're not ignoring it
			for (int i = 0; i<(int)sysex->dwBytesRecorded; ++i)
				apiData->message.bytes.push_back(sysex->lpData[i]);
		}

		// The WinMM API requires that the sysex buffer be requeued after
		// input of each sysex message.  Even if we are ignoring sysex
		// messages, we still need to requeue the buffer in case the user
		// decides to not ignore sysex messages in the future.  However,
		// it seems that WinMM calls this function with an empty sysex
		// buffer when an application closes and in this case, we should
		// avoid requeueing it, else the computer suddenly reboots after
		// one or two minutes.
		if (apiData->sysexBuffer[sysex->dwUser]->dwBytesRecorded > 0) {
			//if ( sysex->dwBytesRecorded > 0 ) {
			EnterCriticalSection(&(apiData->_mutex));
			MMRESULT result = midiInAddBuffer(apiData->inHandle, apiData->sysexBuffer[sysex->dwUser], sizeof(MIDIHDR));
			LeaveCriticalSection(&(apiData->_mutex));
			if (result != MMSYSERR_NOERROR)
				std::cerr << "\nRtMidiIn::midiInputCallback: error sending sysex to Midi device!!\n\n";

			if (data->ignoreFlags & 0x01) return;
		}
		else return;
	}

	if (data->usingCallback) {
		//RtMidiCallback callback = static_cast<RtMidiCallback>(data->userCallback);
		data->userCallback(apiData->message.timeStamp, &apiData->message.bytes, data->userData);
	}
	else {
		// As long as we haven't reached our queue size limit, push the message.
		if (data->queue.size < data->queue.ringSize) {
			data->queue.ring[data->queue.back++] = apiData->message;
			if (data->queue.back == data->queue.ringSize)
				data->queue.back = 0;
			data->queue.size++;
		}
		else
			std::cerr << "\nRtMidiIn: message queue limit reached!!\n\n";
	}

	// Clear the vector for the next input message.
	apiData->message.bytes.clear();
}

MidiInWinMM::MidiInWinMM(const std::string clientName, unsigned int queueSizeLimit) : MidiInApi(queueSizeLimit)
{
	initialize(clientName);
}

MidiInWinMM :: ~MidiInWinMM()
{
	// Close a connection if it exists.
	closePort();

	WinMidiData *data = static_cast<WinMidiData *> (apiData_);
	DeleteCriticalSection(&(data->_mutex));

	// Cleanup.
	delete data;
}

void MidiInWinMM::initialize(const std::string& /*clientName*/)
{
	// We'll issue a warning here if no devices are available but not
	// throw an error since the user can plugin something later.
	unsigned int nDevices = midiInGetNumDevs();
	if (nDevices == 0) {
		errorString_ = "MidiInWinMM::initialize: no MIDI input devices currently available.";
		error(RtMidiError::WARNING, errorString_);
	}

	// Save our api-specific connection information.
	WinMidiData *data = (WinMidiData *) new WinMidiData;
	apiData_ = (void *)data;
	inputData_.apiData = (void *)data;
	data->message.bytes.clear();  // needs to be empty for first input message

	if (!InitializeCriticalSectionAndSpinCount(&(data->_mutex), 0x00000400)) {
		errorString_ = "MidiInWinMM::initialize: InitializeCriticalSectionAndSpinCount failed.";
		error(RtMidiError::WARNING, errorString_);
	}
}

void MidiInWinMM::openPort(unsigned int portNumber, const std::string /*portName*/)
{
	if (connected_) {
		errorString_ = "MidiInWinMM::openPort: a valid connection already exists!";
		error(RtMidiError::WARNING, errorString_);
		return;
	}

	unsigned int nDevices = midiInGetNumDevs();
	if (nDevices == 0) {
		errorString_ = "MidiInWinMM::openPort: no MIDI input sources found!";
		error(RtMidiError::NO_DEVICES_FOUND, errorString_);
		return;
	}

	if (portNumber >= nDevices) {
		std::ostringstream ost;
		ost << "MidiInWinMM::openPort: the 'portNumber' argument (" << portNumber << ") is invalid.";
		errorString_ = ost.str();
		error(RtMidiError::INVALID_PARAMETER, errorString_);
		return;
	}

	WinMidiData *data = static_cast<WinMidiData *> (apiData_);
	MMRESULT result = midiInOpen(&data->inHandle,
		portNumber,
		(DWORD_PTR)&midiInputCallback,
		(DWORD_PTR)&inputData_,
		CALLBACK_FUNCTION);
	if (result != MMSYSERR_NOERROR) {
		errorString_ = "MidiInWinMM::openPort: error creating Windows MM MIDI input port.";
		error(RtMidiError::DRIVER_ERROR, errorString_);
		return;
	}

	// Allocate and init the sysex buffers.
	for (int i = 0; i<RT_SYSEX_BUFFER_COUNT; ++i) {
		data->sysexBuffer[i] = (MIDIHDR*) new char[sizeof(MIDIHDR)];
		data->sysexBuffer[i]->lpData = new char[RT_SYSEX_BUFFER_SIZE];
		data->sysexBuffer[i]->dwBufferLength = RT_SYSEX_BUFFER_SIZE;
		data->sysexBuffer[i]->dwUser = i; // We use the dwUser parameter as buffer indicator
		data->sysexBuffer[i]->dwFlags = 0;

		result = midiInPrepareHeader(data->inHandle, data->sysexBuffer[i], sizeof(MIDIHDR));
		if (result != MMSYSERR_NOERROR) {
			midiInClose(data->inHandle);
			errorString_ = "MidiInWinMM::openPort: error starting Windows MM MIDI input port (PrepareHeader).";
			error(RtMidiError::DRIVER_ERROR, errorString_);
			return;
		}

		// Register the buffer.
		result = midiInAddBuffer(data->inHandle, data->sysexBuffer[i], sizeof(MIDIHDR));
		if (result != MMSYSERR_NOERROR) {
			midiInClose(data->inHandle);
			errorString_ = "MidiInWinMM::openPort: error starting Windows MM MIDI input port (AddBuffer).";
			error(RtMidiError::DRIVER_ERROR, errorString_);
			return;
		}
	}

	result = midiInStart(data->inHandle);
	if (result != MMSYSERR_NOERROR) {
		midiInClose(data->inHandle);
		errorString_ = "MidiInWinMM::openPort: error starting Windows MM MIDI input port.";
		error(RtMidiError::DRIVER_ERROR, errorString_);
		return;
	}

	connected_ = true;
}

void MidiInWinMM::openVirtualPort(std::string /*portName*/)
{
	// This function cannot be implemented for the Windows MM MIDI API.
	errorString_ = "MidiInWinMM::openVirtualPort: cannot be implemented in Windows MM MIDI API!";
	error(RtMidiError::WARNING, errorString_);
}

void MidiInWinMM::closePort(void)
{
	if (connected_) {
		WinMidiData *data = static_cast<WinMidiData *> (apiData_);
		EnterCriticalSection(&(data->_mutex));
		midiInReset(data->inHandle);
		midiInStop(data->inHandle);

		for (int i = 0; i<RT_SYSEX_BUFFER_COUNT; ++i) {
			int result = midiInUnprepareHeader(data->inHandle, data->sysexBuffer[i], sizeof(MIDIHDR));
			delete[] data->sysexBuffer[i]->lpData;
			delete[] data->sysexBuffer[i];
			if (result != MMSYSERR_NOERROR) {
				midiInClose(data->inHandle);
				errorString_ = "MidiInWinMM::openPort: error closing Windows MM MIDI input port (midiInUnprepareHeader).";
				error(RtMidiError::DRIVER_ERROR, errorString_);
				return;
			}
		}

		midiInClose(data->inHandle);
		connected_ = false;
		LeaveCriticalSection(&(data->_mutex));
	}
}

unsigned int MidiInWinMM::getPortCount()
{
	return midiInGetNumDevs();
}

std::string MidiInWinMM::getPortName(unsigned int portNumber)
{
	std::string stringName;
	unsigned int nDevices = midiInGetNumDevs();
	if (portNumber >= nDevices) {
		std::ostringstream ost;
		ost << "MidiInWinMM::getPortName: the 'portNumber' argument (" << portNumber << ") is invalid.";
		errorString_ = ost.str();
		error(RtMidiError::WARNING, errorString_);
		return stringName;
	}

	MIDIINCAPS deviceCaps;
	midiInGetDevCaps(portNumber, &deviceCaps, sizeof(MIDIINCAPS));

#if defined( UNICODE ) || defined( _UNICODE )
	int length = WideCharToMultiByte(CP_UTF8, 0, deviceCaps.szPname, -1, NULL, 0, NULL, NULL) - 1;
	stringName.assign(length, 0);
	length = WideCharToMultiByte(CP_UTF8, 0, deviceCaps.szPname, static_cast<int>(wcslen(deviceCaps.szPname)), &stringName[0], length, NULL, NULL);
#else
	stringName = std::string(deviceCaps.szPname);
#endif

	// Next lines added to add the portNumber to the name so that 
	// the device's names are sure to be listed with individual names
	// even when they have the same brand name
	std::ostringstream os;
	os << " ";
	os << portNumber;
	stringName += os.str();

	return stringName;
}

//*********************************************************************//
//  API: Windows MM
//  Class Definitions: MidiOutWinMM
//*********************************************************************//

MidiOutWinMM::MidiOutWinMM(const std::string clientName) : MidiOutApi()
{
	initialize(clientName);
}

MidiOutWinMM :: ~MidiOutWinMM()
{
	// Close a connection if it exists.
	closePort();

	// Cleanup.
	WinMidiData *data = static_cast<WinMidiData *> (apiData_);
	delete data;
}

void MidiOutWinMM::initialize(const std::string& /*clientName*/)
{
	// We'll issue a warning here if no devices are available but not
	// throw an error since the user can plug something in later.
	unsigned int nDevices = midiOutGetNumDevs();
	if (nDevices == 0) {
		errorString_ = "MidiOutWinMM::initialize: no MIDI output devices currently available.";
		error(RtMidiError::WARNING, errorString_);
	}

	// Save our api-specific connection information.
	WinMidiData *data = (WinMidiData *) new WinMidiData;
	apiData_ = (void *)data;
}

unsigned int MidiOutWinMM::getPortCount()
{
	return midiOutGetNumDevs();
}

std::string MidiOutWinMM::getPortName(unsigned int portNumber)
{
	std::string stringName;
	unsigned int nDevices = midiOutGetNumDevs();
	if (portNumber >= nDevices) {
		std::ostringstream ost;
		ost << "MidiOutWinMM::getPortName: the 'portNumber' argument (" << portNumber << ") is invalid.";
		errorString_ = ost.str();
		error(RtMidiError::WARNING, errorString_);
		return stringName;
	}

	MIDIOUTCAPS deviceCaps;
	midiOutGetDevCaps(portNumber, &deviceCaps, sizeof(MIDIOUTCAPS));

#if defined( UNICODE ) || defined( _UNICODE )
	int length = WideCharToMultiByte(CP_UTF8, 0, deviceCaps.szPname, -1, NULL, 0, NULL, NULL) - 1;
	stringName.assign(length, 0);
	length = WideCharToMultiByte(CP_UTF8, 0, deviceCaps.szPname, static_cast<int>(wcslen(deviceCaps.szPname)), &stringName[0], length, NULL, NULL);
#else
	stringName = std::string(deviceCaps.szPname);
#endif

	// Next lines added to add the portNumber to the name so that 
	// the device's names are sure to be listed with individual names
	// even when they have the same brand name
	std::ostringstream os;
	os << " ";
	os << portNumber;
	stringName += os.str();

	return stringName;
}

void MidiOutWinMM::openPort(unsigned int portNumber, const std::string /*portName*/)
{
	if (connected_) {
		errorString_ = "MidiOutWinMM::openPort: a valid connection already exists!";
		error(RtMidiError::WARNING, errorString_);
		return;
	}

	unsigned int nDevices = midiOutGetNumDevs();
	if (nDevices < 1) {
		errorString_ = "MidiOutWinMM::openPort: no MIDI output destinations found!";
		error(RtMidiError::NO_DEVICES_FOUND, errorString_);
		return;
	}

	if (portNumber >= nDevices) {
		std::ostringstream ost;
		ost << "MidiOutWinMM::openPort: the 'portNumber' argument (" << portNumber << ") is invalid.";
		errorString_ = ost.str();
		error(RtMidiError::INVALID_PARAMETER, errorString_);
		return;
	}

	WinMidiData *data = static_cast<WinMidiData *> (apiData_);
	MMRESULT result = midiOutOpen(&data->outHandle,
		portNumber,
		(DWORD)NULL,
		(DWORD)NULL,
		CALLBACK_NULL);
	if (result != MMSYSERR_NOERROR) {
		errorString_ = "MidiOutWinMM::openPort: error creating Windows MM MIDI output port.";
		error(RtMidiError::DRIVER_ERROR, errorString_);
		return;
	}

	connected_ = true;
}

void MidiOutWinMM::closePort(void)
{
	if (connected_) {
		WinMidiData *data = static_cast<WinMidiData *> (apiData_);
		midiOutReset(data->outHandle);
		midiOutClose(data->outHandle);
		connected_ = false;
	}
}

void MidiOutWinMM::openVirtualPort(std::string /*portName*/)
{
	// This function cannot be implemented for the Windows MM MIDI API.
	errorString_ = "MidiOutWinMM::openVirtualPort: cannot be implemented in Windows MM MIDI API!";
	error(RtMidiError::WARNING, errorString_);
}

void MidiOutWinMM::sendMessage(std::vector<unsigned char> *message)
{
	if (!connected_) return;

	unsigned int nBytes = static_cast<unsigned int>(message->size());
	if (nBytes == 0) {
		errorString_ = "MidiOutWinMM::sendMessage: message argument is empty!";
		error(RtMidiError::WARNING, errorString_);
		return;
	}

	MMRESULT result;
	WinMidiData *data = static_cast<WinMidiData *> (apiData_);
	if (message->at(0) == 0xF0) { // Sysex message

								  // Allocate buffer for sysex data.
		char *buffer = (char *)malloc(nBytes);
		if (buffer == NULL) {
			errorString_ = "MidiOutWinMM::sendMessage: error allocating sysex message memory!";
			error(RtMidiError::MEMORY_ERROR, errorString_);
			return;
		}

		// Copy data to buffer.
		for (unsigned int i = 0; i<nBytes; ++i) buffer[i] = message->at(i);

		// Create and prepare MIDIHDR structure.
		MIDIHDR sysex;
		sysex.lpData = (LPSTR)buffer;
		sysex.dwBufferLength = nBytes;
		sysex.dwFlags = 0;
		result = midiOutPrepareHeader(data->outHandle, &sysex, sizeof(MIDIHDR));
		if (result != MMSYSERR_NOERROR) {
			free(buffer);
			errorString_ = "MidiOutWinMM::sendMessage: error preparing sysex header.";
			error(RtMidiError::DRIVER_ERROR, errorString_);
			return;
		}

		// Send the message.
		result = midiOutLongMsg(data->outHandle, &sysex, sizeof(MIDIHDR));
		if (result != MMSYSERR_NOERROR) {
			free(buffer);
			errorString_ = "MidiOutWinMM::sendMessage: error sending sysex message.";
			error(RtMidiError::DRIVER_ERROR, errorString_);
			return;
		}

		// Unprepare the buffer and MIDIHDR.
		while (MIDIERR_STILLPLAYING == midiOutUnprepareHeader(data->outHandle, &sysex, sizeof(MIDIHDR))) Sleep(1);
		free(buffer);
	}
	else { // Channel or system message.

		   // Make sure the message size isn't too big.
		if (nBytes > 3) {
			errorString_ = "MidiOutWinMM::sendMessage: message size is greater than 3 bytes (and not sysex)!";
			error(RtMidiError::WARNING, errorString_);
			return;
		}

		// Pack MIDI bytes into double word.
		DWORD packet;
		unsigned char *ptr = (unsigned char *)&packet;
		for (unsigned int i = 0; i<nBytes; ++i) {
			*ptr = message->at(i);
			++ptr;
		}

		// Send the message immediately.
		result = midiOutShortMsg(data->outHandle, packet);
		if (result != MMSYSERR_NOERROR) {
			errorString_ = "MidiOutWinMM::sendMessage: error sending MIDI message.";
			error(RtMidiError::DRIVER_ERROR, errorString_);
		}
	}
}
