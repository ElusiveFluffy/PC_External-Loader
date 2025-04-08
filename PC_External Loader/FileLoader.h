#pragma once
#include <map>
#include <string>
#include <chrono>
using namespace std::chrono;
#include <filesystem>
namespace fs = std::filesystem;

namespace FileLoader
{
	inline bool HookFailed = false;
	//The amount of seconds the file cache is valid since the last file load
	inline double CacheValidSeconds = 5.0;

	struct FileLookup {
		system_clock::time_point LastLookup;
		//Key is just the file name, the value is the full path
		std::map<std::string, fs::path> FileTable;
	};

	void Init();
	void DeInit();
	bool HookTyLoadResources();
};

