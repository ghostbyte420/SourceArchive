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

#include <windows.h>
using namespace Structs;

namespace IPC
{
	extern unsigned char *input;
	extern unsigned char *output;
	extern unsigned char *clientSendBuffer;
	extern unsigned char *serverSendBuffer;
	extern HANDLE inputThread; 
	extern HANDLE tempInFile;
	extern HANDLE tempOutFile;
	extern HANDLE inFileMap;
	extern HANDLE outFileMap;
	extern HANDLE tempCountFile;
	extern HANDLE countFileMap;
	extern HANDLE ghCountMutex;
	extern HANDLE inSemaphore;
	extern HANDLE outSemaphore;
	BOOL Initialize();
	void ProcessInput();
	DWORD WINAPI InputThreadProc(LPVOID lpParam);
	void Unload();
	void SetIpcInfo();
	unsigned int GetIpcCount();
	extern "C" __declspec(dllexport) BOOL __stdcall GetIpcInfo(HANDLE hProcess, IpcInfoStruct &info);
	unsigned char *FindIpcSignature(HANDLE hProcess, unsigned char *outBuffer, unsigned int len);
}