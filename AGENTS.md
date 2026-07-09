# Agent Instructions

## Project Overview
- **Type**: Win32 Desktop Application (C++)
- **Build System**: Microsoft Visual Studio (MSBuild)
- **Language Standard**: C++20

## Development
- **Entry Point**: `wWinMain` in `wenv.cpp`
- **Resource Management**: Uses `.rc` files and `Resource.h` for UI elements and IDs.

## Build & Run
- Use Visual Studio or `msbuild` to build the project.
- Target platforms: Win32, x64.

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

- Keep code small, simple and straightforward
- Don't overengineer
- Prefer simpler constructs
