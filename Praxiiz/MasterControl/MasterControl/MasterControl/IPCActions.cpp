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

#include "StdAfx.h"
#include <intrin.h>
using namespace IPC;
using namespace Pointers;
using namespace Hooks;
using namespace Utils;

namespace IPCActions
{
	void SendRawOutputMessage(OutgoingMessageType messageType, unsigned char *buffer, unsigned int len)
	{
		volatile long* lock = (long*)output;
		// 0 = free for write access, 1 = in use, 2 = free for read access
		while (0 != _InterlockedCompareExchange(lock, 1, 0))
			Sleep(1);
		output[4] = messageType;
		*(unsigned int*)&output[5] = len;
		memcpy(&output[9], buffer, len);
		_InterlockedExchange(lock, 2);
		ReleaseSemaphore(outSemaphore, 1, 0);
		return;
	}

	void SendPacketToClient(unsigned char *source, unsigned int len)
	{
		while (0 != _InterlockedCompareExchange(&clientSendLock, 1, 0))
			Sleep(1);
		memcpy(clientSendBuffer, source, len);
		clientPacketLen = len;
		_InterlockedExchange(&clientSendLock, 2);
		return;
	}

	void SendPacketToServer(unsigned char *source, unsigned int len)
	{
		while (0 != _InterlockedCompareExchange(&serverSendLock, 1, 0))
			Sleep(1);
		memcpy(serverSendBuffer, source, len);
		_InterlockedExchange(&serverSendLock, 2);
		return;
	}

	void ExecuteGumpFunction(unsigned int gump, unsigned int vTableOffset)
	{
		while (0 != _InterlockedCompareExchange(&gumpFunctionLock, 1, 0))
			Sleep(1);
		gumpHandle = gump;
		gumpFunction = vTableOffset;
		_InterlockedExchange(&gumpFunctionLock, 2);
		return;
	}

	void ExecutePathfindFunction(PathfindStruct p)
	{
		while (0 != _InterlockedCompareExchange(&pathfindLock, 1, 0))
			Sleep(1);
		pathfindPoint->x = p.x;
		pathfindPoint->y = p.y;
		pathfindPoint->z = p.z;
		_InterlockedExchange(&pathfindLock, 2);
		return;
	}

	void ExecuteType1Macro(unsigned int arg1, unsigned short arg2)
	{
		while (0 != _InterlockedCompareExchange(&macroLock, 1, 0))
			Sleep(1);
		macroStruct->arg1 = arg1;
		macroStruct->arg2 = arg2;
		_InterlockedExchange(&macroLock, 2);
		return;
	}

	void ExecuteType2Macro(unsigned int arg1, unsigned short arg2, unsigned char* textBuffer, unsigned int textLen)
	{
		while (0 != _InterlockedCompareExchange(&macroLock, 1, 0))
			Sleep(1);
		macroStruct->arg1 = arg1;
		macroStruct->arg2 = arg2;
		memset(&macroStruct->text[0], 0, 256);
		if (textLen > 256)
			textLen = 256;
		if (textLen > 254)
			memset(&textBuffer[textLen - 1], 0, 2); // protect against potential BOF
		memcpy(&macroStruct->text[0], textBuffer, textLen);
		_InterlockedExchange(&macroLock, 2);
		return;
	}

	void AddIncomingPacketType(unsigned char packet)
	{
		incomingFilters[packet] = 1;
		return;
	}

	void AddOutgoingPacketType(unsigned char packet)
	{
		outgoingFilters[packet] = 1;
		return;
	}

	void RemoveIncomingPacketType(unsigned char packet)
	{
		incomingFilters[packet] = 0;
		return;
	}

	void RemoveOutgoingPacketType(unsigned char packet)
	{
		outgoingFilters[packet] = 0;
		return;
	}

	void PatchClientEncryption()
	{
		Patches::LoginEncryption();
		Patches::TwoFishEncryption();
		Patches::ProtocolDecryption();
		return;
	}

	void PatchLoginServer(unsigned int server, unsigned short port)
	{
		Patches::LoginServer(server, port);
		return;
	}
}