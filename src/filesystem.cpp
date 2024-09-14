#include <Windows.h>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <direct.h>

#include "filesystem.h"

IBaseFileSystem* g_pFileSystem;

// Constants for hardcoded offsets
constexpr uintptr_t CMDLIB_INITFILESYSTEM_OFFSET = 0x2DB10;
constexpr uintptr_t STEAM_DLL_STR_OFFSET = 0xCA988;
constexpr uintptr_t STEAM_ENV_FUNC_OFFSET = 0x43CB0;
constexpr uintptr_t S_BUSEVPROJECTBINDIR_OFFSET = 0x8E3DAF0;
constexpr uintptr_t FORMAT_STRING_OFFSET = 0xCA8E8;
constexpr uintptr_t VPROJECT_ADDR_OFFSET = 0x426AD + 4;
constexpr uintptr_t GPFILESYSTEM_OFFSET = 0xFAD28;
constexpr uintptr_t SUB_4E80_OFFSET = 0x4E80;
constexpr uintptr_t GPFULLFILESYSTEM_OFFSET = 0xF9530;
constexpr uintptr_t GPFILESYSTEM_VAR_OFFSET = 0xF9538;

// Helper function to write data to memory with changed protection
bool WriteMemory(void* address, const void* data, size_t size) {
    DWORD oldProtect;
    if (!VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }
    memcpy(address, data, size);
    VirtualProtect(address, size, oldProtect, &oldProtect);
    return true;
}

// Helper function to overwrite a function with a RET instruction
bool OverwriteFunctionWithRet(void* functionAddress) {
    unsigned char retInstruction = 0xC3;
    return WriteMemory(functionAddress, &retInstruction, sizeof(retInstruction));
}

bool InitFileSystem(const char* gamePath, const char* mapName) {
    // Save the original working directory
    char originalDir[MAX_PATH];
    if (_getcwd(originalDir, sizeof(originalDir)) == nullptr) {
        printf("Failed to get current working directory.\n");
        return false;
    }

    // Change directory to gamePath
    if (_chdir(gamePath) != 0) {
        printf("Failed to change directory to %s\n", gamePath);
        return false;
    }

    // Build binPath
    std::string binPath = std::string(gamePath) + "\\bin\\x64_retail";

    // Set environment variables
    _putenv_s("VPROJECT", "r1");

    // Set DLL directory
    SetDllDirectoryA(binPath.c_str());

    // Load bsppack.dll
    std::string bsppackPath = binPath + "\\bsppack.dll";
    HMODULE hBsppack = LoadLibraryA(bsppackPath.c_str());
    if (!hBsppack) {
        DWORD error = GetLastError();
        char errorMsg[256];
        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            0,
            errorMsg,
            sizeof(errorMsg),
            NULL
        );
        printf("Failed to load bsppack.dll from %s\nError: %s (Error code: %lu)\n",
            bsppackPath.c_str(), errorMsg, error);
        return false;
    }

    // Get CommandLine instance
    auto GetCommandLineFn = reinterpret_cast<ICommandLine * (*)()>(
        GetProcAddress(GetModuleHandleA("tier0.dll"), "CommandLine")
        );
    if (!GetCommandLineFn) {
        printf("Failed to get CommandLine function from tier0.dll\n");
        return false;
    }
    ICommandLine* CommandLine = GetCommandLineFn();
    CommandLine->AppendParm("-game", "r1");

    // Get the address of CmdLib_InitFileSystem
    auto pCmdLib_InitFileSystem = reinterpret_cast<char(__fastcall*)(const char*, __int64)>(
        reinterpret_cast<uintptr_t>(hBsppack) + CMDLIB_INITFILESYSTEM_OFFSET
        );

    // Modify bsppack.dll to prevent loading steam.dll
    uintptr_t steamDllStrAddr = reinterpret_cast<uintptr_t>(hBsppack) + STEAM_DLL_STR_OFFSET;
    char nullChar = '\0';
    if (!WriteMemory(reinterpret_cast<void*>(steamDllStrAddr), &nullChar, sizeof(char))) {
        printf("Failed to nullify steam.dll string.\n");
        return false;
    }

    // Disable loading steam environment by overwriting the function with RET
    uintptr_t steamEnvFuncAddr = reinterpret_cast<uintptr_t>(hBsppack) + STEAM_ENV_FUNC_OFFSET;
    if (!OverwriteFunctionWithRet(reinterpret_cast<void*>(steamEnvFuncAddr))) {
        printf("Failed to overwrite steam environment function.\n");
        return false;
    }

    // Set s_bUseVProjectBinDir to true
    uintptr_t s_bUseVProjectBinDirAddr = reinterpret_cast<uintptr_t>(hBsppack) + S_BUSEVPROJECTBINDIR_OFFSET;
    bool trueValue = true;
    if (!WriteMemory(reinterpret_cast<void*>(s_bUseVProjectBinDirAddr), &trueValue, sizeof(bool))) {
        printf("Failed to set s_bUseVProjectBinDir to true.\n");
        return false;
    }

    // Modify the format string to prevent errors
    uintptr_t formatStringAddr = reinterpret_cast<uintptr_t>(hBsppack) + FORMAT_STRING_OFFSET;
    char* formatString = reinterpret_cast<char*>(formatStringAddr);
    if (!WriteMemory(formatString + 7, &nullChar, sizeof(char))) {
        printf("Failed to modify format string.\n");
        return false;
    }

    // Call CmdLib_InitFileSystem
    pCmdLib_InitFileSystem(gamePath, -1);

    // Load filesystem_stdio.dll
    HMODULE hFilesystem = GetModuleHandleA("filesystem_stdio.dll");
    if (!hFilesystem) {
        printf("Failed to get handle to filesystem_stdio.dll\n");
        return false;
    }

    // Get g_pFileSystem
    void** g_pFileSystemPtr = reinterpret_cast<void**>(
        reinterpret_cast<uintptr_t>(hFilesystem) + GPFILESYSTEM_OFFSET
        );
    if (!g_pFileSystemPtr || !(*g_pFileSystemPtr)) {
        printf("Failed to get g_pFileSystem\n");
        return false;
    }
    void* g_pFileSystemInstance = *g_pFileSystemPtr;

    // Get AddSearchPath function from vtable
    using AddSearchPathFunc = void(__fastcall*)(void*, const char*, const char*, int64_t);
    void** vtable = *reinterpret_cast<void***>(g_pFileSystemInstance);
    AddSearchPathFunc AddSearchPath = reinterpret_cast<AddSearchPathFunc>(vtable[0x50 / sizeof(void*)]);

    // Add current directory to search paths
    AddSearchPath(g_pFileSystemInstance, ".", "MAIN", 1LL);

    // Get sub_4e80 function
    using Sub4e80Func = void(__fastcall*)(int64_t, const char*);
    Sub4e80Func sub_4e80 = reinterpret_cast<Sub4e80Func>(
        reinterpret_cast<uintptr_t>(hFilesystem) + SUB_4E80_OFFSET
        );

    // Get g_pFullFileSystem
    int64_t g_pFullFileSystem = reinterpret_cast<int64_t>(hFilesystem) + GPFULLFILESYSTEM_OFFSET;

    // Load "mp_common" map
    sub_4e80(g_pFullFileSystem, "vpk/client_mp_common.bsp");

    // Load specified map
    std::string mapPath = "vpk/client_" + std::string(mapName);
    sub_4e80(g_pFullFileSystem, mapPath.c_str());

    // Set g_pFileSystem
    g_pFileSystem = reinterpret_cast<IBaseFileSystem*>(
        reinterpret_cast<uintptr_t>(hFilesystem) + GPFILESYSTEM_VAR_OFFSET
        );
    if (!g_pFileSystem) {
        printf("Failed to set g_pFileSystem\n");
        return false;
    }

    // Restore original working directory
    _chdir(originalDir);

    return true;
}
