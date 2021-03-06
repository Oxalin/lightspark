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
	assert( audioStream );
	OpenALAudioStream *ALStream = static_cast<OpenALAudioStream *>( audioStream );

	//Do not delete the stream now, let's wait termination. However, removing it from the list.
	streams.remove( ALStream );
	audioStream = NULL;

	if ( ALStream->isValid() )
	{
		STREAM_STATUS state = ALStream->getStatus();

		while ( state != STOPPED )
		{
			state = ALStream->getStatus();
		}

		ALStream->empty();
	}

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
		OpenALAudioStream *ALStream = static_cast<OpenALAudioStream *>( audioStream );

		//Query source state
		STREAM_STATUS state = ALStream->getStatus();

		if ( state == PLAYING )	//Pause source
		{
			ALStream->pause();
		}
	}
}

void OpenALPlugin::playStream( AudioStream* audioStream )
{
	if ( audioStream->isValid() )
	{
		OpenALAudioStream *ALStream = static_cast<OpenALAudioStream *>( audioStream );

		//Associate and fill stream's buffers
		ALStream->fill();

		//Query source state
		STREAM_STATUS state = ALStream->getStatus();
		if ( state == PLAYING )	//Is the stream already playing?
		{
			return;
		}
		else	//Start playing the stream from where it is
		{
			ALStream->play();
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
		OpenALAudioStream *ALStream = static_cast<OpenALAudioStream *>( audioStream );

		//Query source state
		STREAM_STATUS state = ALStream->getStatus();

		if (( state == PLAYING ) || ( state == PAUSED ) )	//Stop stream and reinitialize it
		{
			ALStream->stop();
			ALStream->empty();
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

		ALStream->setStatus( PAUSED );	//Ready to play, putting it in a paused state
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
		AudioStream( dec ), manager( m ), filling( false ), streamBaseOffset(0)
{
	//Creating default unqueued buffers pool
	uint8_t countBuf = NUM_BUFFERS;

	while ( countBuf > 0 )
	{
		ALuint tmpBuffer;

		if ( !createBuffer( tmpBuffer ) )
		{
			#ifdef DEBUG
			cout << "Error creating buffer. " << countBuf << " couldn't be created." << endl;
			#endif
			deleteBuffer( tmpBuffer );
			break;
		}

		pbBuffers.push_back( tmpBuffer );
		countBuf--;
	}

	alGenSources( NUM_SOURCES, &pbSource );

	if ( checkALError( "Error in alGenSources." ) )
	{
		setStatus( DEAD );
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
	uint32_t numBuf = pbBuffers.size();
	uint32_t bufferIndex = 0;
	empty();	//Emptying the stream, so all buffers are PENDING

	alDeleteSources( NUM_SOURCES, &pbSource );
	while( numBuf )	//Delete buffer one by one
	{
		deleteBuffer(pbBuffers[bufferIndex]);
		numBuf--;
		bufferIndex++;
	}
}

uint32_t OpenALAudioStream::getPlayedTime()
{
	uint32_t time = 0;

	if ( isValid() )
	{
		ALint processedBuf;
		alGetSourcei(pbSource, AL_BUFFERS_PROCESSED, &processedBuf);	//Get the number of buffers processed (free to be used)

		//All the buffers were read, we are stopped, add the remainging bytes read to the counter
		if(( pbBuffers.size() == (ALuint)processedBuf ) && ( processedBuf > 0 ))
		{
			ALuint tmpBuffer;

			while(processedBuf)
			{
				unqueueBuffer(tmpBuffer);
				streamBaseOffset += pbBuffersDataSize.front();
				processedBuf--;
			}
		}

		//Every second, we consume 2Bytes (16bit) * NumChannel * freq (Hz)
		time = streamBaseOffset / ( freq / 1000 );

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
		if ( !decoder->hasDecodedFrames() ) //No decoded data available yet, delay upload
		{
			cout << "Oops, buffer underrun. Waiting for data." << endl;
			pause();
			return;
		}
		
		if(!filling)
		{
			filling = true;
			uint32_t ret = 0;
			bool errorFilling = false;
			ALint queuedBuf;
			ALint processedBuf;
			uint16_t maxNumBuffers;
			
			alGetSourcei(pbSource, AL_BUFFERS_QUEUED, &queuedBuf);	//Are the buffers associated with the source
			alGetSourcei(pbSource, AL_BUFFERS_PROCESSED, &processedBuf);	//Get the number of buffers processed (free to be used)
			ALint unqueuedBuf = pbBuffers.size() - queuedBuf;	//Number of buffers still not associated to the source
			uint32_t unqueuedIndex = pbBuffers.size() - unqueuedBuf;

			#ifdef DEBUG
			cout << processedBuf << " processedBuf, " << unqueuedBuf << " unqueuedBuf available. ";
			cout << "Total of buffers running: " << pbBuffers.size() << endl;
			#endif

			//Calculate maximum number of buffers so buffer MAX_BUFFER_TIME msec
			//Every second, we consume 2Bytes (16bit) * NumChannel * freq (Hz)
			maxNumBuffers = freq * MAX_BUFFER_TIME / 1000 / BUFFER_SIZE;
			if ( format == AL_FORMAT_MONO16 )
			{
				maxNumBuffers *= 2;	//multiplied by 2B/channel * 1 channel
			}
			else if ( format == AL_FORMAT_STEREO16 )
			{
				maxNumBuffers *= 4;	//Consuming twice as much in stereo
			}
			
			while(filling)	//filling all buffers one by one
			{
				//Let's buffer a max of MAX_BUFFER_TIME seconds, otherwise we create to many buffers
				if((pbBuffers.size() >= maxNumBuffers) && (processedBuf + unqueuedBuf == 0))	//No buffer available and won't create new ones
				{
					#ifdef DEBUG
					cout << "Maximum of buffers (" << maxNumBuffers;
					cout << ") reached and none free to be used, let's wait." << endl;
					#endif
					break;
				}
			
				ALuint tmpBuffer;
				createBuffer( tmpBuffer );
				ret = fillBuffer( &tmpBuffer, errorFilling );
				
				if( errorFilling || ( ret == 0 ) )
				{
					deleteBuffer( tmpBuffer );
					break;
				}
				
				pbBuffersDataSize.push_back( ret );//Storing the data size to be written in buffer. This is used to calculate played time
				
				#ifdef DEBUG
				if( ret != BUFFER_SIZE )
				{
					cout << "Oops, the buffer was not full: " << ret << " bytes." <<  endl;
				}
				#endif

				if(processedBuf > 0)	//Reuse free queued buffers
				{
					ALuint emptyBuffer;
					#ifdef DEBUG
					cout << "Filling a processedBuf." << endl;
					#endif

					unqueueBuffer( emptyBuffer );
//					updatePlayedTime();
					emptyBuffer = tmpBuffer;
					queueBuffer( emptyBuffer );

					//Update played bytes (since a buffer is popped once played
					//OpenAL uses FIFO buffering, so should we do for buffer data size
					streamBaseOffset += pbBuffersDataSize.front();
					pbBuffersDataSize.erase( pbBuffersDataSize.begin() );

					//Refresh values
					alGetSourcei(pbSource, AL_BUFFERS_PROCESSED, &processedBuf);	//Get the number of buffers processed (free to be used)
				}
				else if(unqueuedBuf > 0)	//Use buffers that are not queued in the source yet
				{
					#ifdef DEBUG
					cout << "Filling an unqueuedBuf." << endl;
					#endif

					pbBuffers[unqueuedIndex] = tmpBuffer;
					queueBuffer( pbBuffers[unqueuedIndex] );
					
					//Refresh values
					alGetSourcei(pbSource, AL_BUFFERS_QUEUED, &queuedBuf);	//Are the buffers associated with the source
					unqueuedBuf = pbBuffers.size() - queuedBuf;	//Number of buffers still not associated to the source
					unqueuedIndex = pbBuffers.size() - unqueuedBuf;
				}
				else
				{
					#ifdef DEBUG
					cout << "Filling a new unqueuedBuf." << endl;
					#endif

					pbBuffers.push_back( tmpBuffer );
					queueBuffer( pbBuffers.back() );

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

void OpenALAudioStream::getALState(ALint &state)
{
	alGetSourcei( pbSource, AL_SOURCE_STATE, &state );
}

/**
Returns the status according to the AL STATE and status. Just in case, resetting status
*/
STREAM_STATUS OpenALAudioStream::getStatus()
{
	if( ( status != DEAD ) || (status != STARTING) )
	{
		ALint state;
		getALState( state );

		if( state == AL_PLAYING )
		{
			status = PLAYING;
		}

		else if( state == AL_STOPPED )
		{
/*
			if( status == PLAYING )	//The source is stopped because no more buffer, but should be playing otherwise, how should it be treated???
			{
				status = PAUSED;
			}
*/
			status = STOPPED;
		}
		else
		{
			status = PAUSED;
		}
/*
		if(state == AL_PAUSED)
		{
			status = PAUSED;
		}

		if( state == AL_INITIAL )
		{
			status = READY;
		}
*/
	}
	#ifdef DEBUG
	cout << "The status is " << status << endl;
	#endif

	return status;
}

/**
Empty all buffers associated to the source and unqueue
WARNING, make sure you stopped the stream first by calling stop()
*/
void OpenALAudioStream::empty()
{
	ALint numBuf;
	ALuint tmpBuffer;

	alGetSourcei(pbSource, AL_BUFFERS_QUEUED, &numBuf);	//Get number of buffers queued attached

	while(numBuf)	//Remove buffers one by one
	{
	  unqueueBuffer(tmpBuffer);
	  numBuf--;
	}
}

void OpenALAudioStream::pause()
{
	alSourcePause( pbSource );
	setStatus( PAUSED );
}

void OpenALAudioStream::play()
{
	alSourcePlay( pbSource );
	setStatus( PLAYING );
}

void OpenALAudioStream::stop()
{
	alSourceStop( pbSource );
	setStatus( STOPPED );
}

//Check if the stream is paused
bool OpenALAudioStream::paused()
{
	if ( isValid() )
	{
		if ( getStatus() == PAUSED )
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
	if ( getStatus() == DEAD )
	{
		return false;
	}

	return true;
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

bool OpenALAudioStream::unqueueBuffer( ALuint &buffer )
{
	alSourceUnqueueBuffers(pbSource, 1, &buffer);
	if( checkALError("Error unqueuing buffer.") )
	{
		return false;
	}

//	//Update played bytes (since a buffer is popped once played
//	//OpenAL uses FIFO buffering, so should we do for buffer size
//	streamBaseOffset += pbBuffersDataSize.front();
//	pbBuffersDataSize.erase( pbBuffersDataSize.begin() );
	return true;
}

bool OpenALAudioStream::queueBuffer(ALuint &buffer)
{
	alSourceQueueBuffers(pbSource, 1, &buffer );
	checkALError("Error queuing new buffer.");

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
