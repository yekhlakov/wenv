#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace Wenv::Apps
{

class Context
{
	std::unordered_map< std::string, std::pair<size_t, int*>> stored;
	std::string name;

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
};

}
