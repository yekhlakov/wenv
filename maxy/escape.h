#pragma once
#ifndef MAXY_STRINGS_ESCAPE
#define MAXY_STRINGS_ESCAPE

#include <sstream>
#include <iostream>

namespace maxy
{
	namespace strings
	{
		// Escape a string and output it into a stream
		std::ostream & output_escaped (std::ostream & os, const std::string & s);
		// Unescape a string and output it into a stream
		std::ostream & output_unescaped (std::ostream & os, const std::string & s);
		// Escape a string
		std::string escape (const std::string & in);
		// Unescape a string
		std::string unescape (const std::string & in);
	}
}

#endif // MAXY_STRINGS_ESCAPE
