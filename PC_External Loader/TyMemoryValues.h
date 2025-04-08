#pragma once

namespace TyMemoryValues {
	#define MallocOffset 0x1B2

	typedef char* (*TyMalloc_t) (int size);

	UINT FileSys_LoadAddress;
	UINT TyFileSys_Exists;
	UINT GetFMVSizeAddress;
	TyMalloc_t TyMalloc;
	DWORD TyBaseAddress;
}