#pragma once

namespace TyMemoryValues {
	#define MallocOffset 0x1B2

	typedef char* (*TyMalloc_t) (int size);

	UINT FileSys_LoadAddress;
	UINT GetFMVSizeAddress;
	TyMalloc_t TyMalloc;
	DWORD TyBaseAddress;
}