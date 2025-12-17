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

using namespace Structs;

namespace Hooks
{
	extern volatile long serverSendLock;
	extern volatile long clientSendLock;
	extern volatile long gumpFunctionLock;
	extern volatile long pathfindLock;
	extern volatile long macroLock;
	extern volatile unsigned int clientPacketLen;
	extern volatile unsigned int gumpFunction;
	extern volatile unsigned int gumpHandle;
	extern PathfindPointStruct *pathfindPoint;
	extern MacroStruct *macroStruct;
	extern unsigned char incomingFilters[256];
	extern unsigned char outgoingFilters[256];
	extern void __stdcall RecvHook(unsigned char *buffer, unsigned int len);
	extern void __stdcall RecvFilterHook(unsigned char *buffer, unsigned int len);
    extern void __stdcall SendHook(unsigned char *buffer, unsigned int len);
	extern void __stdcall SendFilterHook(unsigned char *buffer, unsigned int len);
	extern void __stdcall PreRecvHook();
	extern void __stdcall PreSendHook();
	extern void __stdcall CallGumpFunction();
	extern void __stdcall SendServerPacket();
	extern void __stdcall SendClientPacket();
	extern void __stdcall CallPathfindFunction();
	extern void __stdcall ExecuteMacro();
	void InstallMainHook();
	void InstallSendHook();
	void InstallRecvHook();
	BOOL Initialize();
}