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

namespace IPCActions
{
	void SendPacketToClient(unsigned char *source, unsigned int len);
	void SendPacketToServer(unsigned char *source, unsigned int len);
	void ExecuteGumpFunction(unsigned int gump, unsigned int vTableOffset);
	void ExecutePathfindFunction(PathfindStruct p);
	void SendRawOutputMessage(OutgoingMessageType messageType, unsigned char *buffer, unsigned int len);
	void ExecuteType1Macro(unsigned int arg1, unsigned short arg2);
	void ExecuteType2Macro(unsigned int arg1, unsigned short arg2, unsigned char* textBuffer, unsigned int textLen);
	void AddIncomingPacketType(unsigned char packet);
	void AddOutgoingPacketType(unsigned char packet);
	void RemoveIncomingPacketType(unsigned char packet);
	void RemoveOutgoingPacketType(unsigned char packet);
	void PatchClientEncryption();
	void PatchLoginServer(unsigned int server, unsigned short port);
}