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

#include "OpenALPlugin.h"

using namespace lightspark;
using namespace std;


OpenALPlugin::OpenALPlugin( string init_Name, string init_audiobackend, bool init_stopped ):
		IAudioPlugin ( init_Name, init_audiobackend, init_stopped )
{
	playbackDevice = NULL;
	captureDevice = NULL;

	start();
}

void OpenALPlugin::start()
{
	/*
	Generate lists (playback and captures)
	Find info on selected devices in config file
	open devices
	create context and activate it
	create buffers
	create sources
	Don't forget to check for errors
	*/
	ALenum error;

	generateDevicesList( playbackDevicesList, PLAYBACK );
	generateDevicesList( captureDevicesList, CAPTURE );
	//getConfig() //To be implemented at a later time

	initPlayback( this );
	initCapture( this );
}

void OpenALPlugin::initCapture( OpenALPlugin *th )
{
/*
	th->captureDevice = alcCaptureOpenDevice( th->captureDeviceName );
	
	if ()	//verify capture device could be opened
	{
	}
*/
}

void OpenALPlugin::initPlayback( OpenALPlugin *th )
{
  ALenum error;
	th->playbackDevice = alcOpenDevice( th->playbackDeviceName );

	if ( th->playbackDevice ) //verify playback device could be opened
	{
		context = alcCreateContext( th->playbackDevice, NULL );
		alcMakeContextCurrent( th->context );
	}

	alGetError();	//Clearing error code

	alGenBuffers( NUM_BUFFERS, th->pbBuffers );

	if (( error = alGetError() ) != AL_NO_ERROR )
	{
		cout << "alGenBuffers :" << error << endl;
		return;
	}

	alGenSources( NUM_SOURCES, th->pbSources );

	if (( error = alGetError() ) != AL_NO_ERROR )
	{
		cout << "alGenSources :" << error << endl;
		return;
	}

}

void OpenALPlugin::freeStream( AudioStream* audioStream )
{

}

bool OpenALPlugin::isTimingAvailable() const
{

}

void OpenALPlugin::pauseStream( AudioStream* audioStream )
{

}

void OpenALPlugin::resumeStream( AudioStream* audioStream )
{

}

bool OpenALPlugin::serverAvailable() const
{

}

void overflow_notify()
{
	cout << "____overflow!!!!" << endl;
}

void underflow_notify()
{
	cout << "____underflow!!!!" << endl;
}

void started_notify()
{
	cout << "____started!!!!" << endl;
}

AudioStream *OpenALPlugin::createStream( AudioDecoder *decoder )
{/*
	while(!contextReady);
	pa_threaded_mainloop_lock(mainLoop);
	uint32_t index=0;
	for(;index<streams.size();index++)
	{
		if(streams[index]==NULL)
			break;
	}
	assert(decoder->isValid());
	if(index==streams.size())
		streams.push_back(new AudioStream(this));
	streams[index]->decoder=decoder;
	if(noServer==false)
	{
		pa_sample_spec ss;
		ss.format=PA_SAMPLE_S16LE;
		ss.rate=decoder->sampleRate;
		ss.channels=decoder->channelCount;
		pa_buffer_attr attrs;
		attrs.maxlength=(uint32_t)-1;
		attrs.prebuf=0;
		attrs.tlength=(uint32_t)-1;
		attrs.fragsize=(uint32_t)-1;
		attrs.minreq=(uint32_t)-1;
		streams[index]->stream=pa_stream_new(context, "AudioStream", &ss, NULL);
		pa_stream_set_state_callback(streams[index]->stream, (pa_stream_notify_cb_t)streamStatusCB, streams[index]);
		pa_stream_set_write_callback(streams[index]->stream, (pa_stream_request_cb_t)streamWriteCB, streams[index]);
		pa_stream_set_underflow_callback(streams[index]->stream, (pa_stream_notify_cb_t)underflow_notify, NULL);
		pa_stream_set_overflow_callback(streams[index]->stream, (pa_stream_notify_cb_t)overflow_notify, NULL);
		pa_stream_set_started_callback(streams[index]->stream, (pa_stream_notify_cb_t)started_notify, NULL);
		pa_stream_connect_playback(streams[index]->stream, NULL, &attrs,
			(pa_stream_flags)(PA_STREAM_START_CORKED), NULL, NULL);
	}
	else
	{
		//Create the stream as dead
		streams[index]->streamStatus=AudioStream::STREAM_DEAD;
	}
	pa_threaded_mainloop_unlock(mainLoop);
	return index+1;
*/}

void OpenALPlugin::generateDevicesList( vector< string* >* devicesList, DEVICE_TYPES desiredType )
{
	if ( alcIsExtensionPresent( NULL, "ALC_ENUMERATION_EXT" ) == AL_TRUE )  //Check if the extension if found
	{
		ALCchar *devices;

		if ( desiredType == PLAYBACK )
		{
			devices = alcGetString( NULL, ALC_DEVICE_SPECIFIER );
		}

		else if ( desiredType == CAPTURE )
		{
			devices = alcGetString( NULL, ALC_CAPTURE_DEVICE_SPECIFIER );
		}

		while () //Split the devices' name and add them to the device list
		{
		  deviceName = ;
			addDeviceToList( devicesList, deviceName );
		}
	}
}

void OpenALPlugin::addDeviceToList( std::vector< string* >* devicesList, string* deviceName )
{
	uint32_t index = devicesList->size(); //we want to add the plugin to the end of the list

	if ( devicesList->size() == ( uint32_t )( index ) )
	{
		devicesList->push_back( new string( *deviceName ) );
	}
}

/**********************
desiredDevice should be empty to use the default. Else, it should be the name of the device.
When setting a device, we should check if there is a context active using the current device
  If so, suspend it, unload previous device, load the new one, update the context and restart it
**********************/
void OpenALPlugin::set_device( string desiredDeviceName, DEVICE_TYPES desiredType )
{
	ALCdevice *tmpDevice = NULL;

	switch ( desiredType )
	{
	case PLAYBACK:

		if (( char * )alcGetString( playbackDevice, ALC_DEVICE_SPECIFIER ) == desiredDeviceName.c_str() )	//already selected
		{
			return;
		}

		if ( desiredDeviceName == "" ) //Find the default device if NULL
		{
			desiredDeviceName = alcGetString( NULL, ALC_DEFAULT_DEVICE_SPECIFIER );
		}

		if ( tmpDevice = alcOpenDevice(( const char * )desiredDeviceName.c_str() ) ) //The device could be opened
		{
			if ( playbackDevice != NULL ) //Close the old device if opened
			{
				alcCloseDevice( playbackDevice );
			}

			playbackDevice = tmpDevice;
		}

		break;

	case CAPTURE:

		if (( char * )alcGetString( captureDevice, ALC_CAPTURE_DEVICE_SPECIFIER ) == desiredDeviceName.c_str() )	//already selected
		{
			return;
		}

		if ( desiredDeviceName == "" ) //Find the default device if NULL
		{
			desiredDeviceName = alcGetString( NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER );
		}

		if ( tmpDevice = alcCaptureOpenDevice(( const char * )desiredDeviceName.c_str() ) ) //The device could be opened
		{
			if ( captureDevice != NULL ) //Close the old device if opened
			{
				alcCaptureCloseDevice( captureDevice );
			}

			captureDevice = tmpDevice;
		}

		break;

	default:
		break;
	}

}


OpenALPlugin::~OpenALPlugin()
{
	stop();
}

void OpenALPlugin::stop()
{
	if ( !stopped )
	{
		stopped = true;

		// stop context
		// delete sources
		// delete buffers
		// delete context


		// close devices

		if (( playbackDevice != NULL ) || ( captureDevice != NULL ) )
		{
			alcCloseDevice( playbackDevice );
			playbackDevice = NULL;
			alcCaptureCloseDevice( captureDevice );
			captureDevice = NULL;
		}
	}
}

OpenALAudioStream::OpenALAudioStream( OpenALPlugin* m ):
		AudioStream( NULL, false )
{

}

uint32_t OpenALAudioStream::getPlayedTime()
{

}

void OpenALAudioStream::fill()
{

}

bool OpenALAudioStream::paused()
{

}

// Plugin factory function
extern "C" DLL_PUBLIC IPlugin *create()
{
	return new OpenALPlugin();
}

// Plugin cleanup function
extern "C" DLL_PUBLIC void release( IPlugin *p_plugin )
{
	//delete the previously created object
	delete p_plugin;
}
