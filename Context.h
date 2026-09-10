#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include "maxy/json.h"
#include "maxy/strings.h"

namespace Wenv
{

class Context
{
	std::unordered_map< std::string, std::pair<size_t, int*>> stored;
	std::string name;

	// Whether a type is a vector (to be converted to a json array)
	template<typename>
	struct is_vector : std::false_type
	{
	};

	template<typename E, typename A>
	struct is_vector<std::vector<E, A>> : std::true_type
	{
	};

	// Convert a stored value into its json representation.
	// json stores numbers (int / long double) and utf8 strings, so
	// wstrings have to be converted to utf8 first.
	template<typename T>
	static maxy::data::json value_to_json (const T &value)
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return maxy::data::json { value };
		}
		else if constexpr (std::is_same_v<T, std::wstring>)
		{
			return maxy::data::json { maxy::strings::wchartoutf8 (value) };
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return maxy::data::json { value };
		}
		else if constexpr (std::is_integral_v<T>)
		{
			return maxy::data::json { (long long) value };
		}
		else if constexpr (std::is_floating_point_v<T>)
		{
			return maxy::data::json { (long double) value };
		}
		else
		{
			// A vector of supported scalars - convert element by element
			auto arr = maxy::data::json::make_array ();

			for (auto e : value)
			{
				arr.push_back (value_to_json (e));
			}

			return arr;
		}
	}

	// Read a scalar value of type T from a json value.
	// json strings are utf8, so wstrings have to be converted back.
	template<typename T>
	static T value_from_json (const maxy::data::json &value)
	{
		if constexpr (std::is_same_v<T, std::wstring>)
		{
			return maxy::strings::utf8towchar ((std::string) value);
		}
		else
		{
			return (T) value;
		}
	}

	// Canonical type name stored in the "type" field of a serialized element.
	// It is used to restore the exact C++ type back during deserialization.
	template<typename T>
	static const char * name_of ()
	{
		if constexpr (std::is_same_v<T, int>) return "int";
		else if constexpr (std::is_same_v<T, long>) return "long";
		else if constexpr (std::is_same_v<T, float>) return "float";
		else if constexpr (std::is_same_v<T, double>) return "double";
		else if constexpr (std::is_same_v<T, bool>) return "bool";
		else if constexpr (std::is_same_v<T, std::string>) return "string";
		else if constexpr (std::is_same_v<T, std::wstring>) return "wstring";
		else if constexpr (std::is_same_v<T, std::vector<int>>) return "vector<int>";
		else if constexpr (std::is_same_v<T, std::vector<long>>) return "vector<long>";
		else if constexpr (std::is_same_v<T, std::vector<float>>) return "vector<float>";
		else if constexpr (std::is_same_v<T, std::vector<double>>) return "vector<double>";
		else if constexpr (std::is_same_v<T, std::vector<bool>>) return "vector<bool>";
		else if constexpr (std::is_same_v<T, std::vector<std::string>>) return "vector<string>";
		else if constexpr (std::is_same_v<T, std::vector<std::wstring>>) return "vector<wstring>";
		else return "?";
	}

	// If the stored type hash matches T, serialize the heap object into the element
	template<typename T>
	static bool try_serialize (size_t hash_code, int *ptr, maxy::data::json &element)
	{
		if (hash_code != typeid (T).hash_code ())
		{
			return false;
		}

		element["type"] = name_of<T> ();
		element["value"] = value_to_json (*reinterpret_cast<T *> (ptr));

		return true;
	}

	// Allocate a heap object of type T and fill it from the json value
	template<typename T>
	static int * allocate_from_json (const maxy::data::json &value)
	{
		if constexpr (is_vector<T>::value)
		{
			auto p = new T {};

			for (size_t i = 0; i < value.size (); i++)
			{
				p->push_back (value_from_json<typename T::value_type> (value[i]));
			}

			return (int *) p;
		}
		else
		{
			return (int *) new T { value_from_json<T> (value) };
		}
	}

	// If the "type" name matches T, restore a heap object of type T from the json value
	template<typename T>
	static bool try_deserialize (const std::string &type_name, const maxy::data::json &value, size_t &hash_code, int *&ptr)
	{
		if (type_name != name_of<T> ())
		{
			return false;
		}

		hash_code = typeid (T).hash_code ();
		ptr = allocate_from_json<T> (value);

		return true;
	}

public:

	const std::string &get_name () const { return name; }

	Context (const std::string &n) : name { n } {}

	Context *parent = nullptr;

	template<typename A>
	A * set (const std::string & n, A * a)
	{
		if (stored.find (n) != stored.end ())
		{
			// We replace the previous version with new, so the previous must be deleted
			delete stored[n].second;
		}

		stored[n] = { typeid(A).hash_code (), (int *) a };

		return a;
	}

	void erase (const std::string &n)
	{
		stored.erase (n);
	}

	template<typename A>
	A *get (const std::string &n, std::function<A *(void)> default_value_getter = [] () { return nullptr; })
	{
		if (stored.find (n) == stored.end ())
		{
			if (parent != nullptr)
			{
				auto pv = parent->get<A> (n);

				if (pv != nullptr)
				{
					return pv;
				}
			}
		} 
		else
		{
			if (stored[n].first != typeid(A).hash_code ())
			{
				// Wrong type, terminate
				throw 666;
			}

			return (A *) stored[n].second;
		}

		// Retrieve the default value
		auto defv = default_value_getter ();

		if (defv != nullptr)
		{
			stored[n] = { typeid(A).hash_code (), (int *) defv };
		}

		return defv;
	}

	// Convert the contents of this context into a json object.
	// Only the elements whose types can be represented in json are exported;
	// the rest are skipped. The result has a "version" field and a "data"
	// array of per-element objects {"name","type","value"}.
	maxy::data::json serialize () const
	{
		auto root = maxy::data::json::make_object ();
		root["version"] = "1.0";
		root["data"] = maxy::data::json::make_array ();

		// Iterate over the elements in a fixed order, so the output is deterministic
		std::vector<std::string> names;
		names.reserve (stored.size ());

		for (auto &[name, el] : stored)
		{
			names.push_back (name);
		}
		std::sort (names.begin (), names.end ());

		for (auto &name : names)
		{
			auto &el = stored.find (name)->second;
			auto element = maxy::data::json::make_object ();
			bool serialized =
				try_serialize<int> (el.first, el.second, element) ||
				try_serialize<long> (el.first, el.second, element) ||
				try_serialize<float> (el.first, el.second, element) ||
				try_serialize<double> (el.first, el.second, element) ||
				try_serialize<bool> (el.first, el.second, element) ||
				try_serialize<std::string> (el.first, el.second, element) ||
				try_serialize<std::wstring> (el.first, el.second, element) ||
				try_serialize<std::vector<int>> (el.first, el.second, element) ||
				try_serialize<std::vector<long>> (el.first, el.second, element) ||
				try_serialize<std::vector<float>> (el.first, el.second, element) ||
				try_serialize<std::vector<double>> (el.first, el.second, element) ||
				try_serialize<std::vector<bool>> (el.first, el.second, element) ||
				try_serialize<std::vector<std::string>> (el.first, el.second, element) ||
				try_serialize<std::vector<std::wstring>> (el.first, el.second, element);

			if (!serialized)
			{
				// The element type cannot be converted to json - skip it
				continue;
			}

			element["name"] = name;
			root["data"].push_back (element);
		}

		return root;
	}

	// Restore the contents of this context from a json object produced by serialize ().
	// The previous contents are discarded.
	void deserialize (maxy::data::json &serialized)
	{
		for (auto &[name, el] : stored)
		{
			delete el.second;
		}
		stored.clear ();

		auto &data = serialized["data"];

		for (size_t i = 0; i < data.size (); i++)
		{
			auto &element = data[i];

			auto el_name = (std::string) element["name"];
			auto type_name = (std::string) element["type"];
			auto &value = element["value"];

			size_t hash_code = 0;
			int *ptr = nullptr;

			bool restored =
				try_deserialize<int> (type_name, value, hash_code, ptr) ||
				try_deserialize<long> (type_name, value, hash_code, ptr) ||
				try_deserialize<float> (type_name, value, hash_code, ptr) ||
				try_deserialize<double> (type_name, value, hash_code, ptr) ||
				try_deserialize<bool> (type_name, value, hash_code, ptr) ||
				try_deserialize<std::string> (type_name, value, hash_code, ptr) ||
				try_deserialize<std::wstring> (type_name, value, hash_code, ptr) ||
				try_deserialize<std::vector<int>> (type_name, value, hash_code, ptr) ||
				try_deserialize<std::vector<long>> (type_name, value, hash_code, ptr) ||
				try_deserialize<std::vector<float>> (type_name, value, hash_code, ptr) ||
				try_deserialize<std::vector<double>> (type_name, value, hash_code, ptr) ||
				try_deserialize<std::vector<bool>> (type_name, value, hash_code, ptr) ||
				try_deserialize<std::vector<std::string>> (type_name, value, hash_code, ptr) ||
				try_deserialize<std::vector<std::wstring>> (type_name, value, hash_code, ptr);

			if (!restored)
			{
				// Unknown type name - skip this element
				continue;
			}

			stored[el_name] = { hash_code, ptr };
		}
	}
};

}
