#include "pch.h"
#include "HiteModLoader.h"
#include "Helpers.h"
#include "SigScan.h"

#undef STATUS_TIMEOUT
#ifndef RETRO_REVISION
#define RETRO_REVISION (3)
#endif

#define RETRO_REV01 (RETRO_REVISION == 1)

// RSDKv5 was revised & updated for plus onwards
// this is basically the same functionality as "MANIA_USE_PLUS"
// but I split this one to be more specific about engine changes vs game changes
#define RETRO_REV02 (RETRO_REVISION >= 2)

// RSDKv5 was revised & updated to v5U for sonic origins
// enabling this will add the RSDKv5U funcs and allow this to run properly on that engine ver
#define RETRO_REV0U (RETRO_REVISION >= 3)


// Controls whether EditorLoad & EditorDraw should be included in the final product or not
// This is a copy of what the original game likely had, as the original game does not include EditorLoad or EditorDraw funcs for any objects
#ifndef RETRO_INCLUDE_EDITOR
#define RETRO_INCLUDE_EDITOR (1)
#endif

#ifndef RETRO_USE_MOD_LOADER
#define RETRO_USE_MOD_LOADER (0)
#endif

#ifndef RETRO_MOD_LOADER_VER
#define RETRO_MOD_LOADER_VER (2)
#endif

// used to manage standalone (RSDKv5.exe & Game.dll) and combined (Game.exe) modes
#ifndef RETRO_STANDALONE
#define RETRO_STANDALONE (1)
#endif

// -------------------------
// GAME VERSIONS
// -------------------------

#define VER_100 (0) // 1.00 (initial console release)
#define VER_103 (3) // 1.03 (PC release/console patch)
#define VER_105 (5) // 1.04/1.05
#define VER_106 (6) // 1.06 (steam denuvo removal update)
#define VER_107 (7) // 1.07 (EGS/Origin releases)

#if MANIA_PREPLUS

#ifdef MANIA_FIRST_RELEASE
#define GAME_VERSION VER_100
#else
#ifndef GAME_VERSION
#define GAME_VERSION VER_103
#endif
#endif

#undef RETRO_REV02
#define RETRO_REV02 (0)
#else
#ifndef GAME_VERSION
#define GAME_VERSION VER_106
#endif
#endif

#define MANIA_USE_PLUS (GAME_VERSION >= VER_105)
#define MANIA_USE_EGS  (GAME_VERSION == VER_107)
#include "GameLink.h"

void RegisterObjectMod(void** staticVars, const char* name, uint32 entityClassSize, uint32 staticClassSize, void (*update)(void),
	void (*lateUpdate)(void), void (*staticUpdate)(void), void (*draw)(void), void (*create)(void*), void (*stageLoad)(void),
	void (*editorDraw)(void), void (*editorLoad)(void), void (*serialize)(void), void (*staticLoad)(void* staticVars)) {}

char userFileDir[MAX_PATH];

bool32 RSDK_LoadUserFileMod(const char* filename, void* buffer, uint32 bufSize)
{
    char fullFilePath[0x400];
#if RETRO_USE_MOD_LOADER
    if (strlen(customUserFileDir))
        sprintf_s(fullFilePath, sizeof(fullFilePath), "%s%s", customUserFileDir, filename);
    else
        sprintf_s(fullFilePath, sizeof(fullFilePath), "%s%s", userFileDir, filename);
#else
    sprintf_s(fullFilePath, sizeof(fullFilePath), "%s%s", userFileDir, filename);
#endif
    //PrintLog(PRINT_NORMAL, "Attempting to load user file: %s", fullFilePath);

    FILE* file = fopen(fullFilePath, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        uint32 fSize = (uint32)ftell(file);
        fseek(file, 0, SEEK_SET);
        uint32 size = bufSize;
        if (bufSize > fSize)
            size = fSize;
        fread(buffer, 1, size, file);
        fclose(file);

        return true;
    }
    else {
        //PrintLog(PRINT_NORMAL, "Nope!");
    }

    return false;
}

void API_LoadUserFileMod(const char* name, void* buffer, uint32 size, void (*callback)(int32 status))
{
    int32 status = STATUS_OK;
    if (!RSDK_LoadUserFileMod(name, buffer, size))
        status = STATUS_NOTFOUND;
    if (callback)
        callback(status);
}

bool32 RSDK_SaveUserFileMod(const char* filename, void* buffer, uint32 bufSize)
{
    char fullFilePath[0x400];
#if RETRO_USE_MOD_LOADER
    if (strlen(customUserFileDir))
        sprintf_s(fullFilePath, sizeof(fullFilePath), "%s%s", customUserFileDir, filename);
    else
        sprintf_s(fullFilePath, sizeof(fullFilePath), "%s%s", userFileDir, filename);
#else
    sprintf_s(fullFilePath, sizeof(fullFilePath), "%s%s", userFileDir, filename);
#endif
    //PrintLog(PRINT_NORMAL, "Attempting to save user file: %s", fullFilePath);

    FILE* file = fopen(fullFilePath, "wb");
    if (file) {
        fwrite(buffer, 1, bufSize, file);
        fclose(file);

        return true;
    }
    else {
        //PrintLog(PRINT_NORMAL, "Nope!");
    }
    return false;
}

void API_SaveUserFileMod(const char* name, void* buffer, uint32 size, void (*callback)(int32 status), bool32 compressed)
{
    int32 status = STATUS_OK;
    if (!RSDK_SaveUserFileMod(name, buffer, size))
        status = STATUS_ERROR;
    if (callback)
        callback(status);
}

void API_DeleteUserFileMod(const char* name, void (*callback)(int32 status))
{
    char fullFilePath[0x400];
#if RETRO_USE_MOD_LOADER
    if (strlen(customUserFileDir))
        sprintf_s(fullFilePath, sizeof(fullFilePath), "%s%s", customUserFileDir, name);
    else
        sprintf_s(fullFilePath, sizeof(fullFilePath), "%s%s", userFileDir, name);
#else
    sprintf_s(fullFilePath, sizeof(fullFilePath), "%s%s", userFileDir, name);
#endif
    //PrintLog(PRINT_NORMAL, "Attempting to delete user file: %s", fullFilePath);
    int32 status = remove(fullFilePath);

    if (callback)
        callback(status == 0 ? STATUS_OK : STATUS_ERROR);
}

void(__fastcall* LinkGameLogicDLL)(EngineInfo* engineInfo);
HOOK(void, __fastcall, LinkGameLogic, SigLinkGameLogic(), EngineInfo* engineInfo)
{
	auto functbl = (RSDKFunctionTable*)engineInfo->functionTable;
    auto apitbl = (APIFunctionTable*)engineInfo->APITable;

    auto loadfilersdk = functbl->LoadUserFile;
    auto loadfileapi = apitbl->LoadUserFile;
    functbl->LoadUserFile = RSDK_LoadUserFileMod;
    apitbl->LoadUserFile = API_LoadUserFileMod;
    auto savefilersdk = functbl->SaveUserFile;
    auto savefileapi = apitbl->SaveUserFile;
    functbl->SaveUserFile = RSDK_SaveUserFileMod;
    apitbl->SaveUserFile = API_SaveUserFileMod;
    auto delfileapi = apitbl->DeleteUserFile;
    apitbl->DeleteUserFile = API_DeleteUserFileMod;

	LinkGameLogicDLL(engineInfo);

    functbl->LoadUserFile = loadfilersdk;
    apitbl->LoadUserFile = loadfileapi;
    functbl->SaveUserFile = savefilersdk;
    apitbl->SaveUserFile = savefileapi;
    apitbl->DeleteUserFile = delfileapi;

    auto regobj = functbl->RegisterObject;
	functbl->RegisterObject = RegisterObjectMod;

    originalLinkGameLogic(engineInfo);
	
    functbl->RegisterObject = regobj;
}

extern "C" __declspec(dllexport) void Init(ModInfo * modInfo)
{
	// Check signatures
	if (!SigValid)
	{
		MessageBoxW(nullptr, L"Signature Scan Failed!\n\nThis usually means there is a conflict or the mod is running on an incompatible game version.", L"Scan Error", MB_ICONERROR);
		return;
	}

    sprintf_s(userFileDir, "%s\\", modInfo->CurrentMod->Path);

	char dllpath[MAX_PATH]{};
	sprintf_s(dllpath, "%s\\Game.dll", modInfo->CurrentMod->Path);

	HMODULE hmod = LoadLibraryA(dllpath);
	if (!hmod)
	{
		MessageBoxW(nullptr, L"Failed to load Game.dll!\n\nPlease place Game.dll in the mod's folder.", L"DLL Load Error", MB_ICONERROR);
		return;
	}

	LinkGameLogicDLL = (LinkGameLogic*)GetProcAddress(hmod, "LinkGameLogicDLL");

	if (!LinkGameLogicDLL)
	{
		MessageBoxW(nullptr, L"Failed to find LinkGameLogicDLL function in Game.dll!", L"DLL Load Error", MB_ICONERROR);
		return;
	}

	INSTALL_HOOK(LinkGameLogic);
}