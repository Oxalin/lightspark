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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "compat.h"
#include <iostream>
#include <vector>

#include "interfaces/IPlugin.h"

//convenience typedef for the pointers to the 2 functions we expect to find in the plugin libraries
typedef IPlugin * ( *PLUGIN_FACTORY ) ();
typedef void ( *PLUGIN_CLEANUP ) ( IPlugin * );

namespace lightspark
{

class PluginModule;

class PluginManager
{
private:
	std::vector<PluginModule *> pluginsList;
	void findPlugins();
	void addPluginToList ( IPlugin *oPlugin, std::string pathToPlugin );
	void removePluginFromList ( std::string pluginPath );
	int32_t findPluginInList ( std::string desiredName = "", std::string desiredBackend = "", std::string desiredPath = "",
	                           HMODULE hdesiredLoadPlugin = NULL, IPlugin *oDesiredPlugin = NULL );
	void loadPlugin ( uint32_t desiredIndex );
	void unloadPlugin ( uint32_t desiredIndex );

public:
	PluginManager();
	std::vector<std::string *> getBackendsList ( PLUGIN_TYPES typeSearched );
	IPlugin *getPlugin ( std::string desiredBackend );
	void releasePlugin ( IPlugin *oPlugin );
	~PluginManager();
};

class PluginModule
{
	friend class PluginManager;
protected:
	std::string pluginName;		//plugin name
	PLUGIN_TYPES pluginType;	//plugin type to be able to filter them
	std::string backendName;	//backend (can be something like pulseaudio, opengl, ffmpeg)
	std::string pluginPath;		//full path to the plugin file
	bool enabled;		//should it be enabled (if the audio backend is present)?
	HMODULE hLoadedPlugin;	//when loaded, handle to the plugin so we can unload it later
	IPlugin *oLoadedPlugin;	//when instanciated, object to the class

public:
	PluginModule();
	~PluginModule();
};

}

#endif
