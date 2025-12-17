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
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
using namespace Pointers;
using namespace std;

namespace Utils
{
	#ifdef _DEBUG
	CRITICAL_SECTION logCS;
	#endif

/*********************************************************************************************************************************************
 * Little function to log messages.  Used in tracing.  Critical section protects against race conditions if multiple threads call it at
 * the same time.  Only available if MasterControl.dll is compiled in DEBUG configuration.
 *********************************************************************************************************************************************/
	#ifdef _DEBUG
	void Log(std::string message)
	{
		EnterCriticalSection(&logCS);
		message.append("\r\n");
		fstream fs(L"log.txt", ios::out | ios::binary | ios::app);
		fs.write(message.c_str(), message.length());
		fs.close();
		LeaveCriticalSection(&logCS);
		return;
	}
	#endif

/*********************************************************************************************************************************************
 * Search memory for specified byte signature.  Note that 0xCC is a wildcard byte, it matches anything.  A more proper way to do this is
 * one pointer to a signature and another pointer to wildcard flags, but this is simpler and works well.
 * Returns a pointer to the beginning of the signature match.  Returns 0 on no match.
 *********************************************************************************************************************************************/
	unsigned char *FindSignatureOffset(unsigned char *sigBuffer, unsigned int sigLen, unsigned char *buffer, unsigned int bufferLen)
	{
		BOOL found = FALSE;
		for (unsigned int x = 0; x < bufferLen - sigLen; x++)
		{
			for(unsigned int y = 0; y < sigLen; y++)
			{
				if (sigBuffer[y] == 0xCC || buffer[x + y] == sigBuffer[y])
					found = TRUE;
				else
				{
					found = FALSE;
					break;
				}
			}
			if (found)
			{
				return buffer + x;
			}
		}
		return 0;
	}

/*********************************************************************************************************************************************
 * Assemble CALL or JMP instructions as needed.  Please be a good person and pass a valid pointer to a buffer which is >=5 bytes in length.
 *********************************************************************************************************************************************/
	void CreateCALL(void *sourceAddress, void *targetAddress, CallType callType, unsigned char *callResult)
	{
		unsigned int offset = (unsigned int)targetAddress - (unsigned int)sourceAddress - 5;
		switch (callType)
		{
		case CallType::CALL:
			callResult[0] = 0xE8;
			break;
		case CallType::JMP:
			callResult[0] = 0xE9;
			break;
		}
		*(unsigned int*)&callResult[1] = offset;
		return;
	}

/*********************************************************************************************************************************************
 * Here we change the protection of memory pages for client.exe.  The main executable code (.text) is PAGE_EXECUTE_READ which prevents
 * us from patching the memory.  We need PAGE_EXECUTE_READWRITE access.
 *********************************************************************************************************************************************/
	void GainMemoryAccess(unsigned char *address, unsigned int len)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		// Round up to page boundary
		unsigned long mod = (unsigned long)address % si.dwPageSize;
		if (mod > 0)
			address += (si.dwPageSize - mod);

		MEMORY_BASIC_INFORMATION mbi;
		unsigned char *endAddress = address + len;
		unsigned int ret = 0;
		DWORD oldProtect = 0;

		while (address < endAddress)
		{
			ret = VirtualQuery(address, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
			if (ret != 0)
			{
				if (mbi.State == MEM_COMMIT)
					VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &oldProtect);
				if (mbi.RegionSize > 0)
					address += mbi.RegionSize;
				else
					address += si.dwPageSize;
			}
			else
				address += si.dwPageSize;
		}
		return;
	}

/*********************************************************************************************************************************************
 * Grant SeDebugPrivilege to client.exe.  Not needed right now.  Added in for previous testing.  Should probably be removed later.
 *********************************************************************************************************************************************/
	int ActivateSeDebugPrivilege()
	{
		HANDLE hToken;
		LUID id;
		TOKEN_PRIVILEGES tp;

		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
			return(GetLastError());

		if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &id))
			return(GetLastError());

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = id;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL))
			return(GetLastError());

		CloseHandle(hToken);
		return 0;
	}

/*********************************************************************************************************************************************
 * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *
 *********************************************************************************************************************************************/

	void Initialize()
	{
		#ifdef _DEBUG
		InitializeCriticalSection(&logCS);
		char t[64];
		SYSTEMTIME time;
		GetLocalTime(&time);
		sprintf(t, "%02d-%02d-%d %02d:%02d:%02d", time.wMonth, time.wDay, time.wYear, time.wHour, time.wMinute, time.wSecond);
		stringstream ss;
		ss << "Logging started at: " << t << "\r\n";
		Log(ss.str());
		#endif

		GainMemoryAccess((unsigned char *)0x400000, 0x200000);
		return;
	}

}