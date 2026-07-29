#pragma once

namespace Wenv
{

namespace Apps
{
class App;
class Context;
}

namespace Layout
{

// Declared elsewhere
struct Layout;
struct Grid;
struct Block;
}

namespace Display
{

// Declared elsewhere
struct Display;
struct Palette;

struct Pos
{
	int x;
	int y;
};

struct Rect
{
	int x;
	int y;
	int width;
	int height;
};

}


}