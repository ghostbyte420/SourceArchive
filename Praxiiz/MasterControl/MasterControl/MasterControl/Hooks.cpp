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
using namespace Pointers;
using namespace IPC;
using namespace Structs;
using namespace IPCActions;

namespace Hooks
{
	volatile long serverSendLock = 0;
	volatile long clientSendLock = 0;
	volatile long gumpFunctionLock = 0;
	volatile long pathfindLock = 0;
	volatile long macroLock = 0;
	volatile unsigned int clientPacketLen = 0;
	volatile unsigned int gumpFunction = 0;
	volatile unsigned int gumpHandle = 0;
	PathfindPointStruct pStruct;
	PathfindPointStruct *pathfindPoint = &pStruct;
	MacroStruct mStruct;
	MacroStruct *macroStruct = &mStruct;
	unsigned char incomingFilters[256] = { };
	unsigned char outgoingFilters[256] = { };

/*********************************************************************************************************************************************
 * The receive packet hook.
 * [ This executes under the context of client.exe ]
 *********************************************************************************************************************************************/
	__inline void __stdcall RecvHook(unsigned char *buffer, unsigned int len)
	{
		SendRawOutputMessage(OutgoingMessageType::IncomingPacket, buffer, len);
		return;
	}

/*********************************************************************************************************************************************
 * Our handler for incoming filtered packets.
 * [ This executes under the context of client.exe ]
 *********************************************************************************************************************************************/
	__inline void __stdcall RecvFilterHook(unsigned char *buffer, unsigned int len)
	{
		SendRawOutputMessage(OutgoingMessageType::IncomingFilteredPacket, buffer, len);
		return;
	}

/*********************************************************************************************************************************************
 * This sets things up for our packet receive hook above.
 * [ This executes under the context of client.exe ]
 *********************************************************************************************************************************************/
	__inline void __declspec(naked) __stdcall PreRecvHook()
	{

		_asm {
			push ecx
			push edx
			xor edx, edx  // begin packet filter code
			mov eax, [esp + 0xC]
			mov dl, [eax]
			mov al, byte ptr [incomingFilters + edx]
			cmp al, 1
			je filtered   // end packet filter code
			push edi
			push [esp + 0x10]
			call RecvHook
			pop edx
			pop ecx
			cmp recvHookType, 2
			je type2
	posthook1:
			push ebx     // restore stolen code
			push esi     // restore stolen code
			push edi     // restore stolen code
			mov edi, ecx // restore stolen code
			jmp done
	filtered:            // packet is on our blacklist
			push edi
			push [esp + 0x10]
			call RecvFilterHook
			pop edx
			pop ecx
			ret 4
	skipfilter:
			pop edx
			pop ecx
			cmp recvHookType, 1
			je posthook1
	type2:
			mov eax, recvHookType2Data // restore stolen code
			mov eax, dword ptr [eax]
	done:
			jmp dword ptr [recvHookPostAddress]
		}
	}

/*********************************************************************************************************************************************
 * The send packet hook.
 * [ This executes under the context of client.exe ]
 *********************************************************************************************************************************************/
	void __stdcall SendHook(unsigned char *buffer, unsigned int len)
	{
		SendRawOutputMessage(OutgoingMessageType::OutgoingPacket, buffer, len);
		return;
	}

/*********************************************************************************************************************************************
 * Our handler for filtered outgoing packets.
 * [ This executes under the context of client.exe ]
 *********************************************************************************************************************************************/
	void __stdcall SendFilterHook(unsigned char *buffer, unsigned int len)
	{
		SendRawOutputMessage(OutgoingMessageType::OutgoingFilteredPacket, buffer, len);
		//memset(buffer, 0, len);
		return;
	}

/*********************************************************************************************************************************************
 * This sets things up for our packet send hook above.
 * [ This executes under the context of client.exe ]
 *********************************************************************************************************************************************/
	__inline void __declspec(naked) __stdcall PreSendHook()
	{
		_asm {
			push eax
			push ecx
			push edx
			cmp sendHookType, 2
			je type2
			xor eax, eax           // begin packet filter code
			mov al, byte ptr [esi]
			mov dl, byte ptr [outgoingFilters + eax]
			cmp dl, 1
			je filter1             // end packet filter code
			push ebp               // arg2, packet len
			push esi               // arg1, packet buffer
			call SendHook
	posthook1:
			pop edx
			pop ecx
			pop eax
			push ebp               // restore stolen code
			lea ecx, [ebx + 0xBC]  // restore stolen code
			jmp done
	filter1:
			mov eax, dword ptr [esp + 0x24]
			cmp eax, 0xDEADFED5    // did we send this?
			je posthook1           // normally we would drop and log this packet, but it's ours so we do neither
			push ebp               // arg2, packet len
			push esi               // arg1, packet buffer
			call SendFilterHook
			xor ebp, ebp           // set packet length to 0, this is how we drop the packet
			jmp posthook1
	type2:
			xor eax, eax           // begin packet filter code
			mov al, byte ptr [esi]
			mov dl, byte ptr [outgoingFilters + eax]
			cmp dl, 1
			je filter2             // end packet filter code
			push ebx               // arg2, packet len
			push esi               // arg1, packet buffer
			call SendHook
	posthook2:
			pop edx
			pop ecx
			pop eax
			push ebx               // restore stolen code
			push eax               // restore stolen code
			lea ecx, [edi + 0x6C]  // restore stolen code
			jmp done
	filter2:
			mov eax, dword ptr [esp + 0x20]
			cmp eax, 0xDEADFED5    // did we send this?
			je posthook2           // normally we would drop and log this packet, but it's ours so we do neither
			push ebx			   // arg2, packet len
			push esi			   // arg1, packet buffer
			call SendFilterHook
			xor ebx, ebx           // set packet length to 0, this is how we drop the packet
			jmp posthook2
	done:
			jmp dword ptr [sendHookPostAddress]
		}
	}

	void InstallMainHook()
	{
		unsigned char hook[5] = {0};
		CreateCALL((void*)hookAddress, &ExecuteMacro, CallType::CALL, hook);
		memcpy((void*)hookAddress, hook, 5);
		return;
	}

/*********************************************************************************************************************************************
 * This is our main hook code.  It starts the first in the chain of calls responsible for implementing most of our functionality.
 * This code fragment is meant to be JMPed into.
 * [ This executes under the context of client.exe ]
 *********************************************************************************************************************************************/
	__inline void __declspec(naked) __stdcall ExecuteMacro()
	{
		_asm {
			call SendServerPacket
			mov eax, 2 // comparand, 2 means data to be read, 1 = in use, 0 = free for write access
			mov ecx, 1
			lock cmpxchg dword ptr [macroLock], ecx
			jnz nodata // no macro requests are pending, go to next in line
			push 0
			push dword ptr [macroStruct]
			push macroReturn
			jmp macroFunction
	macroReturn:
			add esp, 8
			xor ecx, ecx
			lock xchg dword ptr [macroLock], ecx // set to 0, now it's free for write access
	nodata:
			jmp originalDest
		}
	}

/*********************************************************************************************************************************************
 * Call client's pathfind function with our specified X,Y,Z coords.  The client will only pathfind up to a certain distance.
 * [ This executes under the context of client.exe ]
 *********************************************************************************************************************************************/
	__inline void __stdcall CallPathfindFunction()
	{
		_asm {
			mov eax, 2 // 2 means data to be read, 1 = in use, 0 = free for write access
			mov ecx, 1
			lock cmpxchg dword ptr [pathfindLock], ecx
			jnz nodata // no pathfind requests are pending, go to next in line
			mov eax, dword ptr [pathfindPoint]
			sub eax, 0x24
			sub esp, 8
			push pathreturn  // push return address onto stack
			mov ecx, pathfindType
			cmp ecx, 2
			je type2
			sub esp, 8  // we're jumping into the middle of a function so we have to fix up the stack
			push ebp
			sub esp, 8
			jmp pathfindFunction
	type2:
			sub esp, 4
			jmp pathfindFunction
	pathreturn:
			xor ecx, ecx
			lock xchg dword ptr [pathfindLock], ecx // set to 0, now it's free for write access
	nodata:
			}
	}

/*********************************************************************************************************************************************
 * Call vTable function from specified gump. 0 should always be gump destructor.  Invalid values -will- crash the client.
 * [ This executes under the context of client.exe ]
 *********************************************************************************************************************************************/
	__inline void __stdcall CallGumpFunction()
	{
		_asm {
			mov eax, 2 // 2 means data to be read, 1 = in use, 0 = free for write access
			mov ecx, 1
			lock cmpxchg dword ptr [gumpFunctionLock], ecx
			jnz nodata
			mov ecx, gumpHandle
			cmp ecx, 0
			je done // one way ticket to crashville, let's avoid
			mov eax, gumpFunction
			mov edx, dword ptr [ecx]
			call dword ptr [edx + eax * 4]
	done:
			xor ecx, ecx
			lock xchg dword ptr [gumpFunctionLock], ecx // set to 0, now it's free for write access
	nodata:
			call CallPathfindFunction
			}
	}

/*********************************************************************************************************************************************
 * We hijack the client.exe packet receiving function and tell it to use our own packet buffer.
 * Invalid packets -can- crash the client.  Trying to send packets before the client is connected can also crash the client.
 * [ This executes under the context of client.exe ]
 *********************************************************************************************************************************************/
	__inline void __stdcall SendClientPacket()
	{
		_asm {
			mov eax, 2 // 2 means data to be read, 1 = in use, 0 = free for write access
			mov ecx, 1
			lock cmpxchg dword ptr [clientSendLock], ecx
			jnz nodata
			mov ecx, dword ptr [networkObject]
			mov ecx, dword ptr [ecx]
			cmp ecx, 0
			je done  // <DON'T RELY ON THIS> we're not connected yet, this packet is lost forever.  i shed a tear for you dear packet =(
			cmp recvHookType, 2
			je type2
			mov eax, clientSendBuffer
			push eax
			mov edi, clientPacketLen
			mov esi, ecx
			push done    // return address
			push ebx     // restore stolen code
			push esi     // restore stolen code
			push edi     // restore stolen code
			mov edi, ecx // restore stolen code
			jmp recvHookPostAddress
	type2:
			mov eax, clientSendBuffer
			push eax
			mov edi, clientPacketLen
			mov esi, ecx
			mov eax, recvHookType2Data // restore stolen code
			mov eax, dword ptr [eax]
	dosend:
			call dword ptr [recvHookPostAddress]
	done:
			xor ecx, ecx
			lock xchg dword ptr [clientSendLock], ecx //set to 0, now it's free for write access
	nodata:
			call CallGumpFunction
			}
	}

/*********************************************************************************************************************************************
 * We hijack the client.exe packet sending function and tell it to use our own packet buffer.  As far as I can tell it will only process one
 * packet from the buffer.  Note that we don't pass a buffer len.  This is because all outgoing packets already have their length set 
 * in the packet header.  The client just needs a buffer and is ready to go.  Invalid packets -can- crash the client.
 * [ This executes under the context of client.exe ]
 *********************************************************************************************************************************************/
	__inline void __stdcall SendServerPacket()
	{
		__asm
		{
			mov eax, 2 // 2 means data to be read, 1 = in use, 0 = free for write access
			mov ecx, 1
			lock cmpxchg dword ptr [serverSendLock], ecx
			jnz nodata
			mov ecx, dword ptr [networkObject]
			mov ecx, dword ptr [ecx]
			cmp ecx, 0
			je done  // <DON'T RELY ON THIS> we're not connected yet, this packet is lost forever.  i shed a tear for you dear packet =(
			mov eax, serverSendBuffer
			push 0xDEADFED5  // this tells our hook later on that it's our own packet and shouldn't be dropped even if it's on the blacklist
			push eax
			call dword ptr [serverPacketSendFunction]
			pop ecx
	done:
			xor ecx, ecx
			lock xchg dword ptr [serverSendLock], ecx //set to 0, now it's free for write access
	nodata:
			call SendClientPacket
		}
	}

/*********************************************************************************************************************************************
 * Injects a small bit of code into our target process.  The purpose is to call our SendHook function above.
 *********************************************************************************************************************************************/
	void InstallSendHook()
	{
		if (sendHookType == 1)
		{
			unsigned char call[7];
			memset(call, 0x90, 7); //fill with NOPs
			CreateCALL((void*)sendHookAddress, &PreSendHook, CallType::JMP, call);
			memcpy((void*)sendHookAddress, call, 7);
		}
		else if (sendHookType == 2)
		{
			unsigned char call[5] = { };
			CreateCALL((void*)sendHookAddress, &PreSendHook, CallType::JMP, call);
			memcpy((void*)sendHookAddress, call, 5);
		}
		return;
	}

/*********************************************************************************************************************************************
 * Injects a small bit of code into our target process.  The purpose is to call our RecvHook function above.
 *********************************************************************************************************************************************/
	void InstallRecvHook()
	{
		unsigned char call[5] = { };
		CreateCALL((void*)recvHookAddress, &PreRecvHook, CallType::JMP, call);
		memcpy((void*)recvHookAddress, call, 5);
		return;
	}

/*********************************************************************************************************************************************
 * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *** * *
 *********************************************************************************************************************************************/

	BOOL Initialize()
	{
		mStruct.field1 = 0x00000005;
		mStruct.field2 = 0x00190000;
		mStruct.field3 = 0x00000000;
		mStruct.field4 = 0x00000001;
		mStruct.field5 = 0x00000000;
		mStruct.field6 = 0x00000001;
		mStruct.textPointer = &mStruct.text[0];
		mStruct.arg1 = 0;
		mStruct.arg2 = 0;
		memset(&mStruct.text[0], 0, 256);
		InstallRecvHook();
		InstallSendHook();
		InstallMainHook();
		return TRUE;
	}

}