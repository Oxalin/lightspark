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
	th->captureDevice = alcCaptureOpenDevice( th->captureDeviceName.c_str(), CAPTURE_FREQ, AL_FORMAT_MONO16, BUFFER_SIZE );

	if ( !th->captureDevice )	//verify capture device could be opened
	{
		checkALError( "Error while initializing capture device." );
	}
}

void OpenALPlugin::initPlayback( OpenALPlugin *th )
{
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
			checkALError( "Error while initializing context." );
		}
	}
	else
	{
		checkALError( "Error while initializing playback device." );
	}
}

void OpenALPlugin::freeStream( AudioStream* audioStream )
{
	ALint state = AL_STOPPED;
	assert( audioStream );
	OpenALAudioStream *ALStream = static_cast<OpenALAudioStream *>( audioStream );

	//Do not delete the stream now, let's wait termination. However, removing it from the list.
	streams.remove( ALStream );
	audioStream = NULL;

	do
	{
		alGetSourcei( ALStream->pbSource, AL_SOURCE_STATE, &state );
	}
	while ( state != AL_STOPPED );

	delete ALStream;

}

bool OpenALPlugin::isTimingAvailable() const
{
	return contextReady;
}

void OpenALPlugin::pauseStream( AudioStream* audioStream )
{
	if ( audioStream->isValid() )
	{
		ALint state = AL_INITIAL;
		OpenALAudioStream *ALStream = static_cast<OpenALAudioStream *>( audioStream );

		//Query source state
		alGetSourcei( ALStream->pbSource, AL_SOURCE_STATE, &state );

		if ( state == AL_PLAYING )	//Pause source
		{
			alSourcePause( ALStream->pbSource );
			ALStream->setStatus( PAUSED );
		}
	}
}

void OpenALPlugin::playStream( AudioStream* audioStream )
{
	if ( audioStream->isValid() )
	{
		ALint state = AL_INITIAL;
		OpenALAudioStream *ALStream = static_cast<OpenALAudioStream *>( audioStream );

		//Associate and fill stream's buffers
		ALStream->fill();

		//Query source state
		alGetSourcei( ALStream->pbSource, AL_SOURCE_STATE, &state );

		if ( state == AL_PLAYING )	//Is the stream already playing?
		{
			return;
		}
		else	//Start playing the stream from where it is
		{
			alSourcePlay( ALStream->pbSource );
			ALStream->setStatus( PLAYING );
#ifdef DEBUG
			cout << "Playing stream." << endl;
#endif
		}
	}
}

void OpenALPlugin::stopStream( AudioStream* audioStream )
{
	if ( audioStream->isValid() )
	{
		ALint state = AL_INITIAL;
		OpenALAudioStream *ALStream = static_cast<OpenALAudioStream *>( audioStream );

		//Query source state
		alGetSourcei( ALStream->pbSource, AL_SOURCE_STATE, &state );

		if (( state == AL_PLAYING ) || ( state == AL_PAUSED ) )	//Stop stream and reinitialize it
		{
			alSourceStop( ALStream-> pbSource );
			ALStream->empty();
			ALStream->setStatus( STOPPED );
#ifdef DEBUG
			cout << "Stream stopped." << endl;
#endif
		}
	}
}

AudioStream *OpenALPlugin::createStream( AudioDecoder *decoder )
{
	assert( decoder->isValid() );
	OpenALAudioStream *ALStream = new OpenALAudioStream( this, decoder );
	streams.push_back( ALStream );	//Create new SoundStream

	if ( contextReady )
	{
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
#ifdef DEBUG
			cout << "Unsupported number of channels: " << decoder->channelCount << endl;
#endif
			ALStream->setStatus( DEAD );
		}

		ALStream->setStatus( PAUSED );
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
desiredDeviceName should be empty to use the default. Else, it should be the name of the device.
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

OpenALAudioStream::OpenALAudioStream( OpenALPlugin *m, AudioDecoder *dec ):
		AudioStream( dec ), manager( m ), filling( false ), numBuffers(0), streamBaseOffset(0)
{
	uint8_t countBuf = NUM_BUFFERS;
//	unqueuedIterator = pbBuffers.begin();

	while ( countBuf > 0 )	//Creating default unqueued buffers pool
	{
		ALuint tmpBuffer;

		if ( !createBuffer( tmpBuffer ) )
		{
#ifdef DEBUG
			cout << "Error creating buffer. " << countBuf << " couldn't be created." << endl;
#endif
			status = DEAD;
			deleteBuffer( tmpBuffer );
			break;
		}

		queueBuffer( tmpBuffer );
		countBuf--;
	}

	alGenSources( NUM_SOURCES, &pbSource );

	if ( checkALError( "Error in alGenSources." ) )
	{
		status = DEAD;
	}

	//Setting default parameters for the source
	alSource3f( pbSource, AL_POSITION, 0.0, 0.0, 0.0 );

	alSource3f( pbSource, AL_VELOCITY, 0.0, 0.0, 0.0 );

	alSource3f( pbSource, AL_DIRECTION, 0.0, 0.0, 0.0 );

	alSourcef( pbSource, AL_ROLLOFF_FACTOR, 0.0 );

	alSourcei( pbSource, AL_SOURCE_RELATIVE, AL_TRUE );

	status = READY;
}

OpenALAudioStream::~OpenALAudioStream()
{
	empty();	//Emptying the stream, so all buffers are PENDING

	alDeleteSources( NUM_SOURCES, &pbSource );
	alDeleteBuffers( numBuffers, &pbBuffers.front() );
}

uint32_t OpenALAudioStream::getPlayedTime()
{
	uint32_t time = 0;

	if ( isValid() )
	{
		ALint playedBytes = 0;

		alGetSourcei( pbSource, AL_BYTE_OFFSET, &playedBytes );	//Bytes played since the playing began
		streamBaseOffset += playedBytes;	//Adding streamBaseOffset bytes

		//Every second, we consume 2Bytes (16bit) * NumChannel * freq (Hz)
		time = streamBaseOffset / ( freq / 1000 );	//freq / 1000 = milliseconds

		if ( format == AL_FORMAT_MONO16 )
		{
			time /= 2;
		}
		else if ( format == AL_FORMAT_STEREO16 )
		{
			time /= 4;	//Consuming twice as much in stereo
		}
	}

#ifdef DEBUG
	cout << "Played time (msec): " << time << endl;
#endif
	return time;
}

void OpenALAudioStream::setPlayedTime( uint32_t basetime )
{
	uint64_t Bytes = 0;

	//Every second, we consume 2Bytes (16bit) * NumChannel * freq (Hz)
	Bytes = basetime / 1000 * freq;

	if ( format == AL_FORMAT_MONO16 )
	{
		Bytes *= 2;
	}
	else if ( format == AL_FORMAT_STEREO16 )	//Consuming twice as much in stereo
	{
		Bytes *= 4;
	}

	streamBaseOffset = Bytes;
}

void OpenALAudioStream::fill()
{
	if ( isValid() )
	{
		if(!filling)
		{
			filling = true;
			uint32_t ret = 0;
			bool errorFilling = false;
			ALint queuedBuf;
			ALint unqueuedBuf;
			ALint processedBuf;
			
			alGetSourcei(pbSource, AL_BUFFERS_QUEUED, &queuedBuf);	//Are the buffers associated with the source
			alGetSourcei(pbSource, AL_BUFFERS_PROCESSED, &processedBuf);	//Get the number of buffers processed (free to be used)
			unqueuedBuf = numBuffers - queuedBuf;	//Number of buffers still not associated to the source

			uint32_t unqueuedIndex = numBuffers - unqueuedBuf;

			#ifdef DEBUG
			cout << processedBuf << " processedBuf, " << unqueuedBuf << " unqueuedBuf available. ";
			cout << "Total of buffers running: " << numBuffers << endl;
			#endif

			while(filling)	//filling all buffers one by one
			{
				if(unqueuedBuf > 0)	//Use buffers that are not queued in the source yet
				{
					#ifdef DEBUG
					cout << "Filling an unqueuedBuf." << endl;
					#endif
					ret = fillBuffer( &pbBuffers[unqueuedIndex], errorFilling );
					if( errorFilling || (ret == 0) )
					{
						break;
					}
					
					alSourceQueueBuffers(pbSource, 1, &pbBuffers[unqueuedIndex] );
					checkALError("Error queuing buffer.");
					
					unqueuedIndex++;
					unqueuedBuf--;
				}
				else
				{
					ALuint tmpBuffer;
					createBuffer( tmpBuffer );
					ret = fillBuffer( &tmpBuffer, errorFilling );
					
					if( errorFilling || ( ret == 0 ) )
					{
						deleteBuffer( tmpBuffer );
						break;
					}

					if(processedBuf > 0)	//Reuse free queued buffers
					{
						ALuint emptyBuffer;
						#ifdef DEBUG
						cout << "Filling a processedBuf." << endl;
						#endif
		
						alSourceUnqueueBuffers(pbSource, 1, &emptyBuffer);
						checkALError("Error unqueuing buffer.");

						emptyBuffer = tmpBuffer;
						alSourceQueueBuffers( pbSource, 1, &emptyBuffer );
						checkALError( "Error queuing buffer." );
						
						processedBuf--;
					}
					else
					{
						#ifdef DEBUG
						cout << "Filling a new unqueuedBuf." << endl;
						#endif

						queueBuffer( tmpBuffer );
						alSourceQueueBuffers(pbSource, 1, &pbBuffers.back() );
						checkALError("Error queuing new buffer.");
					}
				}
			}
			filling = false;
		}
	}
	else
	{
		//Just skip all the contents
		decoder->skipAll();
	}
}

/**
Returns data size written to know if there might be more.
If 0, no more data to copy for now
@buffer to fill
@error detected when filling
*/
uint32_t OpenALAudioStream::fillBuffer( ALuint *buffer, bool &err )
{
	int16_t *dest = ( int16_t * )malloc( BUFFER_SIZE );
	uint32_t totalWritten = 0;
	uint32_t retSize = 0;
	size_t frameSize = BUFFER_SIZE;

	do
	{
		retSize = this->decoder->copyFrame( dest + ( totalWritten / 2 ), frameSize );	//copying data while moving in temp buffer

		if ( retSize == 0 )	//No more data to buffer
		{
			break;
		}
		else	//We retrieved data, adding retSize to totalWritten
		{
			totalWritten += retSize;
			frameSize -= retSize;
		}
	}
	while ( frameSize );	//Looping buffer as much data as possible

#ifdef DEBUG
	cout << totalWritten << " bytes ready to be buffered." << endl;
#endif

	if ( totalWritten )	//We have data to put in buffer
	{
		alBufferData( *buffer, format, dest, totalWritten, freq );

		if ( checkALError( "Error filling buffer." ) )	//Error buffering?
		{
			err = true;
		}
	}

	free( dest );
	return totalWritten;
}

//Empty all buffers associated to the source and unqueue
void OpenALAudioStream::empty()
{
	ALint numBuf;

	alSourceStop(pbSource);		//Before emptying, we need to make sure the source is stopped, markinh all buffers as processed
	alGetSourcei(pbSource, AL_BUFFERS_QUEUED, &numBuf);	//Get number of buffers queued attached

	ALuint tmpBuffers[numBuf];
	alSourceUnqueueBuffers(pbSource, numBuf, tmpBuffers);	//Unqueues all buffers associated with the source

	checkALError("Error emptying buffers from stream.");	//Nothing more to be done
}

//Check if the stream is paused
bool OpenALAudioStream::paused()
{
	if ( isValid() )
	{
		ALint state;
		alGetSourcei( pbSource, AL_SOURCE_STATE, &state );

		//Return true either if the source has been volontarily paused or if it stopped because of underrun
		if ((status == PAUSED) || ( state == AL_PAUSED ) || (( status == PLAYING ) && (state == AL_STOPPED)) )
		{
#ifdef DEBUG
			cout << "paused() == true" << endl;
#endif
			return true;
		}
	}

	return false;
}

//Check if the stream is still alive
bool OpenALAudioStream::isValid()
{
	if ( status != DEAD )
	{
#ifdef DEBUG
		cout << "isValid() == true" << endl;
#endif
		return true;
	}

	return false;
}

//Returns true if new buffer created at the end of the vector
bool OpenALAudioStream::createBuffer(ALuint &buffer)
{
	alGenBuffers( 1, &buffer );

	if ( checkALError( "Error generating new buffer." ) )
	{
		status = DEAD;	//Really???
		return false;
	}
	
	return true;
}

bool OpenALAudioStream::deleteBuffer(ALuint &buffer)
{
	alDeleteBuffers( 1, &buffer );

	if ( checkALError( "Error deleting buffer." ) )
	{
		return false;
	}
	
	return true;
}

bool OpenALAudioStream::queueBuffer(ALuint &buffer)
{
	pbBuffers.push_back( buffer );
	numBuffers++;
	return true;
}

/**
Returns true if there is an OpenAL error
*/
bool checkALError( string errorMessage )
{
	ALenum error;

	if (( error = alGetError() ) != AL_NO_ERROR )
	{
		cout << errorMessage << endl;
		cout << "OpenAL error: " << error << endl;
		return true;
	}

	return false;
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
