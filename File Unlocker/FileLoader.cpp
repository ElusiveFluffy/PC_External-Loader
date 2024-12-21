#include "pch.h"
#include "MinHook.h"
#include "FileLoader.h"
#include "TyMemoryValues.h"
#include "TygerFrameworkAPI.hpp"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>

using namespace TyMemoryValues;

typedef void* (*tyFileSys_Load) (char* fileName, int* pOutLen, char* pMemoryAllocated, int spaceAllocated);
typedef int (*tyGetFMVSize) (char* fileName, int* pOutLen, bool unknown, int* alwaysNullPtr);
tyGetFMVSize Original_tyGetFMVSize;
tyFileSys_Load Original_tyFileSys_Load;
static char PC_ExternalPath[0x121];

char* ReadFile(std::ifstream* fileStream, int* pOutLen, char* pMemoryAllocated, int spaceAllocated) {
    int fSize = (int)(*fileStream).tellg();
    if (fSize > 0) {

        if (pMemoryAllocated) {
            if (spaceAllocated >= 0)
                fSize = spaceAllocated;
        }
        //If the pMemoryAllocated isn't already allocated, allocate it through the function Ty uses
        else {
            pMemoryAllocated = TyMemoryValues::TyMalloc(fSize + 1);
            (pMemoryAllocated)[fSize] = 0;
        }

        if (pOutLen)
            *pOutLen = fSize;

        //Read all the data
        (*fileStream).seekg(0, std::ios::beg);
        if ((*fileStream).read(pMemoryAllocated, fSize)) {
            return pMemoryAllocated;
        }
    }

    return NULL;
}

void* FileSys_Load(char* fileName, int* pOutLen, char* pMemoryAllocated, int spaceAllocated) {
    PC_ExternalPath[0] = 0;
    sprintf_s(PC_ExternalPath, "PC_External\\%s", fileName);
    std::ifstream fileStream(PC_ExternalPath, std::ios::binary | std::ios::ate);

    //Just use the loading function from the game if the file doesn't exist in PC_External
    if (!fileStream)
        return Original_tyFileSys_Load(fileName, pOutLen, pMemoryAllocated, spaceAllocated);
    else {
        //The API may not be initialized yet here
        if (API::IsInitialized())
            API::LogPluginMessage("Loading \"" + std::string(fileName) + "\" From PC_External");
        else
            std::cout << "[Info] Loading \"" << fileName << "\" From PC_External" << std::endl;

        pMemoryAllocated = ReadFile(&fileStream, pOutLen, pMemoryAllocated, spaceAllocated);
        fileStream.close();
        return pMemoryAllocated;
    }
}

//Not fully sure??
int GetFMVSize(char* fileName, int* pOutLen, bool unknown, int* alwaysNullPtr) {
    auto testReturn = Original_tyGetFMVSize(fileName, pOutLen, unknown, alwaysNullPtr);
    return testReturn;
}

void FileLoader::Init()
{
    TyMemoryValues::TyBaseAddress = (DWORD)GetModuleHandle(0);
    TyMemoryValues::FileSys_LoadAddress = TyBaseAddress + 0x1B87C0;
    TyMemoryValues::GetFMVSizeAddress = TyBaseAddress + 0x1B8A00;
    // Get the offset the function call uses, which is a relative offset (can be negative and positive), so the address to the function is needed to be added on to get a non relative pointer, 
    // + 5 is added onto the end as the offset is relative from the ending of the function call, not the begining, and the function call is 5 bytes long.
    // It then gets cast as a function to be called later
    TyMemoryValues::TyMalloc = (TyMalloc_t)(*(int*)(FileSys_LoadAddress + MallocOffset + 1) + (FileSys_LoadAddress + MallocOffset) + 5); // TY.exe+1B8972 - E8 C9DEFDFF - call TY.exe+196840 (Addresses from new patch)
    HookFailed = !HookTyLoadResources();
}

void FileLoader::DeInit()
{
    //Remove the hooks
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

bool FileLoader::HookTyLoadResources() {
    //Setup MinHook
    MH_STATUS minHookStatus = MH_Initialize();
    if (minHookStatus != MH_OK)
        return false;

    minHookStatus = MH_CreateHook((LPVOID*)TyMemoryValues::FileSys_LoadAddress, &FileSys_Load, reinterpret_cast<LPVOID*>(&Original_tyFileSys_Load));
    if (minHookStatus != MH_OK)
        return false;

    //minHookStatus = MH_CreateHook((LPVOID*)GetFMVSizeAddress, &GetFMVSize, reinterpret_cast<LPVOID*>(&Original_tyGetFMVSize));
    //if (minHookStatus != MH_OK)
    //    return false;

    minHookStatus = MH_EnableHook(MH_ALL_HOOKS);
    if (minHookStatus != MH_OK)
        return false;

    return true;
}
