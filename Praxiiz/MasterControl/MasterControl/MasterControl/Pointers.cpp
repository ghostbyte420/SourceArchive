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
using namespace Utils;
using namespace std;

/*********************************************************************************************************************************************
 * This namespace is all about managing the various addresses we'll use in other parts of the DLL.  Here the addresses are defined
 * and also the logic for tracking down the addresses across multiple versions of client.exe.
 *********************************************************************************************************************************************/
namespace Pointers
{
	unsigned int hookAddress = 0;
	unsigned int originalDest = 0;  // place to jump after main hook
	unsigned int macroFunction = 0;
	unsigned int serverPacketSendFunction = 0;
	unsigned int pathfindFunction = 0;
	unsigned int pathfindType = 0;
	unsigned int recvHookAddress = 0;
	unsigned int recvHookType = 0;
	unsigned int recvHookType2Data = 0; // this is part of the instruction we overwrote with our hook.  We have to restore it.
	unsigned int recvHookPostAddress = 0; // place to JMP after our recv hook
	unsigned int sendHookAddress = 0;
	unsigned int sendHookType = 0;
	unsigned int sendHookPostAddress = 0; // place to JMP after our send hook
	unsigned int *networkObject = 0;
	unsigned int *topGump = 0;
	unsigned char *baseAddress = (unsigned char *)0x400000;
	unsigned int ipcIndex = 0;

/*********************************************************************************************************************************************
 * This finds the address for the function used to send packets to the server.
 *********************************************************************************************************************************************/
	BOOL GetSendAddress()
	{
		unsigned char sig1[] = { 
			0x8D, 0x8B, 0x94, 0x00, 0x00, 0x00, 0xE8, 0xCC, 
			0xCC, 0xCC, 0xCC, 0x55, 0x8D, 0x8B, 0xBC, 0x00, 
			0x00, 0x00 };

		unsigned char sig2[] = { 
			0x0F, 0xB7, 0xD8, 0x0F, 0xB6, 0x06, 0x83, 0xC4, 
			0x04, 0x53, 0x50, 0x8D, 0x4F, 0x6C };

		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);

		if (offset != 0)
		{
			sendHookAddress = (unsigned int)offset + 0x0B;
			sendHookPostAddress = sendHookAddress + 7;  // we overwrite 7 bytes of instructions
			sendHookType = 1;
			serverPacketSendFunction = (unsigned int)offset - 0x22;
			return TRUE;
		}
		else
		{
			offset = FindSignatureOffset(sig2, sizeof(sig2), baseAddress, 0x200000);
			if (offset != 0)
			{
				sendHookAddress = (unsigned int)offset + 9;
				sendHookPostAddress = sendHookAddress + 5;
				sendHookType = 2;
				serverPacketSendFunction = (unsigned int)offset - 0xF;
				return TRUE;
			}
		}
		return FALSE;
	}

/*********************************************************************************************************************************************
 * This finds the address for the function used to handle received packets.  We use this to hook incoming packets and to send packets
 * to the game client.
 *********************************************************************************************************************************************/
	BOOL GetRecvAddress()
	{
		unsigned char sig1[] = { 0x53, 0x56, 0x57, 0x8B, 0xF9, 0x8B, 0x0D, 0xCC, 
			0xCC, 0xCC, 0xCC, 0x33, 0xD2 };

		unsigned char sig2[] = { 
			0xA1, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 
			0xC7, 0x05, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 
			0xCC, 0xCC, 0x8B, 0x38, 0x8B, 0xCC, 0xBE };

		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);

		if (offset != 0)
		{
			recvHookAddress = (unsigned int)offset;
			recvHookPostAddress = recvHookAddress + 5;
			recvHookType = 1;
			return TRUE;
		}
		else
		{
			offset = FindSignatureOffset(sig2, sizeof(sig2), baseAddress, 0x200000);
			if (offset != 0)
			{
				recvHookAddress = (unsigned int)offset;
				recvHookPostAddress = recvHookAddress + 5;
				recvHookType = 2;
				/* Original instruction = MOV EAX,DWORD PTR DS:[7F49B4]
				 * I know it's a MOV EAX, but I need to grab that address to restore the instruction later */
				recvHookType2Data = *(unsigned int *)(offset + 1);
				return TRUE;
			}
		}
		return FALSE;
	}

/*********************************************************************************************************************************************
 * This finds the address for the pathfind function
 *********************************************************************************************************************************************/
	BOOL GetPathfindAddress()
	{
		unsigned char sig1[] = { 0x0F, 0xBF, 0x68, 0x24, 0x0F, 0xBF, 0x50, 0x26 };
		unsigned char sig2[] = { 0x0F, 0xBF, 0x50, 0x26, 0x53, 0x8B, 0x1D };
		pathfindFunction = (unsigned int)FindSignatureOffset(sig1, sizeof(sig1), (unsigned char *)0x401000, 0x200000);

		if (pathfindFunction == 0)
		{
			pathfindFunction = (unsigned int)FindSignatureOffset(sig2, sizeof(sig2), (unsigned char *)0x401000, 0x200000);
			if (pathfindFunction == 0)
			{
				return FALSE;
			}
			else
			{
				pathfindType = 2;
			}
		}
		else
		{
			pathfindType = 1;
		}
		return TRUE;
	}

/*********************************************************************************************************************************************
 * This finds a pointer which always holds a handle to the topmost gump.  Gump may be custom one sent from server or an
 * internal one from the client.
 *********************************************************************************************************************************************/
	BOOL GetTopGumpAddress()
	{
		//3.x - 5.x:
		unsigned char sig1[] = { 0x8B, 0x44, 0x24, 0x04, 0xC7, 0x41 };
		//6.x - 7.x:
		unsigned char sig2[] = { 0x8B, 0x44, 0x24, 0x04, 0x85, 0xC0, 0x89, 0x41, 0x64 };

		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);
		if (offset == 0)
		{
			offset = FindSignatureOffset(sig2, sizeof(sig2), baseAddress, 0x200000);
			if (offset == 0)
			{
				return false;
			}
			else
			{
				topGump = (unsigned int *)*(unsigned int *)(offset + 0x14);
			}
		}
		else
		{
			topGump = (unsigned int *)*(unsigned int *)(offset + 0x14);
		}

		return TRUE;
	}

/*********************************************************************************************************************************************
 * This finds the address to use for calling EUO style client macros
 *********************************************************************************************************************************************/
	BOOL GetMacroAddress()
	{
		//4.x - 5.x:
		unsigned char sig1[] = { 
             0x50, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0x64, 0x89, 
             0x25, 0x00, 0x00, 0x00, 0x00, 0xE8, 0xCC, 0xCC, 
             0xCC, 0xCC, 0x8B, 0x84, 0x24, 0xCC, 0xCC, 0xCC, 
			 0xCC, 0x8B, 0x8C, 0x24 };
		//6.x - 7.x:
		unsigned char sig2[] = { 
             0x50, 0xB8, 0xCC, 0xCC, 0xCC, 0xCC, 0xE8, 0xCC, 
             0xCC, 0xCC, 0xCC, 0xA1, 0xCC, 0xCC, 0xCC, 0xCC, 
             0x33, 0xC4, 0x89, 0x84, 0x24, 0xCC, 0xCC, 0xCC, 
             0xCC, 0x56, 0xA1 };
		//3.x:
		unsigned char sig3[] = { 
			0x64, 0xA1, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x4C, 
			0x24, 0x04, 0x6A, 0xFF, 0x68, 0xCC, 0xCC, 0xCC, 
			0xCC, 0x50, 0x8B, 0x44, 0x24 };

		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);
		if (offset == 0)
		{
			offset = FindSignatureOffset(sig2, sizeof(sig2), baseAddress, 0x200000);
			if (offset == 0)
			{
				offset = FindSignatureOffset(sig3, sizeof(sig3), baseAddress, 0x200000);
				if (offset == 0)
				{
					return FALSE;
				}
				else
				{
					macroFunction = (unsigned int)offset;
				}
			}
			else
			{
				macroFunction = (unsigned int)offset - 0xD;
			}
		}
		else
		{
			macroFunction = (unsigned int)offset - 0xD;
		}

		return TRUE;
	}

/*********************************************************************************************************************************************
 * This identifies where our main hook in client.exe goes (in the main window message loop) and also stores the original CALL
 * destination so that later on we can restore the code where we placed our hook.
 *********************************************************************************************************************************************/
	BOOL GetHookAddress()
	{
		//3.x - 7.x:
		unsigned char sig1[] = { 0x53, 0x68, 0xE8, 0x03, 0x00, 0x00, 0x52 };

		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);

		if (offset == 0)
			return FALSE;

		hookAddress = (unsigned int)offset + 8; //CALL XXXXXXXX, we're overwriting with our main hook
		originalDest = (unsigned int)offset + 9 + 4 + *(int *)(offset + 9);  //calculate address of the CALL destination so we can do it later

		return TRUE;
	}

/*********************************************************************************************************************************************
 * This is a pointer to the client's Network class object.  It's responsible for sending packets and receiving/decrypting/handling packets.
 *********************************************************************************************************************************************/
	BOOL GetNetworkObjectAddress()
	{
		unsigned char sig1[] = { 
			0xA1, 0xCC, 0xCC, 0xCC, 0xCC, 0x8B, 0x0D, 0xCC, 
			0xCC, 0xCC, 0xCC, 0x8B, 0x16 };

		unsigned char sig2[] = { 
			0xC7, 0x06, 0xCC, 0xCC, 0xCC, 0xCC, 0x89, 0x35, 
			0xCC, 0xCC, 0xCC, 0xCC, 0x8B, 0x4C, 0x24, 0x0C };
		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);
		if (offset == 0)
		{
			offset = FindSignatureOffset(sig2, sizeof(sig2), baseAddress, 0x200000);
			if (offset == 0)
			{
				return FALSE;
			}
			else
			{
				networkObject = (unsigned int *)*(unsigned int *)(offset + 8);
				return TRUE;
			}
		}
		else
		{
			networkObject = (unsigned int *)*(unsigned int *)(offset + 7);
			return TRUE;
		}
		return FALSE;
	}

/*********************************************************************************************************************************************
 * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *
 *********************************************************************************************************************************************/
	BOOL Initialize()
	{
		if (!GetHookAddress())
			return FALSE;
		if (!GetMacroAddress())
			return FALSE;
		if (!GetNetworkObjectAddress())
			return FALSE;
		if (!GetTopGumpAddress())
			return FALSE;
		if (!GetPathfindAddress())
			return FALSE;
		if (!GetRecvAddress())
			return FALSE;
		if (!GetSendAddress())
			return FALSE;

		#ifdef _DEBUG
		LogDebugMessages();
		#endif

		return TRUE;
	}

/*********************************************************************************************************************************************
 * Log information helpful for debugging.  Default filename is "log.txt."  It will be located wherever client.exe is located.
 *********************************************************************************************************************************************/
	#ifdef _DEBUG
	void LogDebugMessages()
	{
		stringstream ss;

		ss << "hookAddress = 0x" << hex << (unsigned int)hookAddress;
		Log(ss.str());
		ss.str("");

		ss << "pathfindFunction = 0x" << hex << pathfindFunction << " , pathfindType = " << pathfindType;
		Log(ss.str());
		ss.str("");

		ss << "recvHookAddress = 0x" << hex << recvHookAddress << " , recvHookType = " << recvHookType;
		Log(ss.str());
		ss.str("");

		ss << "sendHookAddress = 0x" << hex << sendHookAddress << " , sendHookType = " << sendHookType;
		Log(ss.str());
		ss.str("");

		ss << "macroFunction = 0x" << hex << macroFunction;
		Log(ss.str());
		ss.str("");

		ss << "originalDest = 0x" << hex << originalDest;
		Log(ss.str());
		ss.str("");
	
		ss << "topGump = 0x" << hex << (unsigned int)topGump<< " , *topGump = 0x" << hex << *topGump;
		Log(ss.str());
		ss.str("");
	
		ss << "networkObject = 0x" << hex << networkObject << " , *networkObject = 0x" << hex << *networkObject;
		Log(ss.str());
		ss.str("");
	
		ss << "serverPacketSendFunction = 0x" << hex << serverPacketSendFunction;
		Log(ss.str());
		ss.str("");

		ss << "Hooks::SendServerPacket = 0x" << hex << (unsigned int)Hooks::SendServerPacket;
		Log(ss.str());
		ss.str("");

		ss << "Hooks::CallGumpFunction = 0x" << hex << (unsigned int)Hooks::CallGumpFunction;
		Log(ss.str());
		ss.str("");

		ss << "Hooks::SendClientPacket = 0x" << hex << (unsigned int)Hooks::SendClientPacket;
		Log(ss.str());
		ss.str("");
		return;
	}
	#endif
}