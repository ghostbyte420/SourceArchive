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
#include <intrin.h>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;
using namespace Utils;
using namespace Structs;
using namespace IPCActions;
using namespace Pointers;

namespace IPC
{
	HANDLE inputThread = NULL; 
	HANDLE tempInFile = NULL;
	HANDLE tempOutFile = NULL;
	HANDLE inFileMap = NULL;
	HANDLE outFileMap = NULL;
	HANDLE tempCountFile = NULL;
	HANDLE countFileMap = NULL;
	HANDLE ghCountMutex = NULL;
	HANDLE inSemaphore = NULL;
	HANDLE outSemaphore = NULL;
	unsigned char *input = 0;
	unsigned char *output = 0;
	unsigned char *clientSendBuffer = 0;
	unsigned char *serverSendBuffer = 0;
	DWORD inputThreadId = 0;

	unsigned int GetIpcCount()
	{
		//IpcCountMutex already exists, that means we have to open the existing filemapping instead of creating one
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			countFileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, L"CountFileMap");
			if (countFileMap == NULL)
				return 0xFFFFFFFF;
			unsigned int *count = (unsigned int *)MapViewOfFile(countFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

			if (count == NULL)
				return 0xFFFFFFFF;
			*count += 1;
			ipcIndex = *count;
			return *count;
		}
		else
		{
			TCHAR szTempFileNameCount[MAX_PATH + 1] = {0};

			unsigned int ret = GetTempFileName(L".", L"!cou", 0, szTempFileNameCount);
			if (ret == 0)
				return 0xFFFFFFFF;
			
			tempCountFile = CreateFile(szTempFileNameCount,
			FILE_ALL_ACCESS,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
			NULL);


			if (tempCountFile == INVALID_HANDLE_VALUE)
				return 0xFFFFFFFF;

			countFileMap = CreateFileMapping(tempCountFile, NULL, PAGE_EXECUTE_READWRITE, 0, 4, L"CountFileMap");
			if (countFileMap == NULL)
				return 0xFFFFFFFF;

			unsigned int *count = (unsigned int *)MapViewOfFile(countFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

			if (count == NULL)
				return 0xFFFFFFFF;

			*count = 1;
			ipcIndex = *count;
			return *count;
		}
		return 0xFFFFFFFF;
	}

/*********************************************************************************************************************************************
 * Initialize our IPC code.  The injected DLL acts as IPC server and needs to be initialized before our listener "connects".
 * Note that our IPC client will have to map their own view of the file mappings.
 *********************************************************************************************************************************************/
	BOOL Initialize()
	{
		wstring name = L"";
		TCHAR szTempFileNameInput[MAX_PATH + 1] = {0};
		TCHAR szTempFileNameOutput[MAX_PATH + 1] = {0};
		wchar_t count[8];

		clientSendBuffer = (unsigned char *)VirtualAlloc(0, 0x20000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		serverSendBuffer = (unsigned char *)VirtualAlloc(0, 0x20000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		
		if (clientSendBuffer == NULL || serverSendBuffer == NULL)
			return FALSE;

		ghCountMutex = CreateMutex(NULL, true, L"IpcCountMutex");
		if (ghCountMutex == NULL)
			return FALSE;

		//scoped lock
		MutexLock countLock(ghCountMutex);

		unsigned int ipcCount = GetIpcCount();
		if (ipcCount == 0xFFFFFFFF)
			return FALSE;

		swprintf(count, 8, L"%u", ipcCount);

		if (0 == GetTempFileName(L".", L"!inp", 0, szTempFileNameInput))
			return FALSE;
		if (0 == GetTempFileName(L".", L"!out", 0, szTempFileNameOutput))
			return FALSE;

		tempInFile = CreateFile(szTempFileNameInput,
			FILE_ALL_ACCESS,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
			NULL);

		tempOutFile = CreateFile(szTempFileNameOutput,
			FILE_ALL_ACCESS,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
			NULL);

		if (tempInFile == INVALID_HANDLE_VALUE || tempOutFile == INVALID_HANDLE_VALUE)
			return FALSE;

		name = L"UltimaSemaphoreInput";
		name.append(count);
		inSemaphore = CreateSemaphore(NULL, 0, 3, name.c_str());

		name = L"UltimaSemaphoreOutput";
		name.append(count);
		outSemaphore = CreateSemaphore(NULL, 0, 3, name.c_str());

		if (inSemaphore == NULL || outSemaphore == NULL)
			return FALSE;

		name = L"UltimaFileMapInput";
		name.append(count);
		inFileMap = CreateFileMapping(tempInFile, NULL, PAGE_EXECUTE_READWRITE, 0, 0x20000, name.c_str());

		name = L"UltimaFileMapOutput";
		name.append(count);
		outFileMap = CreateFileMapping(tempOutFile, NULL, PAGE_EXECUTE_READWRITE, 0, 0x20000, name.c_str());

		if (inFileMap == NULL || outFileMap == NULL)
			return FALSE;

		input = (unsigned char *)MapViewOfFile(inFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		output = (unsigned char *)MapViewOfFile(outFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

		if (input == NULL || output == NULL)
			return FALSE;

		// first DWORD = lock.  setting to zero gives write access first
		memset(input, 0, 4);
		memset(output, 0, 4);

		inputThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InputThreadProc, NULL, 0, &inputThreadId);

		if( inputThread == NULL)
			return FALSE;

		SetIpcInfo();

		#ifdef _DEBUG
		stringstream ss;
		ss << "ipcCount = " << ipcCount;
		Log(ss.str());
		#endif

		return TRUE;
	}

/*********************************************************************************************************************************************
 * This performs the actual command parsing for receiving commands from our IPC client.
 *********************************************************************************************************************************************/
	void ProcessInput(unsigned char* rawMessage, unsigned int len, unsigned char messageType)
	{
		switch (messageType)
		{
			case IncomingMessageType::IncomingMessage:
				{
				//LPWSTR message = (LPWSTR)(input + 9);
				//TODO: things.
				break;
				}
			case IncomingMessageType::SendClientPacket:
				{
					SendPacketToClient(rawMessage, len);
					break;
				}
			case IncomingMessageType::SendServerPacket:
				{
					SendPacketToServer(rawMessage, len);
					break;
				}
			case IncomingMessageType::CallGumpFunction:
				{
					CallGumpFunctionStruct cg;
					cg = *(CallGumpFunctionStruct*)rawMessage;
					if (cg.gumpHandle == 0)
						ExecuteGumpFunction(*topGump, cg.functionIndex);
					else
						ExecuteGumpFunction(cg.gumpHandle, cg.functionIndex);
					break;
				}
			case IncomingMessageType::Pathfind:
				{
					PathfindStruct p;
					p = *(PathfindStruct*)rawMessage;
					ExecutePathfindFunction(p);
					break;
				}
			case IncomingMessageType::Type1Macro:
				{
					Type1MacroStruct m;
					m = *(Type1MacroStruct*)rawMessage;
					ExecuteType1Macro(m.arg1, m.arg2);
					break;
				}
			case IncomingMessageType::Type2Macro:
				{
					Type2MacroStruct m;
					m = *(Type2MacroStruct*)rawMessage;
					ExecuteType2Macro(m.arg1, m.arg2, rawMessage + 10, m.textLen);
					break;
				}
			case IncomingMessageType::AddIncomingPacketFilter:
				{
					AddIncomingPacketType(*rawMessage);
					break;
				}
			case IncomingMessageType::AddOutgoingPacketFilter:
				{
					AddOutgoingPacketType(*rawMessage);
					break;
				}
			case IncomingMessageType::RemoveIncomingPacketFilter:
				{
					RemoveIncomingPacketType(*rawMessage);
					break;
				}
			case IncomingMessageType::RemoveOutgoingPacketFilter:
				{
					RemoveOutgoingPacketType(*rawMessage);
					break;
				}
			case IncomingMessageType::PatchEncryption:
				{
					PatchClientEncryption();
					break;
				}
			case IncomingMessageType::SetLoginServer:
				{
					LoginServerStruct l = *(LoginServerStruct*)rawMessage;
					PatchLoginServer(l.server, l.port);
					break;
				}
		}
		return;
	}

/*********************************************************************************************************************************************
 * This is executed from a worker thread and is used for processing incoming IPC commands.
 *********************************************************************************************************************************************/
	DWORD WINAPI ProcessThreadProc(void *lpParam)
	{
		ProcessThreadStruct *p = (ProcessThreadStruct*)lpParam;
		ProcessInput(p->rawMessage, p->len, p->messageType);
		delete (p->rawMessage);
		delete (p);
		return 0;
	}

/*********************************************************************************************************************************************
 * This runs on a separate thread and monitors for commands from our IPC client.
 *********************************************************************************************************************************************/
	DWORD WINAPI InputThreadProc(void *lpParam)
	{
		UNREFERENCED_PARAMETER(lpParam);
		DWORD dwWaitResult; 

		volatile long *lock = (long *)input;
		for (;;)
		{
			dwWaitResult = WaitForSingleObject(inSemaphore, INFINITE);

			if (dwWaitResult == WAIT_OBJECT_0)
			{
				// 0 = free for write access, 1 = in use, 2 = free for read access
				// scoped lock
				//AtomicLock atomicLock(lock, 2);
				while (2 != _InterlockedCompareExchange(lock, 1, 2))
					Sleep(1);
				unsigned int len = *((unsigned int*)&input[5]);
				unsigned char *rawMessage = new unsigned char[len];
				unsigned char messageType = input[4];
				memcpy(rawMessage, &input[9], len);
				// Grab data from shared buffer and release it ASAP
				_InterlockedExchange(lock, 0);
				// Spawn a worker thread to avoid blocking this one
				ProcessThreadStruct *p = new ProcessThreadStruct;
				p->rawMessage = rawMessage;
				p->messageType = messageType;
				p->len = len;
				DWORD id = 0;
				HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ProcessThreadProc, (void*)p, 0, &id);
				// We don't need the handle, just get rid of it now so the thread can be cleaned up when it returns (if it hasn't already)
				CloseHandle(hThread);
			}
		}
		return TRUE;
	}

/*********************************************************************************************************************************************
 * Searches remote process (client.exe after it's been injected) for our IPC information struct.
 * outBuffer MUST be 0x1000 in length.
 * Returns a pointer to the signature location within outBuffer.
 * Returns 0xFFFFFFFFF on error.
 * This should be quite fast as it only looks at allocated memory sections which are 0x1000 in length.  This is the size of our allocated
 * memory containing the information struct. On my system it returns a match in less than 1ms and searches the entire 2GB of address
 * space in only 2ms.
 *********************************************************************************************************************************************/
	unsigned char *FindIpcSignature(HANDLE hProcess, unsigned char *outBuffer, unsigned int len)
	{
		if (len != 0x1000)
			return (unsigned char*)0xFFFFFFFF;
		//build ipc signature, must be same as one in SetIpcInfo()
		unsigned char sig[16];
		for (int x = 0; x < 16; x++)
			sig[x] = (x * 15) + 7;
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		unsigned char *address = (unsigned char *)0x10000;
		unsigned char *endAddress = (unsigned char *)0x7FFFFFFF;

		MEMORY_BASIC_INFORMATION mbi;
		unsigned char *offset = 0;
		DWORD oldProtect = 0;
		unsigned long read = 0;

		while (address < endAddress)
		{
			if (0 != VirtualQueryEx(hProcess, address, &mbi, sizeof(MEMORY_BASIC_INFORMATION)))
			{
				if (mbi.State == MEM_COMMIT && mbi.AllocationProtect == PAGE_EXECUTE_READWRITE && mbi.RegionSize == len) // we're only interested in these memory sections
				{
					if (ReadProcessMemory(hProcess, (LPVOID)address, outBuffer, len, &read) && read == len)
					{
						offset = FindSignatureOffset(sig, 16, outBuffer, len);
						if (offset != 0)
							return offset;
					}
				}
				if (mbi.RegionSize > 0)
					address += mbi.RegionSize; // we simply skip over most of the memory
				else
					address += si.dwPageSize;
			}
			else // VirtualQueryEx failed, check next memory page
				address += si.dwPageSize;
		}
		return (unsigned char *)0xFFFFFFFF;
	}

/*********************************************************************************************************************************************
 * Returns (if present) the IPC info structure from an external process.
 *********************************************************************************************************************************************/
	extern "C" __declspec(dllexport) BOOL __stdcall GetIpcInfo(HANDLE hProcess, IpcInfoStruct &info)
	{
		unsigned char *buffer = new unsigned char[0x1000];
		unsigned char *offset = FindIpcSignature(hProcess, buffer, 0x1000);

		if (offset != (unsigned char *)0xFFFFFFFF)
		{
			// this value tells us struct is done being copied to memory
			if (*(unsigned int*)(offset + 0x29A) = 0xDEADFED5)
			{
				// signature length = 16
				offset += 16;
				memcpy(&info, offset, sizeof(IpcInfoStruct));
				delete(buffer);
				return TRUE;
			}
		}
		delete(buffer);
		return FALSE;
	}

/*********************************************************************************************************************************************
 * We put a struct in memory which holds our IPC handles.
 * This struct is located later according to a byte signature which identifies it.
 * This is extremely hackish, but it simplifies things.  Anyone else is free to do a proper method.
 *********************************************************************************************************************************************/
	void SetIpcInfo()
	{
		/* Build our signature.  It's dynamically created so we don't pick it up in the resources section of our injected DLL
		 * when we search for it in memory later */
		unsigned char sig[16];
		for (int x = 0; x < 16; x++)
			sig[x] = (x * 15) + 7;
		// Later on we search for a memory page with exactly these attributes, greatly speeding up the search
		unsigned char *mem = (unsigned char *)VirtualAlloc(0, 0x1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		memcpy(mem, &sig[0], 16);
		IpcInfoStruct info;
		info.inFileMap = (DWORD)inFileMap;
		info.outFileMap = (DWORD)outFileMap;
		info.inSemaphore = (DWORD)inSemaphore;
		info.outSemaphore = (DWORD)outSemaphore;
		memcpy(mem + 16, &info, sizeof(IpcInfoStruct));
		// We put a value here indicating that we're done writing entire struct
		*(unsigned int*)(mem + 0x29A) = 0xDEADFED5; // No FED5 were harmed in the coding of this project
		return;
	}


}