#include "App.h"
#include "../display/Display.h"

namespace Wenv::Apps
{
::Wenv::Display::Rect App::get_client_area (const std::string &path)
{
	// Return exact match if we have one
	if (client_areas.find (path) != client_areas.end ())
	{
		return client_areas[path];
	}

	auto px = path.find_last_of ('.');

	if (px != std::string::npos)
	{
		auto ppath = path.substr (0, px + 1);

		for (auto &el : client_areas)
		{
			if (el.first.starts_with (ppath)) {
				return el.second;
			}
		}
	}

	return {};
}

}
