#include <algorithm>
#include <cctype>
#include <string>
#include <sstream>
#include <locale.h>

#define _MAXY_STRINGS_IMPL_
#include "strings.h"

namespace maxy
{
	namespace strings
	{
		/**
		 * Output a (utf8 encoded) unicode character to a stream
		 */
		void emit_char (std::ostream &os, int c)
		{
			if (c < 128)
				os << (char) c;
			else if (c < 0x800)
				// two bytes 110yyyyy	10xxxxxx
				// yyy yyxxxxxx
				os << (char) (((c & 0x7c0) >> 6) | 0xc0)
				   << (char) (((c & 0x03f)) | 0x80);
			else // if (c < 0x10000)
				 // three bytes 1110zzzz	10yyyyyy	10xxxxxx
				 // zzzzyyyy yyxxxxxx
				os << (char) (((c & 0xf000) >> 12) | 0xe0)
				   << (char) (((c & 0x0fc0) >> 6) | 0x80)
				   << (char) (((c & 0x003f)) | 0x80);
		}

		/**
		 * Convert utf8 encoded char string into wchar_t string
		 */
		std::wstring utf8towchar (const std::string & in)
		{
			std::wostringstream os;

			for (size_t i = 0; i < in.size (); i++)
			{
				char p = in[i];
				wchar_t o;
				int size = 0;

				if (p >= 0 && p < 128)
					o = p;
				else if ((p & 0xe0) == 0xc0) // first byte of 2-byte sequence
					size = 1;
				else if ((p & 0xf0) == 0xe0) // first byte of 3-byte sequence
					size = 2;
				else if ((p & 0xf8) == 0xf0) // first byte of 4-byte sequence
					size = 3;

				if (size)
				{
					o = p & (0x7f >> size);
					while (size--)
					{
						p = in[++i];
						if ((p & 0xc0) != 0x80)
						{
							o = 0;
							break;
						}
						o = (o << 6) | (p & 0x3f);
					}
				}

				if (o) os << o;
			}

			return os.str ();
		}

		/**
		 * Convert wchar_t string to utf8 encoded char string
		 */
		std::string wchartoutf8 (const std::wstring & in)
		{
			std::ostringstream os;
			for (auto c : in) emit_char (os, c);
			return os.str ();
		}

		/**
		 * Split string in two with = as separator
		 */
		std::pair<std::string, std::string> eq_split (const std::string & in, char sep)
		{
			auto ptr = in.find_first_of (sep);
			if (ptr == std::string::npos) return std::make_pair<> (in, std::string{});

			return std::make_pair<> 
			(
				std::string{in.cbegin (),in.cbegin () + ptr}, 
				std::string{in.cbegin () + ptr + 1, in.cend ()}
			);
		}

		/**
		 * Check if the string contains integer
		 */
		bool is_int (const std::string & s)
		{
			bool has_sign = s[0] == '+' || s[0] == '-';
			for (char c : s)
			{
				if ((c == '+' || c == '-') && has_sign) return false;
				if (c < '0' || c > '9') return false;
			}
			return true;
		}

		/**
		 * Check if the string contains float
		 */
		bool is_float (const std::string & s)
		{
			bool has_sign = s[0] == '+' || s[0] == '-',
				has_point = false, has_exp = false, has_exp_sign = false;

			size_t p = has_sign ? 1 : 0;

			if (p >= s.size ()) return false;

			for (; p < s.size (); p++)
			{
				char c = s[p];
				if ((c == '+' || c == '-') && has_sign) return false;
				if (c == '.')
				{
					if (has_point) return false;
					has_point = true;
				}
				else if (c == 'e' || c == 'E')
				{
					has_exp = true;
					break;
				}
				else if (c < '0' || c > '9')
				{
					return false;
				}
			}

			if (!has_exp) return true;

			p++;

			if (p >= s.size ()) return false;

			if (s[p] == '+' || s[p] == '-') has_exp_sign = true;
			p++;
			if (p >= s.size ()) return false;

			for (; p < s.size (); p++)
			{
				if (s[p] < '0' || s[p] > '9') return false;
			}
			return true;
		}

		// Check if the string is a representation of a number
		bool is_numeric (const std::string & s) 
		{ 
			return is_int (s) || is_float (s); 
		}

		// Trimming template
		template<class C>
		std::basic_string<C> ttrim (const std::basic_string<C> & s, const std::basic_string<C> & characters)
		{
			if (!characters.size ())
			{
				// Nothing to trim
				return s;
			}

			auto pfx = s.find_first_not_of (characters);

			if (pfx == std::string::npos)
			{
				// Nothing to trim
				return {};
			}

			auto sfx = s.find_last_not_of (characters);

			if (sfx == std::string::npos)
			{
				return s.substr (pfx);
			}

			return s.substr (pfx, sfx - pfx + 1);
		};

		// Trimming
		std::string trim (const std::string & s, const std::string & characters)
		{
			return ttrim (s, characters);
		}
		std::wstring trim (const std::wstring & s, const std::wstring & characters)
		{
			return ttrim (s, characters);
		}

		// Convert a string to lowercase
		std::string tolower (const std::string & s)
		{
			std::string o = s;
			std::transform (o.begin (), o.end (), o.begin (), [] (int c) { return std::tolower (c); });

			return o;
		}

		// Convert a string to upper case
		std::string toupper (const std::string & s)
		{
			std::string o = s;
			std::transform (o.begin (), o.end (), o.begin (), [] (int c) { return std::toupper (c); });

			return o;
		}

		// Convert a wstring to lowercase
		std::wstring tolower (const std::wstring & s)
		{
			std::wstring o = s;
			auto & f = std::use_facet<std::ctype<wchar_t>> (std::locale ());
			std::transform (o.begin (), o.end (), o.begin (), [&] (int c) { return f.tolower (c); });

			return o;
		}

		// Convert a wstring to upper case
		std::wstring toupper (const std::wstring & s)
		{
			std::wstring o = s;
			auto & f = std::use_facet<std::ctype<wchar_t>> (std::locale ());
			std::transform (o.begin (), o.end (), o.begin (), [&] (int c) { return f.toupper (c); });

			return o;
		}
	}
}
