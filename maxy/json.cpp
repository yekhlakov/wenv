#include "json.h"
#include "strings.h"
#include "escape.h"

namespace maxy
{
	namespace data
	{
		/**
		 * Read a string from input stream
		 * A string is either a regular double-quoted string, or an unquoted token
		 * Return the parsed json string and the next character
		 */
		std::pair<json, char> json::read_string (std::istream & is, char first)
		{
			std::ostringstream os;

			// if the first character is not a quote, it is part of the string
			if (first != '"') os << first;

			char c;
			while (true)
			{
				// read next char
				c = is.get ();
				if (is.eof ()) break;

				if (first == '"')
				{
					// if the string is quoted - check for escaped quotes and ignore them
					if (c == '\\')
					{
						char cc = is.get ();
						if (is.eof ()) break;
						if (cc == '"') { os << cc; continue; }
						os << c << cc;
						continue;
					}
					else if (c == '"')
					{
						// if we found a double quote and it is not escaped - it is the end
						// so skip any following whitespace and return
						c = skip_whitespace (is);
						return std::make_pair (json{maxy::strings::unescape (os.str ())}, c);
					}
				}
				else
				{
					// the unquoted string is terminated by whitespace
					// or by any of terminating characters (see below)
					if (c == ' ' || c == 9 || c == 10 || c == 13)
					{
						c = skip_whitespace (is);
						return std::make_pair (json{maxy::strings::unescape (os.str ())}, c);
					}
					else if (c == ':' || c == ',' || c == '}' || c == ']')
					{
						return std::make_pair (json{maxy::strings::unescape (os.str ())}, c);
					}
				}
				// add character to our string
				os.put (c);
			}

			// we exit the loop only if eos is reached, so next char is 0
			return std::make_pair (json{maxy::strings::unescape(os.str ())}, char{0});
		}

		/**
		 * Read a token from an input stream
		 * A token is an unquoted string containing only regular chars
		 */
		std::pair<json, char> json::read_token (std::istream & is, char first)
		{
			std::ostringstream os;
			os << first;
			char c;
			while (true)
			{
				c = is.get ();
				// loop until we find any of terminating characters
				if (c == ' ' ||
					c == 9 ||
					c == 10 ||
					c == 13 ||
					c == ',' ||
					c == ']' ||
					c == '}' ||
					is.eof ()) break;
				os << c;
			}

			if (is.eof ()) c = char{0};

			// return a json converted from the found token (probably invalid)
			return std::make_pair (from_token (os.str ()), c);
		}

		/**
		 * Discard characters from a stream until any non-whitespace char is found
		 * Return this char
		 */
		char json::skip_whitespace (std::istream & is)
		{
			char c;
			while (true)
			{
				c = is.get ();
				if (is.eof ()) return 0;
				if (!(c == 32 || c == 9 || c == 10 || c == 13))
					return c;
			}
		}

		/**
		 * Create a json from a token such as null, false etc.
		 */
		json json::from_token (const std::string s)
		{
			if (s == "null") return json{nullptr};
			if (s == "true") return json{true};
			if (s == "false") return json{false};
			if (maxy::strings::is_int (s)) return json{std::stoll (s)};
			if (maxy::strings::is_float (s)) return json{std::stold (s)};

			// otherwise it is a parse error
			return make_error ();
		}

		// Read the object
		// first is always '{' here
		std::pair<json, char> json::read_object (std::istream & is, char first)
		{
			auto rv = std::make_pair (make_object (), char {0});

			// Skip the whitespace between opening { and the first key
			char cx = skip_whitespace (is);

			// read all key:value pairs
			while (true)
			{
				rv.second = cx;
				if (!cx || is.eof () || cx == '}')
				{
					rv.second = ',';
					break;
				}

				// skip commas and the following whitespace entirely
				if (cx == ',' || cx == ' ' || cx == 9 || cx == 10 || cx == 13)
				{
					cx = skip_whitespace (is);
					continue;
				}

				// read the key
				auto s = read_string (is, cx);

				// propagate errors
				if (s.first.error != json_error::None)
					rv.first.error = s.first.error;

				// a string must be followed by a semicolon
				if (s.second != ':')
				{
					rv.first.error = json_error::ParseError;
					return rv;
				}

				// read value following the :
				auto p = parse_value (is, 0);
				if (p.first.is_empty ()) break;

				// add key-value pair to our object
				rv.first[s.first.string_value] = p.first;

				// propagate error
				if (p.first.error != json_error::None)
					rv.first.error = p.first.error;

				// store the next character and loop
				cx = p.second;
			}
			return rv;
		}

		// Read the array
		std::pair<json, char> json::read_array (std::istream & is, char first)
		{
			auto rv = std::make_pair (make_array (), char {0});

			// read array elements
			while (true)
			{
				auto cx = skip_whitespace (is);
				rv.second = cx;
				if (is.eof () || cx == ']')
				{
					rv.second = ',';
					break;
				}

				// skip commas and the following whitespace entirely
				if (cx == ',' || cx == ' ' || cx == 9 || cx == 10 || cx == 13)
				{
					continue;
				}

				auto p = parse_value (is, cx);
				if (p.first.is_empty ()) break;

				rv.first.push_back (p.first);
				rv.second = p.second;
				if (p.second == ']')
				{
					// the array terminated while reading one of its elements
					rv.second = skip_whitespace (is);
					break;
				}
				if (!p.second || is.eof ()) break;
			}

			return rv;
		}

		/**
		 * Parse a string value into a json (probably recursively)
		 */
		std::pair<json, char> json::parse_value (std::istream & is, char first)
		{
			// get the first character of the token
			char c = first;

			// skip whitespace if needed
			if (!c) c = skip_whitespace (is);

			// if the first character is a string - return string
			if (c == '"')
				return read_string (is, c);

			// if the first character is one of the following - a token
			if (c == 'n'  // null
				|| c == 't'  // true
				|| c == 'f'  // false
				|| c == '+' // explicitly positive number
				|| c == '-' // negative number
				|| (c >= '0' && c <= '9') // positive number
				|| c == '.' // float with zero int part
				)
				return read_token (is, c);

			// if the first character is [, it is an array
			if (c == '[')
			{
				return read_array (is, c);
			}

			// if the first character is {, it is an object
			if (c == '{')
			{
				return read_object (is, c);
			}

			// closing brackets - the object or array have no more elements
			if (c == ']' || c == '}')
			{
				// empty value
				return std::make_pair (json{}, char{0});
			}

			// any other char is error
			return std::make_pair (make_error (), char{0});
		}

		/**
		 * Parse a string into a json object
		 */
		json json::parse (const std::string & s)
		{
			std::istringstream is{s};
			return parse_value (is, 0).first;
		}
		json json::parse (std::istream & is)
		{
			return parse_value (is, 0).first;
		}

		// Default constructor
		json::json () : type {json_type::Empty}, error {json_error::None} {}
		// Construct a json string from a string
		json::json (const std::string & s) : type {json_type::String}, string_value {s}, error {json_error::None} {}
		json::json (std::string && s) : type {json_type::String}, string_value {s}, error {json_error::None} {}
		// Construct from a nullptr
		json::json (std::nullptr_t) : type {json_type::Null}, error {json_error::None} {};
		// Construct a json int from an int
		json::json (int n) : type {json_type::NumberInt}, error {json_error::None} { int_value = n; }
		// Construct a json int from a long long
		json::json (long long n) : type {json_type::NumberInt}, error {json_error::None} { int_value = n; }
		// Construct a json float from a long double
		json::json (long double n) : type {json_type::NumberFloat}, error {json_error::None} { float_value = n; }
		// Construct a json bool from a bool
		json::json (bool n) : error {json_error::None} { if (n) { type = json_type::True; } else { type = json_type::False; } }
		// Construct a json string from a char ptr
		json::json (const char * const p) : error {json_error::None}
		{
			if (p) { type = json_type::String; string_value = p; }
			else { type = json_type::Null; }
		}
		// Construct a json object from initilalizer list containing key, value pairs
		json::json (std::initializer_list<std::pair<const char *, json>> list)
		{
			type = json_type::Object;

			for (auto & el : list)
			{
				object_elements[el.first] = el.second;
			}
		}

		/**
		 * Conversion to bool
		 */
		json::operator bool () const
		{
			return !(type == json_type::Empty
					 || type == json_type::Null
					 || type == json_type::False
					 || (type == json_type::NumberInt && !int_value)
					 || (type == json_type::NumberFloat && float_value == 0.)
					 || (type == json_type::String && !string_value.size ())
					 || (type == json_type::Array && !array_elements.size ())
					 || (type == json_type::Object && !object_elements.size ())
					 );
		}

		// Conversion to int
		json::operator int () const {
			if (type == json_type::NumberFloat)
				return static_cast<int>(float_value);

			if (type == json_type::NumberInt)
				return static_cast<int>(int_value);

			return 0;
		}

		// Conversion to long
		json::operator long () const
		{
			if (type == json_type::NumberFloat)
				return static_cast<long>(float_value);

			if (type == json_type::NumberInt)
				return static_cast<long>(int_value);

			return 0;
		}

		// Conversion to long long
		json::operator long long () const
		{
			if (type == json_type::NumberFloat)
				return static_cast<long long>(float_value);

			if (type == json_type::NumberInt)
				return int_value;

			return 0;
		}

		// Conversion to float
		json::operator float () const
		{
			if (type == json_type::NumberFloat)
				return static_cast<float>(float_value);

			if (type == json_type::NumberInt)
				return static_cast<float>(int_value);

			return 0.f;
		}

		// Conversion to double
		json::operator double () const
		{
			if (type == json_type::NumberFloat)
				return static_cast<double>(float_value);

			if (type == json_type::NumberInt)
				return static_cast<double>(int_value);

			return 0.;
		}

		// Conversion to long double
		json::operator long double () const
		{
			if (type == json_type::NumberFloat)
				return float_value;

			if (type == json_type::NumberInt)
				return static_cast<long double>(int_value);

			return 0.;
		}

		// Conversion to string
		json::operator const std::string () const { return string_value; }

		/**
		* Equalities
		*/
		bool json::operator== (long long other) const noexcept
		{
			if (type == json_type::NumberInt) return other == int_value;
			else if (type == json_type::NumberFloat) return static_cast<long double>(other) == float_value;

			return false;
		}
		bool json::operator== (long double other) const noexcept
		{
			if (type == json_type::NumberInt) return static_cast<long long>(other) == int_value;
			else if (type == json_type::NumberFloat) return other == float_value;

			return false;
		}
		bool json::operator== (const std::string & other) const noexcept
		{
			if (type != json_type::String) return false;
			return other == string_value;
		}

		/**
		 * Whether a json is equal to another json
		 */
		bool json::operator== (const json & other) const noexcept
		{
			// it is the same object - true
			if (&other == this) return true;

			// different types - false
			if (other.type != type) return false;

			// per-type checks
			if (type == json_type::String)
					return other.string_value == string_value;

			if (type == json_type::NumberInt)
					return other.int_value == int_value;

			if (type == json_type::NumberFloat)
					return other.float_value == float_value;

			if (type == json_type::Array)
			{
				if (other.array_elements.size () != array_elements.size ()) return false;
				std::vector<json>::const_iterator i1 = other.array_elements.begin ();
				std::vector<json>::const_iterator i2 = array_elements.begin ();
				for (;
					 i1 != other.array_elements.end() &&
					 i2 != array_elements.end();
					 ++i1, ++i2
					)
				{
					if (*i1 != *i2) return false;
				}
				return true;
			}

			if (type == json_type::Object)
			{
				if (other.object_elements.size () != object_elements.size ()) return false;
				std::map<std::string, json>::const_iterator i1 = other.object_elements.begin ();
				std::map<std::string, json>::const_iterator i2 = object_elements.begin ();
				for (;
				    i1 != other.object_elements.end () &&
					i2 != object_elements.end ();
					++i1, ++i2
					)
				{
					if (i1->first != i2->first) return false;
					if (i1->second != i2->second) return false;
				}
				return true;
			}
			return true;
		}
		bool json::operator== (const char * const p) const noexcept
		{
			if (p == nullptr) return (type == json_type::Null ? true : false);
			if (type != json_type::String) return false;

			return string_value == p;
		}

		/**
		 *  Assignments
		 */
		json & json::operator= (int n)
		{
			type = json_type::NumberInt;
			int_value = n;
			clear ();
			return *this;
		}
		json & json::operator= (long n)
		{
			type = json_type::NumberInt;
			int_value = n;
			clear ();
			return *this;
		};
		json & json::operator= (long long n)
		{
			type = json_type::NumberInt;
			int_value = n;
			clear ();
			return *this;
		}
		json & json::operator= (float n)
		{
			type = json_type::NumberFloat;
			float_value = n;
			clear ();
			return *this;
		}
		json & json::operator= (double n)
		{
			type = json_type::NumberFloat;
			float_value = n;
			clear ();
			return *this;
		}
		json & json::operator= (long double n)
		{
			type = json_type::NumberFloat;
			float_value = n;
			clear ();
			return *this;
		}
		json & json::operator= (const std::string & n)
		{
			type = json_type::String;
			string_value = n;
			clear ();
			return *this;
		}
		json & json::operator= (const char * const p)
		{
			if (p == nullptr)
			{
				type = json_type::Null;
			}
			else
			{
				type = json_type::String;
				string_value = p;
			}
			clear ();
			return *this;
		}
		json & json::operator= (bool n)
		{
			type = n ? json_type::True : json_type::False;
			clear ();
			return *this;
		};

		/**
		 *  Element access for container types
		 */
		json & json::operator[] (const char * const c)
		{
			type = json_type::Object;
			array_elements.clear ();
			return (*this)[std::string{c}];
		}
		json & json::operator[] (const std::string & key)
		{
			type = json_type::Object;
			array_elements.clear ();
			return object_elements[key];
		}
		json & json::operator[] (size_t n)
		{
			type = json_type::Array;
			object_elements.clear ();

			if (n + 1 > array_elements.size ())
			{
				for (size_t i = array_elements.size (); i < n + 1; i++)
					array_elements.push_back (json{});
			}
			return array_elements[n];
		}

		const json empty_json {};

		const json & json::operator[] (const char * const c) const
		{
			return (*this)[std::string {c}];
		}

		const json & json::operator[] (const std::string & key) const
		{
			if (type != json_type::Object)
			{
				return empty_json;
			}

			auto e = object_elements.find (key);
			if (e == object_elements.end ())
			{
				return empty_json;
			}

			return e->second;
		}

		const json & json::operator[] (size_t n) const
		{
			if (type != json_type::Array)
			{
				return empty_json;
			}

			if (n + 1 > array_elements.size ())
			{
				return empty_json;
			}

			return array_elements[n];
		}

		/**
		 * Array operations
		 */
		json & json::push_back (json j)
		{
			type = json_type::Array;
			object_elements.clear ();
			array_elements.push_back (j);
			return *this;
		}
		json json::pop_back ()
		{
			type = json_type::Array;
			object_elements.clear ();
			if (!array_elements.size ()) return make_error (json_error::OutOfRange);
			json j = std::move(array_elements[array_elements.size () - 1]);
			array_elements.pop_back ();
			return j;
		}

		/**
		 * Output an object to a stream
		 * The object is written in more or less strict format
		 */
		std::ostream & operator<< (std::ostream & os, const json & j)
		{
			auto jj = j.array_elements.cbegin ();
			auto xjj = j.object_elements.cbegin ();
			switch (j.type)
			{
				case json::json_type::True: return os << "true";
				case json::json_type::False: return os << "false";
				case json::json_type::String: 
					os << "\""; 
					maxy::strings::output_escaped (os, j.string_value);
					return os << "\"";

				case json::json_type::NumberInt: return os << j.int_value;
				case json::json_type::NumberFloat: return os << j.float_value;
				case json::json_type::Array:
					os << "[";

					while (jj != j.array_elements.cend ())
					{
						os << *jj;
						++jj;
						if (jj != j.array_elements.cend ())
							os << ",";
						else break;
					}
					return os << "]";

				case json::json_type::Object:
					os << "{";
					while (xjj != j.object_elements.cend ())
					{
						os << "\"";
						maxy::strings::output_escaped (os, xjj->first);
						
						os << "\":" << xjj->second;
						++xjj;
						if (xjj != j.object_elements.cend ())
							os << ",";
					}
					return os << "}";
				case json::json_type::Null: default: return os << "null";
			}
		}

		// iterator construction
		json::iterator::iterator (json & j, size_t p) : 
			ref{j}, 
			ptr{p}, 
			initial_type{j.type}
		{
			if (ref.type == json_type::Object)
			{
				if (!p)
					object_iterator = ref.object_elements.begin ();
				else
					object_iterator = ref.object_elements.end ();
			}
		}

		// iterator preincrement
		json::iterator & json::iterator::operator++ ()
		{
			ptr++;
			if (ref.type == json_type::Object)
				++object_iterator;
			return *this;
		}
		// iterator predecrement
		json::iterator & json::iterator::operator-- ()
		{
			ptr--;
			if (ref.type == json_type::Object)
				--object_iterator;
			return *this;
		}
		// iterator postincrement
		json::iterator json::iterator::operator++ (int)
		{
			auto temp = *this;
			++(*this);
			return temp;
		}
		// iterator postdecrement
		json::iterator json::iterator::operator-- (int)
		{
			auto temp = *this;
			--(*this);
			return temp;
		}
		// equality
		bool json::iterator::operator== (json::iterator & other) const
		{
			if (&ref != &other.ref) return false;
			return ptr == other.ptr;
		}
		// dereferencing
		std::pair<const std::string, json &> json::iterator::operator* () const
		{
			if (initial_type != ref.type)
				throw json::iterator::invalidated{};

			switch (initial_type)
			{
				case json::json_type::Array:
					if (ptr >= ref.array_elements.size ())
						throw json::iterator::out_of_range{};
					return{std::string{}, ref.array_elements[ptr]};

				case json::json_type::Object:
					if (ptr >= ref.object_elements.size ())
						throw json::iterator::out_of_range{};
					return{object_iterator->first, object_iterator->second};

				default:
					if (ptr > 0) 
						throw json::iterator::out_of_range{};
					
					return{std::string{}, ref};
			}
		}


		// Const iterator
		// iterator construction
		json::const_iterator::const_iterator (const json & j, size_t p) :
			ref {j},
			ptr {p},
			initial_type {j.type}
		{
			if (ref.type == json_type::Object)
			{
				if (!p)
					object_iterator = ref.object_elements.begin ();
				else
					object_iterator = ref.object_elements.end ();
			}
		}

		// iterator preincrement
		json::const_iterator & json::const_iterator::operator++ ()
		{
			ptr++;
			if (ref.type == json_type::Object)
				++object_iterator;
			return *this;
		}
		// iterator predecrement
		json::const_iterator & json::const_iterator::operator-- ()
		{
			ptr--;
			if (ref.type == json_type::Object)
				--object_iterator;
			return *this;
		}
		// iterator postincrement
		json::const_iterator json::const_iterator::operator++ (int)
		{
			auto temp = *this;
			++(*this);
			return temp;
		}
		// iterator postdecrement
		json::const_iterator json::const_iterator::operator-- (int)
		{
			auto temp = *this;
			--(*this);
			return temp;
		}
		// equality
		bool json::const_iterator::operator== (json::const_iterator & other) const
		{
			if (&ref != &other.ref) return false;
			return ptr == other.ptr;
		}
		// dereferencing
		std::pair<const std::string, const json &> json::const_iterator::operator* () const
		{
			if (initial_type != ref.type)
				throw json::const_iterator::invalidated {};

			switch (initial_type)
			{
			case json::json_type::Array:
				if (ptr >= ref.array_elements.size ())
					throw json::const_iterator::out_of_range {};
				return{std::string{}, ref.array_elements[ptr]};

			case json::json_type::Object:
				if (ptr >= ref.object_elements.size ())
					throw json::const_iterator::out_of_range {};
				return{object_iterator->first, object_iterator->second};

			default:
				if (ptr > 0)
					throw json::const_iterator::out_of_range {};

				return{std::string{}, ref};
			}
		}

		// begin iterator
		json::const_iterator json::begin () const
		{
			return json::const_iterator (*this);
		}

		json::iterator json::begin ()
		{
			return json::iterator (*this);
		}

		// end iterator
		json::const_iterator json::end () const
		{
			switch (type)
			{
			case json_type::Array:
				return const_iterator (*this, array_elements.size ());
			case json_type::Object:
				return const_iterator (*this, object_elements.size ());
			default:
				return const_iterator (*this, 1);
			}
		}

		json::iterator json::end ()
		{
			switch (type)
			{
				case json_type::Array:
					return iterator (*this, array_elements.size ());
				case json_type::Object:
					return iterator (*this, object_elements.size ());
				default:
					return iterator (*this, 1);
			}
		}

		// Get the size
		size_t json::size () const
		{
			switch (type)
			{
			case json_type::Array:
				return array_elements.size ();
			case json_type::Object:
				return object_elements.size ();
			default:
				return 0;
			}
		}

		// merge two jsons
		void json::merge (const json & incoming, bool keep_existing)
		{
			// if the types differ, replace the whole element
			// do the same if the types match and the type is not object
			// (which means arrays are overwritten as a whole)
			if (type != incoming.type || type != json_type::Object)
			{
				// Overwrite local element if we're required to do so
				// or if it is empty (which means it does not actually exist)
				if (!keep_existing || type == json_type::Empty)
				{
					*this = incoming;
				}

				return;
			}

			for (auto inc : incoming)
			{
				(*this)[inc.first].merge (inc.second, keep_existing);
			}
		}

		// erase an element from a json object
		void json::erase (const std::string & k)
		{
			if (type != json_type::Object)
			{
				return;
			}

			object_elements.erase (k);
		}

		// remove existing elements that are also present in the incoming
		void json::subtract (const json & incoming)
		{
			if (type != json_type::Object || incoming.type != json_type::Object)
			{
				// subtract may operate on objects only
				return;
			}

			for (auto inc : incoming)
			{
				if (inc.second.type != json_type::Object)
				{
					// If the incoming element is not object, it has no further structure
					// So delete it at once
					erase (inc.first);
					continue;
				}

				// Get the local element with the same name
				auto target = (*this)[inc.first];

				if (target.type != json_type::Object)
				{
					// This is not an object, so delete is at once
					erase (inc.first);
					continue;
				}

				target.subtract (inc.second);
			}
		}
	}
}
