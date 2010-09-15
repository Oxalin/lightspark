/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef OPENALPLUGIN_H
#define OPENALPLUGIN_H

#ifdef MACOSX
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include "../IAudioPlugin.h"
#include "../../../decoder.h"
#include "../../../../compat.h"
#include <iostream>

#define NUM_BUFFERS 4	//One that was played, one being played, two to be played
#define BUFFER_SIZE 19200
#define NUM_SOURCES 1
#define CAPTURE_FREQ 44100
//#define NUM_ENVIRONMENTS 1

class OpenALPlugin : public IAudioPlugin
{
private:
	ALCcontext *context;
	ALCdevice *playbackDevice;
	ALCdevice *captureDevice;
//	ALuint	environments[NUM_ENVIRONMENTS];
	void initPlayback( OpenALPlugin *th );
	void initCapture( OpenALPlugin *th );
	void addDeviceToList ( std::vector<std::string *> *devicesList, std::string *deviceName );
	void generateDevicesList ( std::vector<std::string *> *devicesList, DEVICE_TYPES desiredType ); //To populate the devices lists, devicesType must be playback or capture
	void initialize();
	void terminate();
	bool contextReady;
//	bool noServer;
public:
	OpenALPlugin( std::string initName = "OpenAL plugin", std::string initAudiobackend = "openal",
		      bool initContext = false, bool initStopped = false );
	void setDevice( std::string desiredDeviceName, DEVICE_TYPES desiredType );
	AudioStream *createStream( lightspark::AudioDecoder *decoder );
	void freeStream( AudioStream *audioStream );
	void pauseStream( AudioStream *audioStream );
	void resumeStream( AudioStream *audioStream );
	bool isTimingAvailable() const;
//	bool serverAvailable() const;
	~OpenALPlugin();
};

class OpenALAudioStream : public AudioStream
{
public:
	OpenALAudioStream( OpenALPlugin *m );
	~OpenALAudioStream();
	uint32_t getPlayedTime();
	bool paused();
	bool isValid();
	void fill();
	void empty();
	OpenALPlugin *manager;
	ALuint	pbBuffers[NUM_BUFFERS];
	ALuint	pbSource;	//In flash, only one source per stream
	ALenum	format;
	ALuint	freq;

  private:
	void fillBuffer(ALuint *buffer);
};

bool checkALError();

#endif