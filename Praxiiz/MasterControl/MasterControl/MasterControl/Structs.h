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


namespace Structs
{
	extern enum OutgoingMessageType : unsigned char {
		OutgoingMessage = 1,
		IncomingPacket, 
		OutgoingPacket,
		IncomingFilteredPacket,
		OutgoingFilteredPacket
	};

	extern enum IncomingMessageType : unsigned char {
		IncomingMessage = 1,
		SendClientPacket,
		SendServerPacket,
		CallGumpFunction,
		Pathfind,
		Type1Macro, // 2 parameter
		Type2Macro, // 3 parameter
		AddIncomingPacketFilter,
		RemoveIncomingPacketFilter,
		AddOutgoingPacketFilter,
		RemoveOutgoingPacketFilter,
		PatchEncryption,
		SetLoginServer
	};

	struct IpcInfoStruct {
		unsigned int inFileMap;
		unsigned int outFileMap;
		unsigned int inSemaphore;
		unsigned int outSemaphore;
	};

	struct ProcessThreadStruct{
		unsigned char* rawMessage;
		unsigned int len;
		unsigned char messageType;
	};

	struct PathfindPointStruct {
		volatile unsigned short x;
		volatile unsigned short y;
		volatile unsigned short z;
	};

	struct MacroStruct {
		unsigned int field1;
		unsigned int field2;
		unsigned int field3;
		unsigned int field4;
		unsigned int field5;
		unsigned int field6;
		volatile unsigned int arg1;
		volatile unsigned short arg2;
		unsigned char *textPointer;
		unsigned char text[256]; //unicode
	};

   /*********************************************************************************************************************************************
	* These structs are meant for the raw bytes in IPC messages -MINUS- the IPC header (i.e. minus the first 9 bytes)
	*********************************************************************************************************************************************/

	struct Type1MacroStruct {
		unsigned int arg1;
		unsigned short arg2;
	};

	struct Type2MacroStruct {
		unsigned int arg1;
		unsigned short arg2;
		unsigned int textLen;
	};

	struct CallGumpFunctionStruct {
		unsigned int gumpHandle;    // Handle of gump to interact with. 0 = top gump
		unsigned int functionIndex; // vTable function index to call.  Incorrect values -will- cause nasty client crashes.
	};

	struct PathfindStruct {
		unsigned short x;
		unsigned short y;
		unsigned short z;
	};

	struct LoginServerStruct {
		unsigned int server;
		unsigned short port;
	};
	
}