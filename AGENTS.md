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

Consult this structure BEFORE searching (grepping/globbing) the codebase: it usually tells you exactly which file(s) to look at.
When your changes alter the code structure (new/moved/removed files or directories), update this description accordingly in the same change.

. 		-- root directory, contains the core source files, project files and auxiliary data
│ ├─wenv.cpp / wenv.h	-- application entry point (`wWinMain`) and core wiring
│ ├─Context.h		-- `Context` class: named application settings, persisted to/from json
│ ├─Types.h		-- shared basic types (`Pos`, `Rect` etc) and forward declarations
│ ├─framework.h, targetver.h -- Win32 boilerplate headers
│ ├─Resource.h, wenv.rc, wenv.ico -- resources (icons, version info etc)
│ ├─app.manifest	-- application manifest
│ ├─wenv.vcxproj (+ .filters, .user) -- MSBuild project files
│ ├─make.bat		-- quick build script for smoke testing
│ ├─cache.json		-- runtime state/settings cache written by the app
│ ├─opencode.json	-- opencode agent configuration
│ └─test.exe		-- built binary (build output, not source)
├─/apps		-- application fragments ("apps" shown inside windows) are here; App.h/App.cpp define the base App class
│ ├─/FileEditor	-- The text file viewer/editor app collection (File file wrapper, the editor, its status bar)
│ ├─/FileManager -- The file manager app collection (file list, its header, short file info panel)
│ ├─/FuncMenu	-- The menu of functional keys (to be displayed in the bottom of the screen)
│ └─/Modal	-- The modal box components (title, text, buttons)
├─/display	-- window management and displaying of information; Display (core + Output/Input/Init parts), Window (core + Init/Input parts), Modal, Character, Palette
├─/layout	-- abstract-ish handling of display layouts (grid-based layout design): Grid, Layout
├─/maxy		-- imported (library) components: json, strings, escape, control (container/events); not a part of this project; maintained elsewhere; must not modify these; ignore compilation warnings here.
├─/wenv		-- intermediate build output (object files); not source
├─/x64		-- build output (executables); not source
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
