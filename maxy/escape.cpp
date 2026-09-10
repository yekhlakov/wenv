#include "strings.h"
#include "escape.h"
#include <map>

/**
 * Escape
 */
namespace maxy
{
	namespace strings
	{
		/**
		* Escape a string and output it into a stream
		*/
		std::ostream & output_escaped (std::ostream & os, const std::string & s)
		{
			static std::map<char, const char *> mapping = {
				{'\t', "\\t"},
				{'\n', "\\n"},
				{'\r', "\\r"},
				{'\b', "\\b"},
				{'\f', "\\f"},
				{'\\', "\\\\"},
				{'"', "\\\""},

			};

			for (size_t p = 0; p < s.size (); p++)
			{
				auto x = mapping.find (s[p]);
				if (x != mapping.end ())
					os << x->second;
				else os << s[p];
			}

			return os;
		}

		/**
		 * Unescape a string and output it into a stream
		 */
		std::ostream & output_unescaped (std::ostream & os, const std::string & s)
		{
			static std::map<char, const char *> mapping = {
				{'t', "\t"},
				{'n', "\n"},
				{'r', "\r"},
				{'b', "\b"},
				{'f', "\f"},
				{'\\', "\\"},
				{'/', "/"},
				{'"', "\""},
			};

			for (size_t p = 0; p < s.size (); p++)
			{
				if (s[p] == '\\' && p < s.size() - 1)
				{
					auto next = s[p + 1];

					if (next == 'x' && p < s.size() - 3)
					{
						// \xFF - single unicode byte
						std::string hex = "0x";
						hex += s[p + 2];
						hex += s[p + 3];
						emit_char (os, std::stol (hex, nullptr, 0));
						p += 3;
						continue;
					}
					
					if (next == 'u' && p < s.size () - 5)
					{
						// \uFFFF - two unicode bytes
						std::string hex = "0x";
						for (size_t offset = 2; offset < 6; offset++)
						{
							hex += s[p + offset];
						}
						emit_char (os, std::stol (hex, nullptr, 0));
						p += 5;
						continue;
					}

					auto x = mapping.find (next);
					if (x != mapping.end ())
					{
						os << x->second;
					}
					p++;
				}
				else
				{
					os << s[p];
				}
			}

			return os;
		}

		/**
		 * Escape a given string (add slashes)
		 */
		std::string escape (const std::string & in)
		{
			std::ostringstream os;
			output_escaped (os, in);
			return os.str ();
		}

		/**
		 * Unescape a given string (remove slashes)
		 */
		std::string unescape (const std::string & in)
		{
			std::ostringstream os;
			output_unescaped (os, in);
			auto s = os.str ();;
			return s;
		}
	}
}
