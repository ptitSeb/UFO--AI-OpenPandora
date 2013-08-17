/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_ENVIRONMENT_H)
#define INCLUDED_ENVIRONMENT_H

#include <string>

namespace {
	const std::string RKEY_APP_PATH = "user/paths/appPath";
	const std::string RKEY_HOME_PATH = "user/paths/homePath";
	const std::string RKEY_SETTINGS_PATH = "user/paths/settingsPath";
	const std::string RKEY_BITMAPS_PATH = "user/paths/bitmapsPath";
	const std::string RKEY_ENGINE_PATH = "user/paths/enginePath";
	const std::string RKEY_MAP_PATH = "user/paths/mapPath";
	const std::string RKEY_PREFAB_PATH = "user/paths/prefabPath";
}

/** greebo: A base class initialised right at the startup holding
 * 			information about the home and application paths.
 */
class Environment
{
	// The app + home paths
	std::string _appPath;
	std::string _homePath;
	std::string _settingsPath;
	std::string _bitmapsPath;
	std::string _mapsPath;

	int _argc;
	char** _argv;

public:
	// Call this with the arguments from main()
	void init(int argc, char* argv[]);

	int getArgc() const;

	std::string getArgv(unsigned int index) const;

	/** greebo: Get the application/home paths
	 */
	std::string getHomePath();
	std::string getAppPath();
	std::string getSettingsPath();
	std::string getBitmapsPath();

	/** greebo: Creates the runtime-keys for the
	 * 			paths (home path, etc.)
	 */
	void savePathsToRegistry();

	/** greebo: Clears the paths from the registry to avoid
	 * 			bogus paths from being saved and loaded at next startup.
	 */
	void deletePathsFromRegistry();

	// Get/set the path where the .map files are stored
	const std::string& getMapsPath() const;
	void setMapsPath(const std::string& path);

	// Contains the static instance
	static Environment& Instance();

private:
	std::string getHomeDir ();

	// Sets up the bitmap path and settings path
	void initPaths();

	// Initialises the arguments
	void initArgs(int argc, char* argv[]);

	Environment();
};

#endif
