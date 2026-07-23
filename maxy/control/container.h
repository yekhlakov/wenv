#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <type_traits>

namespace maxy::control
{

// "DI"-like container
// Stores objects of various classes and allows retrieval of these objects by their class
class Container
{
	std::unordered_map<size_t, int *> registered;

	std::vector<std::pair<int *, std::function<void (int *)>>> destructors;

	// Version for the stored class that takes container as first argument to its constructor
	template <class C, typename ...As, std::enable_if_t<std::is_constructible<C, Container &, As...>::value, bool> = true >
	C *construct (As&&...args)
	{
		return std::apply (
			[=] (Container &x, As&& ...ys) { return new C (x, ys...); },
			std::forward_as_tuple (*this, args...)
		);
	}

	// Version for the stored class that does NOT take the container as first argument to its constructor
	template <class C, typename ...As, std::enable_if_t<std::is_constructible<C, As...>::value, bool > = true >
	C *construct (As&&...args)
	{
		return std::apply (
			[=] (As&& ...ys) { return new C (ys...); },
			std::forward_as_tuple (args...)
		);
	}

public:
	
	// Upon destruction of the container it destroys all registered objects in reverse order
	~Container ()
	{
		for (auto it = destructors.rbegin (); it != destructors.rend (); it++)
		{
			it->second (it->first);
		}
	}

	// Add a class to the container (possibly providing constructor arguments other than container itself)
	template<class C, typename ... As>
	C &add (As&&... args)
	{
		auto ti = typeid(C).hash_code ();

		C *c = construct<C> (args...);

		registered[ti] = (int *) c;
		destructors.push_back (std::make_pair ((int *) c, [] (int *x) -> void { delete (C *) x; }));

		return *c;
	}

	struct Not_registered {};

	// Get an existing element or fail if no element is present
	template<class C>
	C &get ()
	{
		auto ti = typeid(C).hash_code ();

		if (registered.find (ti) != registered.end ()) {
			return *(C *) (registered[ti]);
		}

		throw Not_registered {};
	}
};

}
