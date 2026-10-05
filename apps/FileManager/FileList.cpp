#include <algorithm>
#include <cwchar>
#include <format>
#include <map>
#include <regex>
#include <Shlwapi.h>
#include <Windows.h>
#include <shellapi.h>
#include "../../maxy/strings.h"
#include "../../display/Display.h"
#include "../../display/Palette.h"
#include "../../display/Window.h"
#include "FileList.h"
#include "../FileEditor/FileEditor.h"
#include "../../Context.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

namespace Wenv::Apps
{

File_list_type *list_directory_contents (const std::wstring &dirname)
{
	auto v = new std::vector<WIN32_FIND_DATAW> {};

	std::wstring searchPath = dirname + L"\\*";

	WIN32_FIND_DATAW findData;
	HANDLE hFind = FindFirstFileW (searchPath.c_str (), &findData);

	if (hFind == INVALID_HANDLE_VALUE) {
		return v;
	}

	do {
		std::wstring fileName = findData.cFileName;

		// Skip current directory dot "."
		if (fileName == L".") {
			continue;
		}

		v->push_back (findData);

	} while (FindNextFileW (hFind, &findData) != 0); // Fetch next item

	// Clean up the search handle resource
	FindClose (hFind);

	return v;
}

File_list_type * sort_file_list (File_list_type *v, int sort_mode)
{	
	File_list_type dirs {};
	File_list_type files {};


	bool has_up = false;
	WIN32_FIND_DATAW up;

	for (auto &f : *v)
	{
		if (std::wstring { f.cFileName } == L"..")
		{
			// `..` directory is always first regardless of sort mode
			has_up = true;
			up = f;

			continue;
		}

		if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			dirs.push_back (f);
		}
		else
		{
			files.push_back (f);
		}
	}

	// Default sort: alphabetical, directories first

	auto sorter = 
		// reverse alphabetical
		sort_mode == FileList::SORT_MODE_REVERSE_NAME ? [] (WIN32_FIND_DATAW & a, WIN32_FIND_DATAW & b) { return std::wstring { a.cFileName } > std::wstring { b.cFileName }; } :
		// size 
		sort_mode == FileList::SORT_MODE_SIZE ? [] (WIN32_FIND_DATAW &a, WIN32_FIND_DATAW &b) {
			return a.nFileSizeHigh < b.nFileSizeHigh || a.nFileSizeHigh == b.nFileSizeHigh && a.nFileSizeLow < b.nFileSizeLow;
		} :
		// reverse size
		sort_mode == FileList::SORT_MODE_REVERSE_SIZE ? [] (WIN32_FIND_DATAW &a, WIN32_FIND_DATAW &b) {
			return a.nFileSizeHigh > b.nFileSizeHigh || a.nFileSizeHigh == b.nFileSizeHigh && a.nFileSizeLow > b.nFileSizeLow;
		} :
		// default: NAME
		[] (WIN32_FIND_DATAW &a, WIN32_FIND_DATAW &b) { return std::wstring { a.cFileName } < std::wstring { b.cFileName }; }
	;

	std::sort (dirs.begin (), dirs.end (), sorter);
	std::sort (files.begin (), files.end (), sorter);

	v->clear ();
	// Combine .., other directories, and files
	v->reserve ((has_up ? 1 : 0) + dirs.size () + files.size ());
	if (has_up)
	{
		v->push_back (up);
	}
	std::ranges::copy (dirs, std::back_inserter (*v));
	std::ranges::copy (files, std::back_inserter (*v));

	return v;
}

int *get_selected_file_idx (::Wenv::Context * c, const std::wstring &dirname)
{
	return c->get<int> ("selected-file-idx " + maxy::strings::wchartoutf8 (dirname), [] () ->int *{ return new int { 0 }; });
}

// The names of the files selected in the given file list panel, as a list
// for the file operation commands. Currently the single file highlighted in
// the panel is returned; an empty list means there is nothing to operate on:
// there is no selection or the ".." parent entry is selected, which cannot
// be copied, moved or deleted
std::vector<std::wstring> get_selected_file_names (::Wenv::Context * c)
{
	std::vector<std::wstring> names;

	auto pwd = c->get<std::wstring> ("pwd");

	if (pwd == nullptr)
	{
		return names;
	}

	auto lst = c->get<File_list_type> ("sorted-list");
	auto idx = get_selected_file_idx (c, *pwd);

	if (lst != nullptr && *idx >= 0 && *idx < (int) lst->size ())
	{
		auto name = std::wstring { (*lst)[*idx].cFileName };

		// The parent directory entry cannot be operated on
		if (name != L"..")
		{
			names.push_back (name);
		}
	}

	return names;
}

// Open the given file of the panel working directory in the editor display,
// either for viewing or for editing
static void open_named_file (::Wenv::Display::Display *display, ::Wenv::Context *c, const std::wstring &filename, bool is_editing)
{
	auto pwd = c->get<std::wstring> ("pwd");

	if (pwd == nullptr || pwd->empty () || filename.empty ())
	{
		return;
	}

	// The editor keeps the shown file together with the requested edit state
	// in its own context, which it reads when it and its status bar redraw
	auto ctx = display->window->get_display ("file-editor")->get_context ("file-editor");

	// Opening a file starts a new editing session: the undo operations of
	// the previous one do not apply
	clear_undo (ctx);

	ctx->set ("edit-target", new std::wstring { filename });
	ctx->set ("edit-pwd", new std::wstring { *pwd });
	ctx->set ("is_editing", new bool { is_editing });

	display->window->set_display ("file-editor");
}

void show_selected_file (::Wenv::Display::Display *display, ::Wenv::Context *c, bool is_editing)
{
	// The selection is read from the context of the panel, so the call may
	// come either from the func menu or from the file list itself
	auto pwd = c->get<std::wstring> ("pwd");

	if (pwd == nullptr)
	{
		return;
	}

	auto lst = c->get<File_list_type> ("sorted-list");
	auto idx = get_selected_file_idx (c, *pwd);

	// Nothing is shown when there is no list yet or when the selection has
	// fallen out of it (e.g. after the directory has changed)
	if (lst == nullptr || *idx < 0 || *idx >= (int) lst->size ())
	{
		return;
	}

	auto &selected = (*lst)[*idx];

	// Only a file can be shown in the editor
	if (selected.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		return;
	}

	open_named_file (display, c, selected.cFileName, is_editing);
}

// The command of the Open button of the "Open a file for editing" modal
static void open_modal_file (::Wenv::Display::Display *display);

// Show the "Open a file for editing" modal with the given text input prefill
static void show_open_file_modal_with (::Wenv::Display::Display *display, const std::wstring &prefill)
{
	std::vector<::Wenv::Display::ModalButton> buttons =
	{
		{ L"Open", open_modal_file },
		{ L"Cancel", nullptr }
	};

	display->show_modal
	(
		"text-input",
		L"Open a file for editing",
		L"File name:",
		buttons,
		::Wenv::Display::Palette::Active_element_color,
		::Wenv::Display::Palette::Default_color,
		::Wenv::Display::Palette::Default_color,
		prefill
	);
}

// The command of the Open button of the "Open a file for editing" modal: the
// file name typed into the text input box is opened in the editor. An empty
// name does nothing: the modal is shown again unchanged
static void open_modal_file (::Wenv::Display::Display *display)
{
	auto ctx = display->get_context ("modal");

	if (ctx != nullptr)
	{
		auto input = ctx->get<std::wstring> ("modal-text-input");

		// The panel that was active when the modal was shown still holds the
		// focus, so its working directory is where the file is opened
		if (input != nullptr && !input->empty () && display->focused_context != nullptr)
		{
			open_named_file (display, display->focused_context, *input, true);
			return;
		}
	}

	show_open_file_modal_with (display, L"");
}

void show_open_file_modal (::Wenv::Display::Display *display, ::Wenv::Context *c)
{
	// The input is prefilled with the file highlighted in the active panel;
	// only a regular file name makes a sensible default, a highlighted
	// directory (or no selection) leaves the box empty
	std::wstring prefill;

	auto pwd = c->get<std::wstring> ("pwd");

	if (pwd != nullptr)
	{
		auto lst = c->get<File_list_type> ("sorted-list");
		auto idx = get_selected_file_idx (c, *pwd);

		if (lst != nullptr && *idx >= 0 && *idx < (int) lst->size ())
		{
			auto &selected = (*lst)[*idx];

			if (!(selected.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				prefill = selected.cFileName;
			}
		}
	}

	show_open_file_modal_with (display, prefill);
}

// The working directory of the panel opposite to the given one; the panel
// contexts are named "file-manager-left-panel" and "file-manager-right-panel"
static std::wstring * get_opposite_pwd (::Wenv::Display::Display *display, ::Wenv::Context *c)
{
	if (display == nullptr || c == nullptr)
	{
		return nullptr;
	}

	auto target = std::string { "file-manager-right-panel" };

	if (c->get_name () == target)
	{
		target = "file-manager-left-panel";
	}

	auto ctx = display->get_context (target);

	return ctx != nullptr ? ctx->get<std::wstring> ("pwd") : nullptr;
}

// Whether the path is absolute (drive-lettered or rooted); a relative path
// points into the working directory of the panel the file is copied from
static bool is_absolute_path (const std::wstring &path)
{
	return path.size () >= 2 && path[1] == L':'
		|| !path.empty () && path[0] == L'\\';
}

// Whether the given path is an existing directory
static bool is_directory (const std::wstring &path)
{
	auto attr = GetFileAttributesW (path.c_str ());
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

// Whether the given path exists, as a file or as a directory
static bool is_existing_path (const std::wstring &path)
{
	return GetFileAttributesW (path.c_str ()) != INVALID_FILE_ATTRIBUTES;
}

// The target path of the copy: an existing directory in the input receives
// the file under its own name, otherwise the input is the target path itself
static std::wstring resolve_copy_destination (const std::wstring &target, const std::wstring &filename)
{
	if (!is_directory (target))
	{
		return target;
	}

	auto dest = target;

	if (dest.back () != L'\\')
	{
		dest += L"\\";
	}

	return dest + filename;
}

// Create the given directory together with its missing parent directories.
// Returns whether the directory exists in the end; the code of the first
// error met on the way is stored into error
static bool create_directories (const std::wstring &path, DWORD &error)
{
	if (path.empty ())
	{
		return false;
	}

	// Anything that already exists needs no creation
	if (GetFileAttributesW (path.c_str ()) != INVALID_FILE_ATTRIBUTES)
	{
		return true;
	}

	// The parents are created first, then the directory itself
	auto pos = path.find_last_of (L"\\/");

	if (pos != std::wstring::npos && pos > 0)
	{
		create_directories (path.substr (0, pos), error);
	}

	if (CreateDirectoryW (path.c_str (), nullptr) != 0 || GetLastError () == ERROR_ALREADY_EXISTS)
	{
		return true;
	}

	if (error == ERROR_SUCCESS)
	{
		error = GetLastError ();
	}

	return false;
}

// The system error message for the given error code, with the trailing line
// breaks FormatMessage appends trimmed off
static std::wstring get_error_message (DWORD error)
{
	wchar_t *buffer = nullptr;

	auto len = FormatMessageW
	(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		error,
		MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
		(wchar_t *) &buffer,
		0,
		nullptr
	);

	std::wstring message;

	if (len > 0 && buffer != nullptr)
	{
		message = buffer;

		while (!message.empty () && (message.back () == L'\r' || message.back () == L'\n' || message.back () == L' '))
		{
			message.pop_back ();
		}
	}

	if (buffer != nullptr)
	{
		LocalFree (buffer);
	}

	// A code the system has no message for is reported as its number
	return !message.empty () ? message : L"Error " + std::to_wstring (error);
}

// Whether the destination coincides with the source directory or lies inside
// it (the Win32 paths are case-insensitive): copying a directory into itself
// would recurse forever
static bool is_copy_into_self (const std::wstring &source, const std::wstring &dest)
{
	if (_wcsicmp (source.c_str (), dest.c_str ()) == 0)
	{
		return true;
	}

	auto dir = source.back () == L'\\' ? source : source + L"\\";
	return dest.size () > dir.size () && _wcsnicmp (dest.c_str (), dir.c_str (), dir.size ()) == 0;
}

// Copy a file or a whole directory (with all its contents) to the given
// target path. A target directory that already exists receives the contents
// of the source, a missing one is created along with its parents; an existing
// target file is overwritten. Returns whether the copy has succeeded; the
// code of the first error met on the way is stored into error
static bool copy_recursively (const std::wstring &source, const std::wstring &dest, DWORD &error)
{
	auto attr = GetFileAttributesW (source.c_str ());

	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		if (error == ERROR_SUCCESS)
		{
			error = GetLastError ();
		}

		return false;
	}

	if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		// A file: the missing intermediate directories of its target path
		// are created, then the file is copied
		auto pos = dest.find_last_of (L"\\/");

		if (pos != std::wstring::npos && pos > 0 && !create_directories (dest.substr (0, pos), error))
		{
			return false;
		}

		if (CopyFileW (source.c_str (), dest.c_str (), FALSE) == 0)
		{
			if (error == ERROR_SUCCESS)
			{
				error = GetLastError ();
			}

			return false;
		}

		return true;
	}

	// A directory: the target directory is created when missing and the
	// contents go into it entry by entry
	if (!create_directories (dest, error))
	{
		return false;
	}

	auto *contents = list_directory_contents (source);
	auto ok = contents != nullptr;

	if (ok)
	{
		for (auto &fd : *contents)
		{
			auto name = std::wstring { fd.cFileName };

			// The parent entry is not a part of the directory contents
			if (name == L"..")
			{
				continue;
			}

			ok = copy_recursively (source + L"\\" + name, dest + L"\\" + name, error) && ok;
		}

		delete contents;
	}

	return ok;
}

// Delete a file or a whole directory (with all its contents). Returns
// whether everything has been deleted; the code of the first error met on
// the way is stored into error
static bool delete_recursively (const std::wstring &path, DWORD &error)
{
	auto attr = GetFileAttributesW (path.c_str ());

	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		if (error == ERROR_SUCCESS)
		{
			error = GetLastError ();
		}

		return false;
	}

	if (!(attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		if (DeleteFileW (path.c_str ()) != 0)
		{
			return true;
		}

		if (error == ERROR_SUCCESS)
		{
			error = GetLastError ();
		}

		return false;
	}

	auto *contents = list_directory_contents (path);
	auto ok = contents != nullptr;

	if (ok)
	{
		for (auto &fd : *contents)
		{
			auto name = std::wstring { fd.cFileName };

			// The parent entry is not a part of the directory contents
			if (name == L"..")
			{
				continue;
			}

			ok = delete_recursively (path + L"\\" + name, error) && ok;
		}

		delete contents;
	}

	if (!ok)
	{
		return false;
	}

	// The directory itself is removable once it is empty
	if (RemoveDirectoryW (path.c_str ()) != 0)
	{
		return true;
	}

	if (error == ERROR_SUCCESS)
	{
		error = GetLastError ();
	}

	return false;
}

// Move (rename) a file or a whole directory to the given target path. A move
// within one volume is a rename, a cross-volume one degenerates into a copy
// with the source deleted afterwards; an existing target file is overwritten,
// an existing target directory receives the contents of the source. Returns
// whether the move has succeeded; the code of the first error met on the way
// is stored into error
static bool move_recursively (const std::wstring &source, const std::wstring &dest, DWORD &error)
{
	// An existing target directory receives the contents, so the move is a
	// copy followed by the removal of the source
	if (is_directory (dest))
	{
		if (!copy_recursively (source, dest, error))
		{
			return false;
		}

		return delete_recursively (source, error);
	}

	// The missing intermediate directories of the target path are created
	auto pos = dest.find_last_of (L"\\/");

	if (pos != std::wstring::npos && pos > 0 && !create_directories (dest.substr (0, pos), error))
	{
		return false;
	}

	// A same-volume move is a plain rename; the copy-allowed flag also moves
	// a file across the volumes, but not a directory
	if (MoveFileExW (source.c_str (), dest.c_str (), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
	{
		return true;
	}

	auto move_error = GetLastError ();

	// A directory cannot be moved across the volumes this way: it is copied
	// and the original is deleted then
	if (move_error != ERROR_NOT_SAME_DEVICE)
	{
		if (error == ERROR_SUCCESS)
		{
			error = move_error;
		}

		return false;
	}

	if (!copy_recursively (source, dest, error))
	{
		return false;
	}

	return delete_recursively (source, error);
}

// Forget the cached directory listings of both panels, so the files copied
// into the shown directories show up on the next redraw
static void refresh_file_lists (::Wenv::Display::Display *display)
{
	for (auto panel : { "file-manager-left-panel", "file-manager-right-panel" })
	{
		auto ctx = display->get_context (panel);

		if (ctx != nullptr)
		{
			ctx->erase ("list");
			ctx->erase ("sorted-list");
		}
	}
}

// The kinds of the pending file operation stored in the modal context (as
// an int), so the buttons of the warning modals can continue it
enum File_operation
{
	OP_COPY,
	OP_MOVE,
	OP_MKDIR,
	OP_DELETE
};

// The outcome of running the pending operation: it either has finished
// (empty error message and no conflict), has stopped on a file that could
// not be processed (the error message, the position left on it), or has
// stopped on a file whose target already exists (the conflict flag)
struct Operation_result
{
	std::wstring error_message;
	bool target_exists = false;
};

// The command of the Copy/Move button of the file operation modal
static void operation_modal_file (::Wenv::Display::Display *display);

// Show the "Copy a file" / "Move a file" modal with the given text input
// prefill. The pending operation itself (its kind, the files and their
// working directory) is remembered in the modal context by the command
// that shows the modal
static void show_file_operation_modal_with (::Wenv::Display::Display *display, bool is_move, const std::wstring &prefill)
{
	std::vector<::Wenv::Display::ModalButton> buttons =
	{
		{ is_move ? L"Move" : L"Copy", operation_modal_file },
		{ L"Cancel", nullptr }
	};

	display->show_modal
	(
		"text-input",
		is_move ? L"Move a file" : L"Copy a file",
		is_move ? L"Move to:" : L"Copy to:",
		buttons,
		::Wenv::Display::Palette::Active_element_color,
		::Wenv::Display::Palette::Default_color,
		::Wenv::Display::Palette::Default_color,
		prefill
	);
}

// The commands of the buttons of the warning and the target-exists modals
static void retry_operation (::Wenv::Display::Display *display);
static void skip_operation_file (::Wenv::Display::Display *display);
static void skip_all_operation_files (::Wenv::Display::Display *display);
static void overwrite_modal_file (::Wenv::Display::Display *display);
static void overwrite_all_modal_files (::Wenv::Display::Display *display);
static void skip_target_modal_file (::Wenv::Display::Display *display);
static void skip_all_target_modal_files (::Wenv::Display::Display *display);

// Show the warning modal reporting the error of the pending file operation,
// in the warning color. The Retry button restarts the operation from the
// file that has failed, the Cancel one just closes the modal, and when there
// are more files to process after the failed one, the Skip and Skip all
// buttons are shown too: they abandon the failed file and continue with the
// next one, the Skip all one leaving every following failing file behind
// silently as well
static void show_operation_error_modal (::Wenv::Display::Display *display, const std::wstring &error_message)
{
	auto ctx = display->get_context ("modal");
	auto kind = ctx != nullptr ? ctx->get<int> ("op-kind") : nullptr;
	auto files = ctx != nullptr ? ctx->get<std::vector<std::wstring>> ("op-files") : nullptr;
	auto index = ctx != nullptr ? ctx->get<int> ("op-index") : nullptr;

	std::vector<::Wenv::Display::ModalButton> buttons =
	{
		{ L"Retry", retry_operation }
	};

	// The failed file is not the last one of the operation, so the rest can
	// still be processed
	if (files != nullptr && index != nullptr && *index + 1 < (int) files->size ())
	{
		buttons.push_back ({ L"Skip", skip_operation_file });
		buttons.push_back ({ L"Skip all", skip_all_operation_files });
	}

	buttons.push_back ({ L"Cancel", nullptr });

	// The modal title names the operation that has failed
	std::wstring title = L"Operation failed";

	if (kind != nullptr)
	{
		if (*kind == OP_COPY)
		{
			title = L"Copy failed";
		}
		else if (*kind == OP_MOVE)
		{
			title = L"Move failed";
		}
		else if (*kind == OP_MKDIR)
		{
			title = L"Creation failed";
		}
		else if (*kind == OP_DELETE)
		{
			title = L"Deletion failed";
		}
	}

	display->show_modal
	(
		"warning",
		title,
		error_message,
		buttons,
		::Wenv::Display::Palette::Warning_element_color,
		::Wenv::Display::Palette::Warning_element_color,
		::Wenv::Display::Palette::Default_color
	);
}

// The info string of the given file for the target-exists modal: its size
// in bytes and its last-write date in the RFC3339 format
static std::wstring make_file_info (const std::wstring &path)
{
	WIN32_FILE_ATTRIBUTE_DATA info;

	if (!GetFileAttributesExW (path.c_str (), GetFileExInfoStandard, &info))
	{
		return L"unavailable";
	}

	auto size = ((unsigned long long) info.nFileSizeHigh << 32) | info.nFileSizeLow;

	auto local_time = SYSTEMTIME {};
	FileTimeToSystemTime (&info.ftLastWriteTime, &local_time);

	return std::format
	(
		L"{} bytes, {:04}-{:02}-{:02}T{:02}:{:02}:{:02}Z",
		size,
		local_time.wYear, local_time.wMonth, local_time.wDay,
		local_time.wHour, local_time.wMinute, local_time.wSecond
	);
}

// Show the modal asking what to do with the file whose target already
// exists, in the warning color: it names the file being processed and shows
// the size and the last-write date of both the source and the target
static void show_target_exists_modal (::Wenv::Display::Display *display)
{
	std::vector<::Wenv::Display::ModalButton> buttons =
	{
		{ L"Overwrite", overwrite_modal_file },
		{ L"Overwrite all", overwrite_all_modal_files },
		{ L"Skip", skip_target_modal_file },
		{ L"Skip all", skip_all_target_modal_files },
		{ L"Cancel", nullptr }
	};

	auto ctx = display->get_context ("modal");
	auto kind = ctx != nullptr ? ctx->get<int> ("op-kind") : nullptr;
	auto files = ctx != nullptr ? ctx->get<std::vector<std::wstring>> ("op-files") : nullptr;
	auto index = ctx != nullptr ? ctx->get<int> ("op-index") : nullptr;
	auto pwd = ctx != nullptr ? ctx->get<std::wstring> ("op-pwd") : nullptr;
	auto dest = ctx != nullptr ? ctx->get<std::wstring> ("op-dest") : nullptr;

	if (files == nullptr || index == nullptr || *index >= (int) files->size () || pwd == nullptr || dest == nullptr)
	{
		return;
	}

	auto name = (*files)[*index];
	auto source = *pwd + L"\\" + name;
	auto target = resolve_copy_destination (*dest, name);
	auto verb = kind != nullptr && *kind == OP_MOVE ? L"moved" : L"copied";

	auto text = L"The target of the file " + name + L" being " + verb + L" already exists:\n"
		L"Source (" + source + L"): " + make_file_info (source) + L"\n"
		L"Target (" + target + L"): " + make_file_info (target);

	display->show_modal
	(
		"warning",
		L"Target already exists",
		text,
		buttons,
		::Wenv::Display::Palette::Warning_element_color,
		::Wenv::Display::Palette::Warning_element_color,
		::Wenv::Display::Palette::Default_color
	);
}

// Perform the operation of the given kind on the single file: the delete
// needs no target, the copy and the move go to the given (already resolved)
// one. Returns an empty string on success, the error message otherwise
static std::wstring perform_file_operation (int kind, const std::wstring &source, const std::wstring &target)
{
	if (kind == OP_DELETE)
	{
		// The list of the paths to operate on must be double-null-terminated
		auto paths = source;
		paths.push_back (L'\0');

		SHFILEOPSTRUCTW operation = {};
		operation.wFunc = FO_DELETE;
		operation.pFrom = paths.c_str ();
		operation.fFlags = FOF_ALLOWUNDO;

		auto result = SHFileOperationW (&operation);

		// A deletion cancelled in the system dialog is not an error
		if (operation.fAnyOperationsAborted)
		{
			return L"";
		}

		// The operation reports its own error codes, not the system ones
		return result != 0 ? L"Error " + std::to_wstring ((unsigned) result) : L"";
	}

	// A directory cannot be copied or moved into itself
	if (is_directory (source) && is_copy_into_self (source, target))
	{
		return kind == OP_MOVE ? L"Cannot move a directory into itself" : L"Cannot copy a directory into itself";
	}

	DWORD error = ERROR_SUCCESS;

	if (kind == OP_MOVE ? !move_recursively (source, target, error) : !copy_recursively (source, target, error))
	{
		return error != ERROR_SUCCESS ? get_error_message (error) : L"The operation has failed";
	}

	return L"";
}

// Run the pending operation over the files that are still to be processed,
// starting at the current position. The modes chosen earlier (skipping the
// errors, overwriting or skipping the existing targets) are read from the
// modal context. On success (or when there is nothing left) the panel
// listings are refreshed and the outcome carries neither an error nor a
// conflict; otherwise the outcome reports either the file that has failed
// (with the position left on it) or the file whose target already exists
static Operation_result run_pending_operation (::Wenv::Display::Display *display)
{
	auto ctx = display->get_context ("modal");

	if (ctx == nullptr)
	{
		return {};
	}

	auto kind = ctx->get<int> ("op-kind");
	auto files = ctx->get<std::vector<std::wstring>> ("op-files");
	auto index = ctx->get<int> ("op-index");
	auto pwd = ctx->get<std::wstring> ("op-pwd");
	auto dest = ctx->get<std::wstring> ("op-dest");
	auto skip_errors = ctx->get<bool> ("op-skip-errors");
	auto overwrite_existing = ctx->get<bool> ("op-overwrite");
	auto skip_existing = ctx->get<bool> ("op-skip-existing");

	if (kind == nullptr || files == nullptr || index == nullptr || pwd == nullptr || dest == nullptr)
	{
		return {};
	}

	// The mode flags are optional: a missing one means it is not set
	auto is_skip_errors = skip_errors != nullptr && *skip_errors;
	auto is_overwrite_existing = overwrite_existing != nullptr && *overwrite_existing;
	auto is_skip_existing = skip_existing != nullptr && *skip_existing;

	Operation_result result;

	while (*index < (int) files->size ())
	{
		auto source = *pwd + L"\\" + (*files)[*index];

		// Only the copy and the move ask what to do when the target already
		// exists, unless the mode chosen earlier decides by itself
		auto target = *kind != OP_DELETE ? resolve_copy_destination (*dest, (*files)[*index]) : std::wstring {};

		if (*kind != OP_DELETE && !is_overwrite_existing && !is_skip_existing && is_existing_path (target))
		{
			// The position stays on the conflicting file
			result.target_exists = true;
			break;
		}

		result.error_message = perform_file_operation (*kind, source, target);

		if (!result.error_message.empty ())
		{
			if (!is_skip_errors)
			{
				// The position stays on the file that has failed
				break;
			}

			// The failed file is abandoned without any notice
			result.error_message = L"";
		}

		(*index)++;
	}

	// The processed files may have changed either of the shown directories,
	// so both panels rescan them
	refresh_file_lists (display);

	return result;
}

// Report the outcome of the run operation: the target-exists modal for the
// file whose target already exists, the error modal for the file that has
// failed, nothing when the operation has finished
static void report_operation_result (::Wenv::Display::Display *display, const Operation_result &result)
{
	if (result.target_exists)
	{
		show_target_exists_modal (display);
	}
	else if (!result.error_message.empty ())
	{
		show_operation_error_modal (display, result.error_message);
	}
}

// Attempt the creation of the directory by the given path (the whole missing
// branch of it): on success the panel listings are refreshed and an empty
// string is returned, on failure the error message to show is
static std::wstring attempt_directory_creation (::Wenv::Display::Display *display, const std::wstring &path)
{
	DWORD error = ERROR_SUCCESS;

	if (!create_directories (path, error))
	{
		return error != ERROR_SUCCESS ? get_error_message (error) : L"The directory has not been created";
	}

	// The new directory may have appeared in either of the shown ones, so
	// both panels rescan them
	refresh_file_lists (display);

	return L"";
}

// The command of the Retry button of the warning modal: the pending
// operation is attempted again from the file that has failed, a new failure
// brings the warning modal back up
static void retry_operation (::Wenv::Display::Display *display)
{
	auto ctx = display->get_context ("modal");

	if (ctx == nullptr)
	{
		return;
	}

	// The pending operation is remembered in the modal context by the
	// command that has shown the warning modal
	auto kind = ctx->get<int> ("op-kind");

	if (kind == nullptr)
	{
		return;
	}

	if (*kind == OP_MKDIR)
	{
		auto dest = ctx->get<std::wstring> ("op-dest");
		std::wstring error_message;

		if (dest != nullptr)
		{
			error_message = attempt_directory_creation (display, *dest);
		}

		if (!error_message.empty ())
		{
			show_operation_error_modal (display, error_message);
		}
	}
	else
	{
		report_operation_result (display, run_pending_operation (display));
	}
}

// Continue the pending operation from the file following the failed or
// conflicting one
static void continue_pending_operation (::Wenv::Display::Display *display)
{
	auto ctx = display->get_context ("modal");
	auto index = ctx != nullptr ? ctx->get<int> ("op-index") : nullptr;

	if (index == nullptr)
	{
		return;
	}

	// Abandon the current file
	(*index)++;

	report_operation_result (display, run_pending_operation (display));
}

// The command of the Skip button of the warning modal: the file that has
// caused the error is abandoned and the operation continues with the next
// one
static void skip_operation_file (::Wenv::Display::Display *display)
{
	continue_pending_operation (display);
}

// The command of the Skip all button of the warning modal: the file that has
// caused the error is abandoned, and so are all the following ones that
// fail, with no notice about the failures
static void skip_all_operation_files (::Wenv::Display::Display *display)
{
	auto ctx = display->get_context ("modal");

	if (ctx != nullptr)
	{
		// The mode for the following failing files
		ctx->set ("op-skip-errors", new bool { true });
	}

	continue_pending_operation (display);
}

// Resolve the conflict of the current file as the pressed button has chosen
// and continue the operation: the file is either forced over its existing
// target or abandoned, and the mode the choice implies for the following
// conflicting targets is remembered in the modal context
static void resolve_target_conflict (::Wenv::Display::Display *display, bool overwrite_current, bool overwrite_all, bool skip_all)
{
	auto ctx = display->get_context ("modal");

	if (ctx == nullptr)
	{
		return;
	}

	auto kind = ctx->get<int> ("op-kind");
	auto files = ctx->get<std::vector<std::wstring>> ("op-files");
	auto index = ctx->get<int> ("op-index");
	auto pwd = ctx->get<std::wstring> ("op-pwd");
	auto dest = ctx->get<std::wstring> ("op-dest");

	if (kind == nullptr || files == nullptr || index == nullptr || pwd == nullptr || dest == nullptr || *index >= (int) files->size ())
	{
		return;
	}

	std::wstring error_message;

	if (overwrite_current)
	{
		// The current file is forced over its existing target; when the
		// forced operation fails itself, the position stays on the file
		auto source = *pwd + L"\\" + (*files)[*index];
		auto target = resolve_copy_destination (*dest, (*files)[*index]);

		error_message = perform_file_operation (*kind, source, target);
	}

	if (error_message.empty ())
	{
		// The mode for the following conflicting targets
		if (overwrite_all)
		{
			ctx->set ("op-overwrite", new bool { true });
		}
		else if (skip_all)
		{
			ctx->set ("op-skip-existing", new bool { true });
		}

		(*index)++;

		report_operation_result (display, run_pending_operation (display));
	}
	else
	{
		// The forced operation may have changed the directories partly
		refresh_file_lists (display);
		show_operation_error_modal (display, error_message);
	}
}

// The command of the Overwrite button of the target-exists modal: the
// current file is forced over its existing target
static void overwrite_modal_file (::Wenv::Display::Display *display)
{
	resolve_target_conflict (display, true, false, false);
}

// The command of the Overwrite all button of the target-exists modal: the
// current file is forced over its existing target, and so are the following
// ones with existing targets, without asking
static void overwrite_all_modal_files (::Wenv::Display::Display *display)
{
	resolve_target_conflict (display, true, true, false);
}

// The command of the Skip button of the target-exists modal: the current
// file is abandoned
static void skip_target_modal_file (::Wenv::Display::Display *display)
{
	resolve_target_conflict (display, false, false, false);
}

// The command of the Skip all button of the target-exists modal: the current
// file is abandoned, and so are the following ones with existing targets
static void skip_all_target_modal_files (::Wenv::Display::Display *display)
{
	resolve_target_conflict (display, false, false, true);
}

// The command of the Copy/Move button of the file operation modal: the files
// of the pending operation are copied or moved to the path typed into the
// text input box. An empty name does nothing: the modal is shown again
// unchanged, a failed operation brings the warning modal up instead
static void operation_modal_file (::Wenv::Display::Display *display)
{
	auto ctx = display->get_context ("modal");

	// The panel that was active when the modal was shown still holds the
	// focus, so its selection decides what is operated on
	if (ctx == nullptr || display->focused_context == nullptr)
	{
		return;
	}

	auto input = ctx->get<std::wstring> ("modal-text-input");
	auto kind = ctx->get<int> ("op-kind");

	if (input == nullptr || input->empty () || kind == nullptr)
	{
		show_file_operation_modal_with (display, kind != nullptr && *kind == OP_MOVE, L"");
		return;
	}

	auto pwd = ctx->get<std::wstring> ("op-pwd");
	auto files = ctx->get<std::vector<std::wstring>> ("op-files");

	if (pwd == nullptr || files == nullptr || files->empty ())
	{
		return;
	}

	// The target path is resolved against the working directory the files
	// are taken from
	auto target = is_absolute_path (*input) ? *input : *pwd + L"\\" + *input;

	// The pending operation is remembered in the modal context, so the
	// buttons of the warning modals can continue it
	ctx->set ("op-dest", new std::wstring { target });
	ctx->set ("op-index", new int { 0 });

	report_operation_result (display, run_pending_operation (display));
}

// The shared body of the copy and the move modals of the active file list
// panel: the pending operation (the files with their working directory) is
// remembered in the modal context, so the commands of its buttons can run it
static void show_file_operation_modal (::Wenv::Display::Display *display, ::Wenv::Context *c, bool is_move, const std::vector<std::wstring> &files)
{
	if (files.empty ())
	{
		return;
	}

	auto pwd = c->get<std::wstring> ("pwd");

	if (pwd == nullptr)
	{
		return;
	}

	auto ctx = display->get_context ("modal");

	if (ctx != nullptr)
	{
		ctx->set ("op-kind", new int { is_move ? OP_MOVE : OP_COPY });
		ctx->set ("op-pwd", new std::wstring { *pwd });
		ctx->set ("op-files", new std::vector<std::wstring> { files });
		ctx->set ("op-dest", new std::wstring {});
		ctx->set ("op-index", new int { 0 });
		ctx->set ("op-skip-errors", new bool { false });
		ctx->set ("op-overwrite", new bool { false });
		ctx->set ("op-skip-existing", new bool { false });
	}

	// The input is prefilled with the directory the opposite panel shows
	std::wstring prefill;
	auto opposite_pwd = get_opposite_pwd (display, c);

	if (opposite_pwd != nullptr && !opposite_pwd->empty ())
	{
		prefill = *opposite_pwd;
	}

	show_file_operation_modal_with (display, is_move, prefill);
}

void show_copy_file_modal (::Wenv::Display::Display *display, ::Wenv::Context *c, const std::vector<std::wstring> &files)
{
	show_file_operation_modal (display, c, false, files);
}

void show_move_file_modal (::Wenv::Display::Display *display, ::Wenv::Context *c, const std::vector<std::wstring> &files)
{
	show_file_operation_modal (display, c, true, files);
}

// The command of the Create button of the mkdir modal
static void mkdir_modal_create (::Wenv::Display::Display *display);

// Show the "Create a directory" modal with an empty text input; the Create
// button creates the directory by the typed path in the working directory
// of the panel focused when it is pressed
void show_mkdir_modal (::Wenv::Display::Display *display)
{
	std::vector<::Wenv::Display::ModalButton> buttons =
	{
		{ L"Create", mkdir_modal_create },
		{ L"Cancel", nullptr }
	};

	// The pending operation is remembered in the modal context, so the
	// buttons of the warning modals observe it; it operates on no files, so
	// the whole state is reset to avoid the leftovers of a previous one
	auto ctx = display->get_context ("modal");

	if (ctx != nullptr)
	{
		ctx->set ("op-kind", new int { OP_MKDIR });
		ctx->set ("op-pwd", new std::wstring {});
		ctx->set ("op-dest", new std::wstring {});
		ctx->set ("op-files", new std::vector<std::wstring> {});
		ctx->set ("op-index", new int { 0 });
		ctx->set ("op-skip-errors", new bool { false });
		ctx->set ("op-overwrite", new bool { false });
		ctx->set ("op-skip-existing", new bool { false });
	}

	display->show_modal
	(
		"text-input",
		L"Create a directory",
		L"Directory name:",
		buttons,
		::Wenv::Display::Palette::Active_element_color,
		::Wenv::Display::Palette::Default_color,
		::Wenv::Display::Palette::Default_color
	);
}

// The command of the Create button of the mkdir modal: the directory is
// created by the path typed into the text input box, in the working directory
// of the active panel. An empty name has no effect
static void mkdir_modal_create (::Wenv::Display::Display *display)
{
	auto ctx = display->get_context ("modal");

	// The panel that was active when the modal was shown still holds the
	// focus, so its working directory receives the new directory
	if (ctx == nullptr || display->focused_context == nullptr)
	{
		return;
	}

	auto input = ctx->get<std::wstring> ("modal-text-input");

	// An empty name has no effect
	if (input == nullptr || input->empty ())
	{
		return;
	}

	auto pwd = display->focused_context->get<std::wstring> ("pwd");

	if (pwd == nullptr || pwd->empty ())
	{
		return;
	}

	// A relative path points into the working directory of the panel
	auto path = is_absolute_path (*input) ? *input : *pwd + L"\\" + *input;

	// The pending creation is remembered in the modal context, so the Retry
	// button of the warning modal can restart it
	ctx->set ("op-dest", new std::wstring { path });

	auto error_message = attempt_directory_creation (display, path);

	if (!error_message.empty ())
	{
		show_operation_error_modal (display, error_message);
	}
}

// Whether the file of the given name is a directory in the listing of the
// given panel
static bool is_listed_directory (::Wenv::Context *c, const std::wstring &name)
{
	auto lst = c->get<File_list_type> ("sorted-list");

	if (lst != nullptr)
	{
		for (auto &fd : *lst)
		{
			if (std::wstring { fd.cFileName } == name)
			{
				return (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			}
		}
	}

	return false;
}

// The question text of the delete confirmation: the files are named one by
// one (a directory is named as such), at most three of them are listed and
// the rest are counted; a single directory additionally warns that its
// contents are deleted along with it
static std::wstring make_delete_question (::Wenv::Context *c, const std::vector<std::wstring> &files)
{
	// The items of the question: the first three files and the count of the
	// rest when there are more
	std::vector<std::wstring> items;
	auto named = (std::min) (files.size (), (size_t) 3);

	for (size_t i = 0; i < named; i++)
	{
		items.push_back (is_listed_directory (c, files[i])
			? L"directory \"" + files[i] + L"\""
			: L"file " + files[i]);
	}

	if (files.size () > 3)
	{
		items.push_back (std::to_wstring (files.size () - 3) + L" more files");
	}

	// A single directory keeps the warning about its contents
	if (files.size () == 1 && is_listed_directory (c, files[0]))
	{
		items[0] += L" and all its contents";
	}

	auto question = std::wstring { L"Are you sure you want to delete " };

	for (size_t i = 0; i < items.size (); i++)
	{
		if (i > 0)
		{
			question += i + 1 < items.size () ? L", " : L" and ";
		}

		question += items[i];
	}

	return question + L"?";
}

// The command of the Yes button of the delete confirmation modal: the files
// selected when the modal was shown are deleted into the Recycle Bin
static void delete_modal_yes (::Wenv::Display::Display *display)
{
	report_operation_result (display, run_pending_operation (display));
}

void show_delete_file_modal (::Wenv::Display::Display *display, ::Wenv::Context *c, const std::vector<std::wstring> &files)
{
	if (files.empty ())
	{
		return;
	}

	auto pwd = c->get<std::wstring> ("pwd");

	if (pwd == nullptr)
	{
		return;
	}

	// The pending deletion is remembered in the modal context, so the
	// buttons of the modals can run and continue it
	auto ctx = display->get_context ("modal");

	if (ctx != nullptr)
	{
		ctx->set ("op-kind", new int { OP_DELETE });
		ctx->set ("op-pwd", new std::wstring { *pwd });
		ctx->set ("op-dest", new std::wstring {});
		ctx->set ("op-files", new std::vector<std::wstring> { files });
		ctx->set ("op-index", new int { 0 });
		ctx->set ("op-skip-errors", new bool { false });
		ctx->set ("op-overwrite", new bool { false });
		ctx->set ("op-skip-existing", new bool { false });
	}

	std::vector<::Wenv::Display::ModalButton> buttons =
	{
		{ L"Yes", delete_modal_yes },
		{ L"No", nullptr }
	};

	display->show_modal
	(
		"warning",
		files.size () > 1 ? L"Delete files" : L"Delete a file",
		make_delete_question (c, files),
		buttons,
		::Wenv::Display::Palette::Warning_element_color,
		::Wenv::Display::Palette::Warning_element_color,
		::Wenv::Display::Palette::Default_color
	);
}

bool is_executable_file (const std::wstring &filename)
{
	static const std::wregex pattern (
		LR"(\.(exe|bat|com|cmd|ps1|msi)$)",
		std::regex_constants::icase
	);

	return std::regex_search (filename, pattern);
}

std::string FileList::get_sort_mode_name (int sort_mode)
{
	// Dictionary of the sort mode indication strings
	static const std::map<int, std::string> sort_mode_names
	{
		{ SORT_MODE_NAME,			"(↑Name)" },
		{ SORT_MODE_REVERSE_NAME,	"(↓Name)" },
		{ SORT_MODE_SIZE,			"(↑Size)" },
		{ SORT_MODE_REVERSE_SIZE,	"(↓Size)" },
		{ SORT_MODE_DATE,			"(↑Date)" },
		{ SORT_MODE_REVERSE_DATE,	"(↓Date)" }
	};

	auto it = sort_mode_names.find (sort_mode);

	return it != sort_mode_names.end () ? it->second : std::string {};
}

void FileList::draw (::Wenv::Display::Display &display, const std::string &path, ::Wenv::Display::Rect client_area)
{
	App::draw (display, path, client_area);

	if (current_context == nullptr)
	{
		return;
	}

	redraw (path);
}

void FileList::redraw(const std::string &path)
{
	auto s = get_pwd ();
	auto sort_mode = get_sort_mode ();
	auto lst = get_file_list ();
	auto sorted_lst = get_sorted_file_list ();
	auto selected_file_idx = get_selected_file_idx (current_context, *s);
	auto current_client_area = get_client_area (path);
	auto list_offset = get_list_offset ();

	*selected_file_idx = min ((int) sorted_lst->size () - 1, *selected_file_idx);
	*selected_file_idx = max (0, *selected_file_idx);

	// The top line is reserved for the sort mode indication. The remaining
	// rows hold the file entries, so the list size is one row shorter
	auto list_size = current_client_area.height - 1;

	if (*selected_file_idx < *list_offset)
	{
		// The cursor moved outside of viewable area, so shift it
		*list_offset = *selected_file_idx;
	}
	else if (*selected_file_idx >= *list_offset + 2 * list_size)
	{
		*list_offset = *selected_file_idx - 2 * list_size + 1;
	}

	// The sort mode indication is printed centered and highlighted only in the
	// leftmost column; the top line in the other columns is kept empty so all
	// the file lists have the same height
	::Wenv::Display::Rect sort_rect = current_client_area;
	sort_rect.height = 1;

	if (name[0] == L'2')
	{
		// Non-leftmost column: just an empty top line
		current_display->print_line
		(
			sort_rect,
			L" ",
			current_display->PF_LEFT | current_display->PF_ERASE_BACKGROUND
		);
	}
	else
	{
		// Active (regular) color for the sort mode indication
		current_display->with_color (::Wenv::Display::Palette::Active_element_color);
		current_display->print_line
		(
			sort_rect,
			maxy::strings::utf8towchar (get_sort_mode_name (*sort_mode)),
			current_display->PF_CENTER | current_display->PF_ERASE_BACKGROUND
		);
	}

	::Wenv::Display::Rect fn_rect = current_client_area;
	fn_rect.height = 1;

	int p = 0;
	int p_begin = *list_offset;
	if (name[0] == L'2')
	{
		// This is the right column so we must skip some of the first elements
		p_begin += list_size;
	}

	for (auto &fd : *sorted_lst)
	{
		if (p < p_begin)
		{
			p++;
			continue;
		}

		// Determine display color based on file attributes
		const char *color = ::Wenv::Display::Palette::Default_color;
		std::wstring display_name { fd.cFileName };

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
		{
			color = ::Wenv::Display::Palette::Dark_element_color;
		}
		else if (is_executable_file (display_name))
		{
			color = ::Wenv::Display::Palette::Active_element_color;
			display_name += L"*";
		}

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			display_name += L"/";
		}

		if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
		{
			display_name += L"&";
		}

		if (p == *selected_file_idx && current_display->focused_context == current_context)
		{
			current_display->with_color (color, true);
		}
		else
		{
			current_display->with_color (color);
		}

		// Skip the top line reserved for the sort mode indication
		fn_rect.y = current_client_area.y + 1 + p - p_begin;

		if (fn_rect.y - current_client_area.y >= current_client_area.height)
		{
			break;
		}

		current_display->print_line
		(
			fn_rect,
			display_name,
			current_display->PF_TOP | current_display->PF_LEFT | current_display->PF_ERASE_BACKGROUND
		);

		p++;
	}


	if (p < p_begin)
	{
		// The column is empty, so start from the beginning
		p = p_begin;
	}

	current_display->with_color (::Wenv::Display::Palette::Default_color);

	// Clear the remains of the skipped top line and the list area
	while (p - p_begin < current_client_area.height - 1)
	{
		fn_rect.y = current_client_area.y + 1 + p - p_begin;
		current_display->print_line
		(
			fn_rect,
			L" ",
			current_display->PF_TOP | current_display->PF_LEFT | current_display->PF_ERASE_BACKGROUND
		);

		p++;
	}
}

bool FileList::handle_click (::Wenv::Display::Rect client_area, ::Wenv::Display::Pos position, int modifiers)
{
	auto path = *get_focused_path ();

	if (current_display->focused_context != current_context)
	{
		// Steal the focus if needed
		auto target_context = current_context;
		auto old_context = current_display->focused_context;
		
		// Temporarily set the old context and redraw (to reflect loss of focus)
		with_context (old_context);
		current_display->focused_context = nullptr;
		redraw_all (*get_focused_path ());
		// Then set the new context
		current_display->focused_context = target_context;
		with_context (target_context);
		// the redraw with "new" context will occur later
	}

	auto pwd = get_pwd ();
	auto sort_mode = get_sort_mode ();
	auto sorted_lst = get_sorted_file_list ();
	auto idx = get_selected_file_idx (current_context, *pwd);
	auto list_offset = get_list_offset ();
	auto list_size = client_area.height - 1;

	if (position.y == 0 && name[0] != L'2')
	{
		// The sort-mode indication is drawn on the first row of the leftmost
		// column; toggling it flips between ascending and descending
		*sort_mode ^= 1;
		current_context->erase ("sorted-list");
	}
	else if (position.y >= 1 && position.y - 1 < list_size)
	{
		// Compute the visible slice of the sorted list: the left panel starts
		// at list_offset, the right panel one list_size further down
		auto p_begin = *list_offset;
		if (name[0] == L'2')
		{
			p_begin += list_size;
		}

		auto file_idx = p_begin + position.y - 1;

		if (sorted_lst != nullptr && file_idx < (int) sorted_lst->size ())
		{
			*idx = file_idx;
		}
	}

	// The display redraws the whole screen when a click is handled
	redraw_all (path);

	return true;
}

bool FileList::handle_keydown (unsigned int key, int modifiers)
{
	auto pwd = get_pwd ();
	auto idx = get_selected_file_idx (current_context, *pwd);
	auto lst = get_sorted_file_list ();
	auto path = *get_focused_path ();

	// The top line of the client area shows the sort indication.
	// The path may belong to a sibling column, so look the area up without
	// inserting (operator[] on the map would silently add a zero-size rect)
	auto list_size = get_client_area (path).height - 1;

	if (modifiers & 1) // control
	{
		if (key == VK_F3) // Name
		{
			auto sort_mode = get_sort_mode ();

			if (*sort_mode == FileList::SORT_MODE_NAME || *sort_mode == FileList::SORT_MODE_REVERSE_NAME)
			{
				*sort_mode ^= 1;
				current_context->erase ("sorted-list");
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else if (key == VK_DOWN)
	{
		(*idx)++;
	}
	else if (key == VK_UP)
	{
		*idx = max (0, (*idx) - 1);
	}
	else if (key == VK_RIGHT)
	{
		(*idx) += list_size;
	}
	else if (key == VK_LEFT)
	{
		*idx = max (0, (*idx) - list_size);
	}
	else if (key == VK_RETURN)
	{
		if ((*lst)[*idx].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// change current directory
			auto next = std::wstring { (*lst)[*idx].cFileName };

			if (next == L"..")
			{
				// Forget the selected file of the directory we are leaving,
				// so when we come back we start from zero
				current_context->erase ("selected-file-idx " + maxy::strings::wchartoutf8 (*pwd));

				// go up
				*pwd += L"\\..";
				wchar_t buf[2000];
				PathCanonicalize (buf, pwd->c_str ());
				*pwd = buf;
			}
			else
			{
				// go down
				if (pwd->back () != L'\\')
				{
					*pwd += L"\\";
				}
				*pwd += next;

			}
			current_context->erase ("list");
			current_context->erase ("sorted-list");
		}
		else
		{
			return false;
		}
	}
	else if (key == VK_TAB)
	{
		// Switch panel focus
		auto n = current_context->get_name ();
		std::string target_context { "file-manager-left-panel" };
		if (n == target_context)
		{
			target_context = "file-manager-right-panel";
		}

		current_display->focused_context = nullptr;
		redraw_all (path);
		current_display->focused_context = current_display->get_context (target_context);
		with_context (current_display->focused_context);
		redraw_all (*get_focused_path ());
		return true;
	}
	else
	{
		return false;
	}

	redraw_all (path);

	return true;
}

void FileList::redraw_all (const std::string & path)
{
	auto apps = get_app_group ();
	if (apps != nullptr)
	{
		for (auto app : *apps)
		{
			app->with_context(current_context)->redraw (path);
		}
	}
}

std::wstring * FileList::get_pwd ()
{
	// No default can be provided for this parameter because it is set
	// from outside (the core) when the display is built
	return current_context->get<std::wstring> ("pwd");
}

int * FileList::get_sort_mode ()
{
	// Default: the alphabetical sort with directories first
	return current_context->get<int> ("sort-mode", [] () { return new int { 0 }; });
}

int * FileList::get_list_offset ()
{
	// Default: the list is scrolled to its first entry
	return current_context->get<int> ("list-offset", [] () { return new int { 0 }; });
}

std::string * FileList::get_focused_path ()
{
	// Set by the core when the display is built; no default makes sense here
	return current_context->get<std::string> ("focused-path");
}

std::vector<App *> * FileList::get_app_group ()
{
	// No default: a missing group simply means there are no apps to redraw
	return current_context->get<std::vector<App *>> ("app-group");
}

File_list_type * FileList::get_file_list ()
{
	// Default: the entries of the shown directory
	return current_context->get<File_list_type> ("list", [this] () { return list_directory_contents (*get_pwd ()); });
}

File_list_type * FileList::get_sorted_file_list ()
{
	// Default: the file list sorted with the current sort mode
	return current_context->get<File_list_type> ("sorted-list", [this] () { return sort_file_list (get_file_list (), *get_sort_mode ()); });
}

}
