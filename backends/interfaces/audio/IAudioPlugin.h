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

#ifndef IAUDIOPLUGIN_H
#define IAUDIOPLUGIN_H

#include "../../../compat.h"
#include "../../decoder.h"
#include "../IPlugin.h"
#include <iostream>


enum STREAM_STATUS { STARTING = 0, READY, DEAD, PLAYING, PAUSED, STOPPED };

class AudioStream
{
  protected:
	STREAM_STATUS status;	//Indicates the stream status
	AudioStream( lightspark::AudioDecoder *dec = NULL );

  public:
	lightspark::AudioDecoder *decoder;
	virtual bool paused() = 0;	//Is the stream paused? (corked)
	virtual bool isValid() = 0;	//Is the stream alive, fully working?
	virtual void setStatus( STREAM_STATUS streamStatus );	//Set the stream status
	virtual STREAM_STATUS getStatus();	//Get the stream status
	virtual uint32_t getPlayedTime() = 0;	//Get the time played in milliseconds
	virtual void setPlayedTime( uint32_t basetime ) = 0;	//Set the basetime (in msec) of a stream (so appropriate time when seeking can be returned)
	virtual void fill() = 0;	//Fill the stream without playing it
	virtual void empty() = 0;	//Empty the stream
	virtual ~AudioStream() {};
};

/**********************
Abstract class for audio plugin implementation
***********************/
class IAudioPlugin : public IPlugin
{
protected:
	std::string playbackDeviceName;
	std::string captureDeviceName;
	std::vector<std::string *> playbackDevicesList;
	std::vector<std::string *> captureDevicesList;
	std::list<AudioStream *> streams;
	typedef std::list<AudioStream *>::iterator stream_iterator;
	bool stopped;
	IAudioPlugin ( std::string plugin_name, std::string backend_name, bool init_stopped = false );

public:
	enum DEVICE_TYPES { PLAYBACK, CAPTURE };
	virtual std::vector<std::string *> *getDevicesList ( DEVICE_TYPES desiredType );
	virtual void setDevice ( std::string desiredDevice, DEVICE_TYPES desiredType ) = 0;
	virtual std::string getDevice ( DEVICE_TYPES desiredType );
	virtual AudioStream *createStream ( lightspark::AudioDecoder *decoder ) = 0;
	virtual void freeStream ( AudioStream *audioStream ) = 0;
	virtual void pauseStream( AudioStream *audioStream ) = 0;	//Pause the stream (stops time from running, cork)
//	virtual void resumeStream( AudioStream *audioStream ) = 0;	//Resume the stream (restart time, uncork)
	virtual void playStream( AudioStream *audioStream ) = 0;	//Start playing the stream
	virtual void stopStream( AudioStream *audioStream ) = 0;	//Stop playing the stream and reinitialize it
	virtual bool isTimingAvailable() const = 0;
	virtual ~IAudioPlugin();
};

#endif

