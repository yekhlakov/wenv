# Agent Instructions

## Project Description

A text-based working environment (file manager / viewer etc) inspired by classic text mode commanders of old.

## Project Overview
- **Type**: Win32 Desktop Application (C++)
- **Build System**: Microsoft Visual Studio (MSBuild)
- **Language Standard**: C++20

## Project Structure

. 		-- root directory, contains several source files and auxiliary data
├─/apps		-- application fragments are here
│ ├─/FileEditor	
│ └─/FileManager
├─/display	-- window management and displaying of information
├─/layout	-- abstract-ish handling of display layouts (grid-based layout design)
├─/maxy		-- imported (library) components; not a part of this project; maintained elsewhere; must not modify these
└─/tmp		-- use this for temporary storage instead of system directories


## Development
- **Entry Point**: `wWinMain` in `wenv.cpp`
- **Resource Management**: Uses `.rc` files and `Resource.h` for UI elements and IDs.

## Build & Run
- Use `make.bat` as a quick build tool for smoke testing.
- Use Visual Studio or `msbuild` to build the project.
- Target platforms: Win32, x64.
- This is a Win32 windowed application (despite being "text-based": the text is drawn on the window, not printed to a console).

## Code style
- Use Allman-like code style
- Use single tabs for indentation
- Put a single space between function name (or a keyword) and opening parenthesis when they are on the same line
- Never use `using namespace std`, specify namespace explicitly instead: `std::string`

Code style example:

```cpp
#include <cmath>
#include <string>
#include "project.h"

void processAgentData (int id)
{
	if (id > 0)
	{
		startAgentTask ();
	}
	else
	{
		terminateAgent ();
	}
}

```

## General rules

- Keep code small, simple and straightforward.
- Don't overengineer.
- Prefer simpler constructs.
- Avoid repetition and boilerplate.
- Comment the code you produce. Comment the resulting state of the code, not the nature of the changes.

