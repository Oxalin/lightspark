/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010 Alexandre Demers (papouta@hotmail.com)

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

#ifndef PULSEPLUGIN_H
#define PULSEPLUGIN_H

#include <pulse/pulseaudio.h>
#include "../IAudioPlugin.h"
#include "../../../decoder.h"
#include "../../../../compat.h"
#include <iostream>

class PulseAudioStream;  //Early declaration

class PulsePlugin : public IAudioPlugin
{
private:
	pa_threaded_mainloop *mainLoop;
	pa_context *context;
	static void contextStatusCB( pa_context *context, PulsePlugin *th );
	void initialize();
	void terminate();
	static void playbackListCB( pa_context *context, const pa_sink_info *list, int eol, void *th );
	static void captureListCB( pa_context *context, const pa_source_info *list, int eol, void *th );
	void addDeviceToList( std::vector<std::string *> *devicesList, std::string *deviceName );
	void generateDevicesList( std::vector<std::string *> *devicesList, DEVICE_TYPES desiredType );  //To populate the devices lists, devicesType must be playback or capture
	static void streamStatusCB( pa_stream *stream, PulseAudioStream *th );
	static void streamWriteCB( pa_stream *stream, size_t nbytes, PulseAudioStream *th );
	bool contextReady;
	bool noServer;
public:
	PulsePlugin( std::string init_Name = "Pulse plugin output only", std::string init_audiobackend = "pulse",
	             bool init_contextReady = false, bool init_noServer = false, bool init_stopped = false );
	void setDevice( std::string desiredDevice, DEVICE_TYPES desiredType );
	AudioStream *createStream( lightspark::AudioDecoder *decoder );
	void freeStream( AudioStream *audioStream );
	void pauseStream( AudioStream *audioStream );
	void playStream( AudioStream *audioStream);
	void stopStream( AudioStream *audioStream);
	bool isTimingAvailable() const;
	void pulseLock();
	void pulseUnlock();
	bool serverAvailable() const;
	~PulsePlugin();
};

class PulseAudioStream: public AudioStream
{
public:
	PulseAudioStream( PulsePlugin *m, lightspark::AudioDecoder *dec );
	uint32_t getPlayedTime();
	void setPlayedTime(uint32_t basetime);
	bool paused();
	bool isValid();
	void fill();
	void empty();

	pa_stream *stream;
	PulsePlugin *manager;
private:
	uint32_t streamBaseOffset;	//Basetime when seeking in msec, so it can be added to the played time.
};

void overflow_notify();
void underflow_notify();
void started_notify();

#endif
