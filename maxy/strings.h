#pragma once
#ifndef MAXY_STRINGS
#define MAXY_STRINGS
#include <string>
#include <sstream>

namespace maxy
{
	namespace strings
	{
		// Convert utf8 char string to wchar_t string
		std::wstring utf8towchar (const std::string & in);
		// Convert wchar_t string to utf8 char string
		std::string	wchartoutf8 (const std::wstring & in);
		// encode unicode character into utf8 and emit it to a stream
		void emit_char (std::ostream &os, int c);
		// Split a string in two using '=' as separator
		std::pair<std::string, std::string> eq_split (const std::string & in, char sep = '=');

		// Check if the string is a representation of an integer
		bool is_int (const std::string & s);
		// Check if the sting is a representation of a float
		bool is_float (const std::string & s);
		// Check if the string is a representation of a number
		bool is_numeric (const std::string & s);
		// Trim a string (remove its prefix (and suffix) consisting of the characters from the given set)
#ifndef _MAXY_STRINGS_IMPL_
		extern std::basic_string<char> ttrim (const std::basic_string<char> & s, const std::basic_string<char> & characters);
		extern std::basic_string<wchar_t> ttrim (const std::basic_string<wchar_t> & s, const std::basic_string<wchar_t> & characters);
#endif
		// Functions to trim various string types (with default trimmable character sets)
		std::string trim (const std::string & s, const std::string & characters = " \t\n\r\b");
		std::wstring trim (const std::wstring & s, const std::wstring & characters = L" \t\n\r\b");

		// Convert a string to lowercase
		std::string tolower (const std::string & s);
		std::wstring tolower (const std::wstring & s);
		// Convert a wstring to upper case
		std::string toupper (const std::string & s);
		std::wstring toupper (const std::wstring & s);
	}
}

#endif //MAXY_STRINGS