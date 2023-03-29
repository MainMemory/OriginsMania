#include "pch.h"
#include "HiteModLoader.h"
#include "Helpers.h"
#include "SigScan.h"

void(__fastcall* LinkGameLogicDLL)(void* engineInfo);
HOOK(void, __fastcall, LinkGameLogic, SigLinkGameLogic(), void* engineInfo)
{
	WRITE_MEMORY(((intptr_t)originalLinkGameLogic + 0x14E), 0x90, 0x90, 0x90);
	LinkGameLogicDLL(engineInfo);
	originalLinkGameLogic(engineInfo);
}

extern "C" __declspec(dllexport) void Init(ModInfo * modInfo)
{
	// Check signatures
	if (!SigValid)
	{
		MessageBoxW(nullptr, L"Signature Scan Failed!\n\nThis usually means there is a conflict or the mod is running on an incompatible game version.", L"Scan Error", MB_ICONERROR);
		return;
	}

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