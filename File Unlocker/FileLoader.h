#pragma once
namespace FileLoader
{
	inline bool HookFailed = false;

	void Init();
	void DeInit();
	bool HookTyLoadResources();
};

