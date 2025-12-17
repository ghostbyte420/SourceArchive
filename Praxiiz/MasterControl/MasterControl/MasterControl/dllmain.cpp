/* Copyright (C) 2012 Matthew Geyer
 * 
 * This file is part of MasterControl.
 * 
 * MasterControl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * MasterControl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MasterControl.  If not, see <http://www.gnu.org/licenses/>. */

#include "stdafx.h"

extern "C" __declspec(dllexport) void Initialize()
{
	Utils::Initialize();

	if (!Pointers::Initialize())
	{
		#ifdef _DEBUG
		Utils::Log("Pointers::Initialize() failed.");
		#endif
		return;
	}
	if (!Patches::Initialize())
	{
		#ifdef _DEBUG
		Utils::Log("Patches::Initialize() failed.");
		#endif
		return;
	}
	if (!Hooks::Initialize())
	{
		#ifdef _DEBUG
		Utils::Log("Hooks::Initialize() failed.");
		#endif
		return;
	}
	if (!IPC::Initialize())
	{
		#ifdef _DEBUG
		Utils::Log("IPC::Initialize() failed.");
		#endif
		return;
	}
	return;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			return TRUE;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
		default:
			return FALSE;
	}
}

