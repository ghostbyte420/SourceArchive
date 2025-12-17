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

namespace Pointers
{
	extern unsigned int hookAddress;
	extern unsigned int originalDest;
	extern unsigned int macroFunction;
	extern unsigned int serverPacketSendFunction;
	extern unsigned int pathfindFunction;
	extern unsigned int pathfindType;
	extern unsigned int recvHookAddress;
	extern unsigned int recvHookType;
	extern unsigned int recvHookType2Data;
	extern unsigned int recvHookPostAddress;
	extern unsigned int sendHookAddress;
	extern unsigned int sendHookType;
	extern unsigned int sendHookPostAddress;
	extern unsigned int *networkObject;
	extern unsigned int *topGump;
	extern unsigned char *baseAddress;
	extern unsigned int ipcIndex;
	BOOL GetSendAddress();
	BOOL GetRecvAddress();
	BOOL GetPathfindAddress();
	BOOL GetTopGumpAddress();
	BOOL GetMacroAddress();
	BOOL GetHookAddress();
	BOOL GetNetworkObjectAddress();
	BOOL Initialize();
	#ifdef _DEBUG
	void LogDebugMessages();
	#endif
}