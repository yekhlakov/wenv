#pragma once

namespace Wenv
{

class Context;

namespace Apps
{
class App;
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