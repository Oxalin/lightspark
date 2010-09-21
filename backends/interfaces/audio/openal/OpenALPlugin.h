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

#define NUM_BUFFERS 3	//Default number of buffers created. The buffer manager will take care of adding some if needed
#define BUFFER_SIZE 4096	//Keeping buffer size as low as possible for latency, but without static noise
#define NUM_SOURCES 1
#define MAX_BUFFER_TIME 250	//Maximum time to buffer in msec
#define CAPTURE_FREQ 44100
//#define DEBUG

class OpenALPlugin : public IAudioPlugin
{
private:
	ALCcontext *context;
	ALCdevice *playbackDevice;
	ALCdevice *captureDevice;
	void initPlayback( OpenALPlugin *th );
	void initCapture( OpenALPlugin *th );
	void addDeviceToList ( std::vector<std::string *> *devicesList, std::string *deviceName );
	void generateDevicesList ( std::vector<std::string *> *devicesList, DEVICE_TYPES desiredType ); //To populate the devices lists, devicesType must be playback or capture
	void initialize();
	void terminate();
	bool contextReady;
public:
	OpenALPlugin( std::string initName = "OpenAL plugin", std::string initAudiobackend = "openal",
		      bool initContext = false, bool initStopped = false );
	void setDevice( std::string desiredDeviceName, DEVICE_TYPES desiredType );
	AudioStream *createStream( lightspark::AudioDecoder *decoder );
	void freeStream( AudioStream *audioStream );
	void pauseStream( AudioStream *audioStream );
	void playStream( AudioStream *audioStream);
	void stopStream( AudioStream *audioStream);
	bool isTimingAvailable() const;
	~OpenALPlugin();
};

class OpenALAudioStream : public AudioStream
{
public:
	OpenALAudioStream( OpenALPlugin *m, lightspark::AudioDecoder *dec );
	~OpenALAudioStream();
	uint32_t getPlayedTime();
	void setPlayedTime(uint32_t basetime);	//set basetime when seeking, basetime is in msec
	void play();
	void pause();
	void stop();
	STREAM_STATUS getStatus();
	bool paused();
	bool isValid();
	void fill();
	void empty();
	OpenALPlugin *manager;
	ALenum	format;
	ALuint	freq;

  private:
	uint64_t streamBaseOffset;	//Basetime when seeking (in BYTES), so it can be added to the played time (AL_BYTE_OFFSET).
	std::vector<ALuint> pbBuffers;	//Vector of buffers
	std::vector<uint32_t> pbBuffersDataSize;	//Vector of data size of vector
	ALuint	pbSource;	//In flash, only one source per stream
	bool filling;		//Are we already filling buffers?
	uint32_t fillBuffer(ALuint *buffer, bool &err);
	bool createBuffer(ALuint &buffer);	//Create a new buffer and add it at queue
	bool deleteBuffer(ALuint &buffer);	//Delete last unqueued buffer
	bool unqueueBuffer(ALuint &buffer);	//Pop the first buffer processed (free) from the source and increment played bytes
	bool queueBuffer(ALuint &buffer);	//Queue the buffer
	void getALState(ALint &state);	//Query the state of the source
};

bool checkALError(std::string errorMessage);

#endif