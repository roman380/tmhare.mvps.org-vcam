#include "Placeholder.h"
#include <iterator>
#include <fstream>
#include <windows.h>
#include <strsafe.h>
#include <stdint.h>

extern HINSTANCE dll_inst;

void Placeholder::initialize_placeholder()
{
	if (!data.empty())
		return;

	load_placeholder();
}

void Placeholder::load_placeholder()
{
	wchar_t file[MAX_PATH];
	if (!GetModuleFileNameW(dll_inst, file, MAX_PATH)) {
		return;
	}

	wchar_t* slash = wcsrchr(file, '\\');
	if (!slash) {
		return;
	}

	slash[1] = 0;

	StringCbCat(file, sizeof(file), L"logo.yuv");

	cx = 1920;
	cy = 1080;

	std::ifstream is(file);
	if (is)
	{
		std::istream_iterator<uint8_t> start(is), end;
		data = std::vector<uint8_t>(start, end);
	}
	return;
}
