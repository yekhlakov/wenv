# Agent Instructions

## Project Description

A text-based working environment (file manager / viewer etc) inspired by classic text mode commanders of old.

## Project Overview
- **Type**: Win32 Desktop Application (C++)
- **Build System**: Microsoft Visual Studio (MSBuild)
- **Language Standard**: C++20
- **Source file encoding**: Utf-8 (preferred)
- **Defautl console**: cmd.exe, default encoding in console is cp866

## Project Structure

. 		-- root directory, contains several source files and auxiliary data
├─/apps		-- application fragments are here
│ ├─/FileEditor	-- The text file viewer/editor app collection (status bar + the editor)
│ ├─/FileManager -- The file manager app collection (status bar(s), file list)
│ ├─/FuncMenu	-- The menu of functional keys (to be displayed in the bottom of the screen)
│ └─/Modal	-- The modal box apps (title, text, button set etc)
├─/display	-- window management and displaying of information
├─/layout	-- abstract-ish handling of display layouts (grid-based layout design)
├─/maxy		-- imported (library) components; not a part of this project; maintained elsewhere; must not modify these; ignore compilation warnings here.
└─/tmp		-- use this for temporary storage instead of system directories


## Development
- **Entry Point**: `wWinMain` in `wenv.cpp`
- **Resource Management**: Uses `.rc` files and `Resource.h` for UI elements and IDs.

## Build & Run
- Use `make.bat` as a quick build tool for smoke testing.
- Use Visual Studio or `msbuild` to build the project.
- Target platforms: Win32, x64.
- This is a Win32 windowed application (despite being "text-based": the text is drawn on the graphics window, not printed to a console).

## Code style
- Use Allman-like code style
- Use single tabs for indentation
- Text files must have newline at the end
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

- Check the tasks the user gives for controversies. If the task cannot be done (or obviously has been done previously), stop and complain instead of trying to solve the unsolvable.
- Keep code small, simple and straightforward.
- Prefer simpler constructs.
- Extract similar code fragments into separate method(s) to reuse.
- Comment the code you produce. Comment the resulting state of the code, not the nature of the changes.
- Use `chcp 65001` in bat scripts to switch shell encoding to utf-8 so that warnings don't appear broken

## Negative

- Don't overengineer.
- Avoid repetition and boilerplate.
