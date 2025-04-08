#include "pch.h"
#include "MinHook.h"
#include "FileLoader.h"
#include "TyMemoryValues.h"
#include "TygerFrameworkAPI.hpp"

#include <iostream>
#include <fstream>

using namespace TyMemoryValues;

typedef void* (*tyFileSys_Load) (char* fileName, int* pOutLen, char* pMemoryAllocated, int spaceAllocated);
typedef bool (*tyFileSys_Exists)(char* pFilename, int* pOutLen);
typedef int (*tyGetFMVSize) (char* fileName, int* pOutLen, bool unknown, int* alwaysNullPtr);
tyGetFMVSize Original_tyGetFMVSize;
tyFileSys_Load Original_tyFileSys_Load;
tyFileSys_Exists Original_tyFileSys_Exists;

FileLoader::FileLookup FileCache{};

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

std::string ToLowerCase(std::string string) {
    std::transform(string.begin(), string.end(), string.begin(), ::tolower);
    return string;
}

void UpdateFileCache() {
    //Reset the file table
    FileCache.FileTable.clear();
    for (const auto entry : fs::recursive_directory_iterator("PC_External")) {
        //Skip any folders
        if (entry.is_directory())
            continue;

        //Easier to check with the requested file if everything is lowercase
        std::string filename = ToLowerCase(entry.path().filename().string());
        //Intended logic, only the first one found is used, was how it worked before the cache too (you shouldn't have more than 1 of the same file anyways XP)
        FileCache.FileTable.emplace(filename, entry.path());
    }

    FileCache.LastLookup = system_clock::now();
}

//Works mostly fine without this, mainly have this here so it'll update the file size of lv2s each load, instead of only on startup,
//to not have it crash if you make them bigger while the game is open
bool FileSys_Exists(char* fileName, int* pOutLen) {

    duration<double> elapsed_seconds = system_clock::now() - FileCache.LastLookup;

    //Been longer than the valid cache duration
    if (elapsed_seconds.count() > FileLoader::CacheValidSeconds)
        UpdateFileCache();
    FileCache.LastLookup = system_clock::now();

    std::string lowerCaseFileName = ToLowerCase(fileName);
    if (!FileCache.FileTable.contains(lowerCaseFileName))
        return Original_tyFileSys_Exists(fileName, pOutLen);
    //Only set it if its not null
    else if (pOutLen != NULL)
    {
        std::ifstream fileStream(FileCache.FileTable[lowerCaseFileName], std::ios::binary | std::ios::ate);
        *pOutLen = (int)(fileStream).tellg();
        fileStream.close();
    }

    return true;
}

void* FileSys_Load(char* fileName, int* pOutLen, char* pMemoryAllocated, int spaceAllocated) {
    duration<double> elapsed_seconds = system_clock::now() - FileCache.LastLookup;

    //Been longer than the valid cache duration
    if (elapsed_seconds.count() > FileLoader::CacheValidSeconds) 
        UpdateFileCache();
    FileCache.LastLookup = system_clock::now();

    std::string lowerCaseFileName = ToLowerCase(fileName);
    //Just use the loading function from the game if the file doesn't exist in PC_External
    //Will be invalid/null if the file doesn't exist in PC_External
    if (!FileCache.FileTable.contains(lowerCaseFileName))
        return Original_tyFileSys_Load(fileName, pOutLen, pMemoryAllocated, spaceAllocated);
    else {
        //The API may not be initialized yet here
        if (API::IsInitialized())
            API::LogPluginMessage("Loading \"" + std::string(fileName) + "\" From PC_External");
        else
            std::cout << "[Info] Loading \"" << fileName << "\" From PC_External" << std::endl;

        std::ifstream fileStream(FileCache.FileTable[lowerCaseFileName], std::ios::binary | std::ios::ate);
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
    TyMemoryValues::TyFileSys_Exists = TyBaseAddress + 0x1B8540;
    TyMemoryValues::GetFMVSizeAddress = TyBaseAddress + 0x1B8A00;
    // Get the offset the function call uses, which is a relative offset (can be negative and positive), so the address to the function is needed to be added on to get a non relative pointer, 
    // + 5 is added onto the end as the offset is relative from the ending of the function call, not the begining, and the function call is 5 bytes long.
    // It then gets cast as a function to be called later
    TyMemoryValues::TyMalloc = (TyMalloc_t)(*(int*)(FileSys_LoadAddress + MallocOffset + 1) + (FileSys_LoadAddress + MallocOffset) + 5); // TY.exe+1B8972 - E8 C9DEFDFF - call TY.exe+196840 (Address and opcode from new patch)
    HookFailed = !HookTyLoadResources();
    UpdateFileCache();
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

    minHookStatus = MH_CreateHook((LPVOID*)TyMemoryValues::TyFileSys_Exists, &FileSys_Exists, reinterpret_cast<LPVOID*>(&Original_tyFileSys_Exists));
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
