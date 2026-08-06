#pragma once
#include "../App.h"

namespace Wenv::Apps
{

class FileList : public App
{
public:

	FileList (const std::wstring &n) : App { n } {}

	// Func menu wants all keypresses
	virtual bool wants_all_keypresses () override { return true; }

	virtual void draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area) override;
	virtual void redraw (const std::string &path) override;
	virtual void click (::Wenv::Display::Rect client_area, int modifiers) override;
	virtual void keypress (int key, int modifiers) override;

	void redraw_all (const std::string &path);

	inline static const int SORT_MODE_DEFAULT = 0;
	inline static const int SORT_MODE_NAME = 0;
	inline static const int SORT_MODE_REVERSE_NAME = 1;
	inline static const int SORT_MODE_SIZE = 2;
	inline static const int SORT_MODE_REVERSE_SIZE = 3;
	inline static const int SORT_MODE_DATE = 4;
	inline static const int SORT_MODE_REVERSE_DATE = 5;
};

}
