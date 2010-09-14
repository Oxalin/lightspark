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

#include <iostream>	//needed because of a bug in ffmpeg, otherwise it complains about UINT64_C not defined
#include "OpenALPlugin.h"

using namespace lightspark;

using namespace std;


OpenALPlugin::OpenALPlugin( string initName, string initAudiobackend, bool initContext, bool initStopped ):
		IAudioPlugin( initName, initAudiobackend, initStopped )
{
	playbackDevice = NULL;
	captureDevice = NULL;
	contextReady = initContext;

	initialize();
}

void OpenALPlugin::initialize()
{
	//Generate lists (playback and capture)
	generateDevicesList( &playbackDevicesList, PLAYBACK );
	generateDevicesList( &captureDevicesList, CAPTURE );

	//Find info on selected devices in config file
	//getConfig() //To be implemented at a later time

	//Initialize context (playback and capture)
	initPlayback( this );
	initCapture( this );
}

void OpenALPlugin::initCapture( OpenALPlugin *th )
{
	ALenum error;
	th->captureDevice = alcCaptureOpenDevice( th->captureDeviceName.c_str(), CAPTURE_FREQ, AL_FORMAT_MONO16, BUFFER_SIZE );

	if ( !th->captureDevice )	//verify capture device could be opened
	{
		error = alGetError();
		cout << "Error while initializing capture device: " << error << endl;
	}
}

void OpenALPlugin::initPlayback( OpenALPlugin *th )
{
	ALenum error;
	th->playbackDevice = alcOpenDevice( th->playbackDeviceName.c_str() );

	if ( th->playbackDevice ) //verify playback device could be opened
	{
		context = alcCreateContext( th->playbackDevice, NULL );

		if ( context )	//If the context could be created, context is ready
		{
			contextReady = true;
			alcMakeContextCurrent( th->context );
		}
		else
		{
			contextReady = false;
			error = alGetError();
			cout << "Error while initializing context: " << error << endl;
		}
	}
	else
	{
		error = alGetError();
		cout << "Error while initializing playback device: " << error << endl;
	}
}

void OpenALPlugin::freeStream( AudioStream* audioStream )
{
	ALint state;
	assert( audioStream );
	OpenALAudioStream *ALStream = static_cast<OpenALAudioStream *>( audioStream );

	//Do not delete the stream now, let's wait termination. However, removing it from the list.
	streams.remove( ALStream );
	audioStream = NULL;

	do
	{
		alSourcei(ALStream->pbSource, AL_SOURCE_STATE, &state);
	}
	while ( state != AL_STOPPED);

	delete ALStream;

}

bool OpenALPlugin::isTimingAvailable() const
{
	return true;	//to be modified later
}

void OpenALPlugin::pauseStream( AudioStream* audioStream )
{
	OpenALAudioStream *ALStream = static_cast<OpenALAudioStream *>( audioStream );

	//Pause source
	alSourcePause( ALStream->pbSource );
	ALStream->setStatus(PAUSED);

}

void OpenALPlugin::resumeStream( AudioStream* audioStream )
{
	OpenALAudioStream *ALStream = static_cast<OpenALAudioStream *>( audioStream );

	//	//Resume context?
//	alcProcessContext(context);

	//Resume source
	alSourcePlay( ALStream->pbSource );
	ALStream->setStatus(PLAYING);
}

/*bool OpenALPlugin::serverAvailable() const
{
	return !noServer;
}*/
/*
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
*/
AudioStream *OpenALPlugin::createStream( AudioDecoder *decoder )
{
	OpenALAudioStream *ALStream = new OpenALAudioStream( this );
	streams.push_back( ALStream );	//Create new SoundStream

	if ( contextReady )
	{
		assert( decoder->isValid() );
		ALStream->decoder = decoder;
		ALStream->setStatus( READY );

		ALStream->freq = decoder->sampleRate;

		if ( decoder->channelCount == 1 )
		{
			ALStream->format = AL_FORMAT_MONO16;
		}
		else if ( decoder->channelCount == 2 )
		{
			ALStream->format = AL_FORMAT_STEREO16;
		}
		else
		{
			cout << "Unsupported number of channels: " << decoder->channelCount << endl;
			ALStream->setStatus( DEAD );
		}
	}
	else
	{
		//Create the stream as dead.
		//Dead streams should never leave that state, so make sure to use isValid.
		ALStream->setStatus( DEAD );
	}

	return ALStream;
}

void OpenALPlugin::generateDevicesList( vector< string* >* devicesList, DEVICE_TYPES desiredType )
{
	if ( alcIsExtensionPresent( NULL, "ALC_ENUMERATION_EXT" ) == AL_TRUE )  //Check if the extension if found
	{
		ALCchar *devices;

		if ( desiredType == PLAYBACK )
		{
			devices = ( ALCchar * ) alcGetString( NULL, ALC_DEVICE_SPECIFIER );
		}

		else if ( desiredType == CAPTURE )
		{
			devices = ( ALCchar * ) alcGetString( NULL, ALC_CAPTURE_DEVICE_SPECIFIER );
		}

		/*		while () //Split the devices' name and add them to the device list
				{
				  deviceName = ;
					addDeviceToList( devicesList, deviceName );
				}
		*/
	}
}

void OpenALPlugin::addDeviceToList( std::vector< string* >* devicesList, string* deviceName )
{
	uint32_t index = devicesList->size(); //we want to add the device to the end of the list

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
void OpenALPlugin::setDevice( string desiredDeviceName, DEVICE_TYPES desiredType )
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

		if (( tmpDevice = alcOpenDevice(( const char * )desiredDeviceName.c_str() ) ) ) //The device could be opened
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

		if (( tmpDevice = alcCaptureOpenDevice(( const char * )desiredDeviceName.c_str(), CAPTURE_FREQ, AL_FORMAT_MONO16, BUFFER_SIZE ) ) ) //The device could be opened
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
	terminate();
}

void OpenALPlugin::terminate()
{
	if ( !stopped )
	{
		stopped = true;

		// Stop context
		alcSuspendContext( context );

		// Delete streams
		for ( stream_iterator it = streams.begin();it != streams.end(); it++ )
		{
			freeStream( *it );
		}

		// Release the context, then delete it (destroying a context destroys sources)
		alcMakeContextCurrent( NULL );
		alcDestroyContext( context );
		context = NULL;

		// Close devices (only when buffers and context are deleted

		if ( playbackDevice != NULL )
		{
			alcCloseDevice( playbackDevice );
			playbackDevice = NULL;
		}

		if ( captureDevice != NULL )
		{
			alcCaptureCloseDevice( captureDevice );
			captureDevice = NULL;
		}
	}
}

OpenALAudioStream::OpenALAudioStream( OpenALPlugin* m ):
		AudioStream( NULL, STARTING ), manager( m )
{
	ALenum error;
	alGenBuffers( NUM_BUFFERS, pbBuffers );

	if (( error = alGetError() ) != AL_NO_ERROR )
	{
		cout << "alGenBuffers: " << error << endl;
		status = DEAD;
//		setStatus(DEAD);
	}

	alGenSources( NUM_SOURCES, &pbSource );

	if (( error = alGetError() ) != AL_NO_ERROR )
	{
		cout << "alGenSources: " << error << endl;
		status = DEAD;
//		setStatus(DEAD);
	}

	//Setting default parameters for the source
	alSource3f( pbSource, AL_POSITION, 0.0, 0.0, 0.0 );
	alSource3f( pbSource, AL_VELOCITY, 0.0, 0.0, 0.0 );
	alSource3f( pbSource, AL_DIRECTION, 0.0, 0.0, 0.0 );
	alSourcef( pbSource, AL_ROLLOFF_FACTOR, 0.0 );
	alSourcei( pbSource, AL_SOURCE_RELATIVE, AL_TRUE );
}

OpenALAudioStream::~OpenALAudioStream()
{
	empty();	//Emptying the stream, so all buffers are PENDING
	
	alDeleteSources(NUM_SOURCES, &pbSource);
	alDeleteBuffers(NUM_BUFFERS, pbBuffers);
}

uint32_t OpenALAudioStream::getPlayedTime()
{
	if ( isValid() )
	{
		return 0;	//returns 0 for now, to be modified
	}
	else
	{
		return 0;
	}
}

void OpenALAudioStream::fill()
{
	if ( isValid() )
	{
		int numBuf;
	    
		alGetSourcei(pbSource, AL_BUFFERS_PROCESSED, &numBuf);	//Get the number of buffers processed (free to be used)
	    
		while(numBuf--)	//filling all processed buffers one by one
		{
			ALuint emptyBuffer;
			
			alSourceUnqueueBuffers(pbSource, 1, &emptyBuffer);
			if (( error = alGetError() ) != AL_NO_ERROR )
			{
				cout << "Error unqueuing buffer: " << error << endl;
			}

			fillBuffer(emptyBuffer);
			alSourceQueueBuffers(pbSource, 1, &emptyBuffer);
			if (( error = alGetError() ) != AL_NO_ERROR )
			{
				cout << "Error queuing buffer: " << error << endl;
			}
		}
	}
	else
	{
		//Just skip all the contents
		decoder->skipAll();
	}
}

void OpenALAudioStream::fillBuffer( ALuint buffer )
{
	int16_t *dest;
	uint32_t totalWritten = 0;
	size_t frameSize = BUFFER_SIZE;
    
	do
	{
		uint32_t retSize = this->decoder->copyFrame( dest + ( totalWritten / 2 ), frameSize);	//copying data while moving in temp buffer
	    
		if(retSize = 0)	//No more data to buffer
		{
			break;
		}
		else	//We retrieved data, adding retSize to totalWritten
		{
			totalWritten += retSize;
			frameSize -= retSize;
		}
	}
	while(frameSize)	//Looping buffer as much data as possible

	cout << "Filled " << totalWritten << endl;
	if ( totalWritten )	//We have data to put in buffer
	{
		alBufferData(buffer, format, dest, totalWritten, freq);
		if (( error = alGetError() ) != AL_NO_ERROR )
		{
			cout << "Error filling buffer: " << error << endl;
		}
	}
}

void OpenALAudioStream::empty()
{
	ALint numBuf;
	ALenum error;
	ALuint buffer;
	
	alSourceStop(pbSource);		//Before emptying, we need to stop
	alGetSourcei(pbSource, AL_BUFFERS_QUEUED, &numBuf);	//Get number of buffers queued attached
	alSourceUnqueueBuffers(pbSource, numBuf, &buffer);	//Unqueues all buffers

	if (( error = alGetError() ) != AL_NO_ERROR )
	{
		cout << "Error emptying buffers from stream: " << error << endl;
	}
}

bool OpenALAudioStream::paused()
{
	if ( isValid() )
	{
		return status == PAUSED;
	}
	else
	{
		return false;
	}
}

bool OpenALAudioStream::isValid()
{
	return status != DEAD;
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
