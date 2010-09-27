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

#include "swf.h"
#include "audio.h"
#include <iostream>
#include "../logger.h"

//Needed or not with compat.h and compat.cpp?
#if defined WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/types.h>
#endif

using namespace lightspark;

using namespace std;

extern TLSDATA SystemState* sys;

/****************
AudioManager::AudioManager
*****************
It should search for a list of audio plugin lib files (liblightsparkAUDIOAPIplugin.so)
Then, it should read a config file containing the user's defined audio API choosen as audio backend
If no file or none selected
  default to none
Else
  Select and load the good audio plugin lib files
*****************/

AudioManager::AudioManager( PluginManager *sharedPluginManager )
{
	pluginManager = sharedPluginManager;
	selectedAudioBackend = "";
	oAudioPlugin = NULL;
//	  string DesiredAudio = get_audioConfig(); //Looks for the audio selected in the user's config
	string desiredAudio = sys->config->getAudioBackendName();
	setAudioBackend( desiredAudio );
}

bool AudioManager::pluginLoaded() const
{
	return oAudioPlugin != NULL;
}

void AudioManager::freeStreamPlugin( AudioStream *audioStream )
{
	if ( pluginLoaded() )
	{
		oAudioPlugin->freeStream( audioStream );
	}
	else
	{
		LOG( LOG_ERROR, _( "No audio plugin loaded, can't free stream" ) );
	}
}

AudioStream *AudioManager::createStreamPlugin( AudioDecoder *decoder )
{
	if ( pluginLoaded() )
	{
		return oAudioPlugin->createStream( decoder );
	}
	else
	{
		LOG( LOG_ERROR, _( "No audio plugin loaded, can't create stream" ) );
		return NULL;
	}
}

void AudioManager::pauseStreamPlugin( AudioStream *audioStream )
{
	if ( pluginLoaded() )
	{
		oAudioPlugin->pauseStream( audioStream );
	}
	else
	{
		LOG( LOG_ERROR, _( "No audio plugin loaded, can't pause stream" ) );
	}

}

void AudioManager::playStreamPlugin( AudioStream *audioStream )
{
	if ( pluginLoaded() )
	{
		oAudioPlugin->playStream( audioStream );
	}
	else
	{
		LOG( LOG_ERROR, _( "No audio plugin loaded, can't play/resume stream" ) );
	}

}

void AudioManager::stopStreamPlugin( AudioStream *audioStream )
{
	if ( pluginLoaded() )
	{
		oAudioPlugin->stopStream( audioStream );
	}
	else
	{
		LOG( LOG_ERROR, _( "No audio plugin loaded, can't resume stream" ) );
	}

}

bool AudioManager::isTimingAvailablePlugin() const
{
	if ( pluginLoaded() )
	{
		return oAudioPlugin->isTimingAvailable();
	}
	else
	{
		LOG( LOG_ERROR, _( "isTimingAvailablePlugin: No audio plugin loaded" ) );
		return false;
	}
}

void AudioManager::setAudioBackend( string backend )
{
	if ( selectedAudioBackend != backend )  	//Load the desired backend only if it's not already loaded
	{
		loadAudioPlugin( backend );
		selectedAudioBackend = backend;
	}
}

void AudioManager::getAudioBackendsList()
{
	audioPluginsList = pluginManager->getBackendsList( AUDIO );
}

void AudioManager::refreshAudioPluginsList()
{
	audioPluginsList.clear();
	getAudioBackendsList();
}

void AudioManager::releaseAudioPlugin()
{
	if ( pluginLoaded() )
	{
		pluginManager->releasePlugin( oAudioPlugin );
	}
}

void AudioManager::loadAudioPlugin( string backend )
{
	LOG( LOG_NO_INFO, _((( string )( "the selected backend is: " + backend ) ).c_str() ) );
	releaseAudioPlugin();
	oAudioPlugin = static_cast<IAudioPlugin *>( pluginManager->getPlugin( backend ) );

	if ( !pluginLoaded() )
	{
		LOG( LOG_NO_INFO, _( "Could not load the audiobackend" ) );
	}
}

/**************************
stop AudioManager
***************************/
AudioManager::~AudioManager()
{
	releaseAudioPlugin();
	pluginManager = NULL;	//The plugin manager is not deleted since it's been created outside of the audio manager
}
