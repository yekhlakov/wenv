#pragma once
#ifndef _MAXY_CONTROL_EVENTS
#define _MAXY_CONTROL_EVENTS

#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <typeinfo>
#include <typeindex>
#include <algorithm>
#include <functional>

namespace maxy::control::events
{

// Base for the events
class Event
{
public:
	virtual ~Event () = default;
};

// Possible results of event handling
enum class Result : int
{
	Void, // No listeners for this event
	Ok,  // All listeners behaved well
	Failed, // Some listeners failed
	Fatal,  // There was a fatal error, some listeners were not invoked at all
	Queued  // We're currently processing an event, so this got queued
};

// The dispatcher
class Dispatcher
{
	// basic listener type for internal storage
	using base_listener_type = Result (Event *);

	// listener type for outside world (matching plain function pointers)
	template<class E>
	using concrete_listener_type = Result (E *);

public:

	// listener type for outside world (for wrapping lambdas)
	template<class E>
	using listener_type = std::function<concrete_listener_type<E>>;

private:
	// a list of sets of listeners
	std::unordered_map<std::type_index, std::vector<std::function<base_listener_type>>> listeners;

	// are we currently processing an event
	volatile bool is_busy = false;

	// event queue
	std::deque<Event *> queue;

	/**
	 * Process the event from the queue front
	 */
	Result dispatch ()
	{
		if (!queue.size ())
		{
			return Result::Void;
		}

		auto e = queue.front ();
		queue.pop_front ();

		auto pool = listeners.find (std::type_index (typeid (*e)));
		if (pool == listeners.end ())
		{
			// No handlers for this event -> void result
			delete e;

			return Result::Void;
		}

		auto res = Result::Ok;

		// Iterate over the registered listeners
		for (auto &listener : pool->second)
		{
			auto partial_res = listener (e);

			if (partial_res == Result::Fatal)
			{
				// a fatal error terminates the iteration
				res = partial_res;
				break;
			}
			else if (partial_res == Result::Failed)
			{
				// a regular error just sets the result
				res = partial_res;
			}
		}

		delete e;

		return res;
	}

	// Add a listener
	// As function objects cannot be compared, we have no way to check if the listener is already added.
	// So duplicates are perfectly possible.
	// Variant for direct addition
	template <typename E>
	void register_listener_fn (std::function<base_listener_type> f)
	{
		auto event_type = std::type_index (typeid (E));
		auto &pool = listeners[event_type];
		pool.push_back (f);
	}

public:

	// Add a listener
	// Variant for lambdas
	// Sadly we cannot pass a lambda directly so we must wrap it in a listener_type
	// like d.register_listener (listener_type<E> ([] (E * e) { /*...*/ }))
	template <typename E>
	void register_listener (listener_type<E> l)
	{
		register_listener_fn<E> (std::function<base_listener_type> ([=] (Event *e) -> Result {return l ((E *) e); }));
	}

	// Variant for good old C-style function pointers
	template <typename E>
	void register_listener (concrete_listener_type<E> l)
	{
		register_listener_fn<E> (std::function<base_listener_type> ([=] (Event *e) -> Result {return l ((E *) e); }));
	}

	// Handle the event
	// Return the result of the event handling
	// The event is destroyed after handling
	Result operator() (Event *e)
	{
		queue.push_back (e);

		if (is_busy)
		{
			return Result::Queued;
		}

		is_busy = true;

		Result r = Result::Void;

		while (queue.size ())
		{
			r = dispatch ();
		}

		is_busy = false;

		return r;
	}
};

}

#endif // _MAXY_CONTROL_EVENTS
