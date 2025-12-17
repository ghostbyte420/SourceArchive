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
using namespace Utils;
using namespace Pointers;

namespace Patches
{

	BOOL ProtocolDecryption()
	{
		//3.x - 5.x
        unsigned char sig1[] = { 0x83, 0xFF, 0xFF, 0x0F, 0x84, 0xCC, 0xCC, 0xCC, 0xCC, 0x8B, 0x86, 0xCC, 0xCC, 0xCC, 0xCC, 0x85, 0xC0 };
		//6.x - 7.x
        unsigned char sig2[] = { 0x74, 0x37, 0x83, 0xBE, 0xB4, 0x00, 0x00, 0x00, 0x00 };
		//3.x (props to Lord Binary)
        unsigned char sig3[] = { 0x33, 0xFF, 0x3B, 0xFD, 0x0F, 0x85 };
		//1.x - 4.x (props to Lord Binary)
        unsigned char sig4[] = { 0x8B, 0xF8, 0x83, 0xFF, 0xFF, 0x74, 0xCC, 0x8B, 0x86 };

		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);
		if (offset == 0)
		{
			offset = FindSignatureOffset(sig2, sizeof(sig2), baseAddress, 0x200000);
			if (offset == 0)
			{
				offset = FindSignatureOffset(sig3, sizeof(sig3), baseAddress, 0x200000);
				if (offset == 0)
				{
					offset = FindSignatureOffset(sig4, sizeof(sig4), baseAddress, 0x200000);
					if (offset == 0)
					{
						return FALSE;
					}
					else
					{
						*(offset + 0xD) = 0x3B;
					}
				}
				else
				{
					*(offset + 0x1A) = 0x3B;
				}
			}
			else
			{
				*offset = 0xEB;
			}
		}
		else
		{
			*(offset + 0xF) = 0x3B;
		}
		return TRUE;
	}

	BOOL TwoFishEncryption()
	{
		// NOTE:  A couple of these patches overlap on many clients.
		// This doesn't seem to impact the client negatively, but it is different from Lord Binary's method.
		// I've tested many encrypted clients and they all work fine.
		
		//3.x - 5.x:
		unsigned char sig1[] = { 0x8B, 0xD9, 0x8B, 0xC8, 0x48, 0x85, 0xC9, 0x0F, 0x84 };
		//6.x - 7.x:
		unsigned char sig2[] = { 0x74, 0x0F, 0x83, 0xB9, 0xB4, 0x00, 0x00, 0x00, 0x00 };
		//3.x - 4.x:
		unsigned char sig3[] = { 0x81, 0xEC, 0x04, 0x01, 0x00, 0x00, 0x85, 0xC0, 0x53, 0x8B, 0xD9, 0x0F, 0x84 };
		//1.x - 3.x (props to Lord Binary)
		unsigned char sig4[] = { 0x5E, 0xC3, 0x8B, 0x86, 0xCC, 0xCC, 0xCC, 0xCC, 0x85, 0xC0 , 0x74 };

		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);
		BOOL success = false;

		if (offset == 0)
		{
			offset = FindSignatureOffset(sig2, sizeof(sig2), baseAddress, 0x200000);
			if (offset == 0)
			{
				offset = FindSignatureOffset(sig3, sizeof(sig3), baseAddress, 0x200000);
				if (offset != 0)
				{
					*(offset + 0xC) = 0x85;
					success = TRUE;
				}
			}
			else
			{
				*offset = 0xEB;
				success = TRUE;
			}
		}
		else
		{
			*(offset + 8) = 0x85;
			success = TRUE;
		}

		offset = FindSignatureOffset(sig4, sizeof(sig4), baseAddress, 0x200000);
		if (offset != 0)
		{
			*(offset + 8) = 0x3B;
			*(offset + 0x12) = 0x3B;
			success = TRUE;
		}
		return success;
	}

	BOOL LoginEncryption()
	{
		//1.x - 5.x:
		unsigned char sig1[] = { 0x81, 0xF9, 0x00, 0x00, 0x01, 0x00, 0x0F, 0x8F };
		//6.x - 7.x:
		unsigned char sig2[] = { 0x75, 0x12, 0x8B, 0x54, 0x24, 0x0C };

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
				*offset = 0xEB;
			}
		}
		else
		{
			*(offset + 0x15) = 0x84;
		}
		return TRUE;
	}

/*********************************************************************************************************************************************
 * Patches login server and port with our own value.  Also patches code to prevent client from changing our values.
 * It doesn't matter what is in login.cfg, it just works.  I don't know how Razor does it, but it's very picky for me and
 * requires a virgin login.cfg...this one doesn't.
 *********************************************************************************************************************************************/
	BOOL LoginServer(unsigned int server, unsigned short port)
	{
		//4.x - 5.x
        unsigned char sig1[] = { 0x8B, 0x44, 0x24, 0x0C, 0xC1, 0xE1, 0x08, 0x0B, 0xC8, 0x66, 0x89, 0x15 };
		//6.x - 7.x
        unsigned char sig2[] = { 0xC1, 0xE1, 0x08, 0x0B, 0x4C, 0x24, 0x0C, 0x89, 0x0D };
		//1.x - 4.x
        unsigned char sig3[] = { 0xC1, 0xE0, 0x08, 0x83, 0xC4, 0x18, 0x0B, 0xC1 };

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
					*(unsigned int *)*(unsigned int *)(offset + 0xE) = server;
					*(unsigned short *)*(unsigned int *)(offset + 0x15) = port;
					memset(offset + 0x12, 0x90, 7);
					memset(offset + 0xD, 0x90, 5);
				}
			}
			else
			{
				*(unsigned int *)*(unsigned int *)(offset + 9) = server;
				*(unsigned short *)*(unsigned int *)(offset - 4) = port;
				memset(offset - 7, 0x90, 7);
				memset(offset + 7, 0x90, 6);
			}
		}
		else
		{
			*(unsigned int *)*(unsigned int *)(offset + 0x12) = server;
			*(unsigned short *)*(unsigned int *)(offset + 0xC) = port;
			memset(offset + 9, 0x90, 7);
			memset(offset + 0x10, 0x90, 6);
		}
		return TRUE;
	}

/*********************************************************************************************************************************************
 * This is part of the MultiUO patch.  Patches the following check: "Another copy of UO is already running!"
 *********************************************************************************************************************************************/
	void SingleCheck()
	{
		//1.x - 7.x
        unsigned char sig1[] = { 0x85, 0xC0, 0x74, 0x15, 0x6A, 0x40 };

		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);
		if (offset != 0)
			*(offset + 2) = 0xEB;
		return;
	}

/*********************************************************************************************************************************************
 * This is part of the MultiUO patch.  Patches the following checks:
 * "Another instance of UO is already running."
 * "An instance of UO Patch is already running."
 *********************************************************************************************************************************************/
	void DoubleCheck()
	{
		//1.x - 3.x
        unsigned char sig1[] = { 0x3D, 0xB7, 0x00, 0x00, 0x00, 0x75, 0x1B, 0x8B, 0x0D };

		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);
		if (offset != 0)
		{
			*(offset + 0x37) = 0xEB;
			*(offset - 8) = 0xEB;
		}
		return;
	}


/*********************************************************************************************************************************************
 * This is part of the MultiUO patch.  Patches the following checks:
 * "Another instance of UO may already be running."
 * "Another instance of UO is already running."
 * "An instance of UO Patch is already running."
 *********************************************************************************************************************************************/
	BOOL TripleCheck()
	{
		//4.x - 5.x
        unsigned char sig1[] = { 0x3B, 0xC3, 0x89, 0x44, 0x24, 0x10, 0x75, 0x12 };
        //6.x - 7.x
        unsigned char sig2[] = { 0x3B, 0xC3, 0x89, 0x44, 0x24, 0x08 };

		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);
		if (offset != 0)
		{
			*(offset + 6) = 0xEB;
			*(offset + 0x25) = 0xEB;
			*(offset + 0x4B) = 0xEB;
		}
		else
		{
			offset = FindSignatureOffset(sig2, sizeof(sig2), baseAddress, 0x200000);
			if (offset != 0)
			{
				*(offset + 6) = 0xEB;
				*(offset + 0x2D) = 0xEB;
				*(offset + 0x5F) = 0xEB;
			}
			else
			{
				return FALSE;
			}
		}
		return TRUE;
	}

/*********************************************************************************************************************************************
 * This is part of the MultiUO patch.  Patches a check on some variable that holds error information.
 * Not all clients require this patch.  These signatures -should- match on all the clients that need it.
 *********************************************************************************************************************************************/
	void ErrorCheck()
	{
		//6.x - 7.x
        unsigned char sig1[] = { 0x85, 0xC0, 0x5F, 0x5E, 0x75, 0x2F };
		//4.x - 5.x
		unsigned char sig2[] = { 0x85, 0xC0, 0x75, 0x2F, 0xBF };

		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);
		if (offset != 0)
		{
			*(offset + 4) = 0x90;
			*(offset + 5) = 0x90;
		}
		else
		{
			offset = FindSignatureOffset(sig2, sizeof(sig2), baseAddress, 0x200000);
			if (offset != 0)
			{
				//XOR AX, AX
				*(offset) = 0x66;
				*(offset + 1) = 0x33;
				*(offset + 2) = 0xC0;
				*(offset + 3) = 0x90;
			}
		}
		return;
	}

/*********************************************************************************************************************************************
 * MultiUO patch for client versions: 7.0.5.0 - 7.0.9.1, 7.0.10.0 - 7.0.19.1, 7.0.20.0 - 7.0.24.2 (and probably some future versions)
 * This is the only patch necessary for those clients.  I didn't remove the check for the UO patcher in this one.
 *********************************************************************************************************************************************/
	BOOL SevenSingleCheck()
	{
		unsigned char sig1[] = { 0x83, 0xC4, 0x04, 0x33, 0xED, 0x55, 0x50, 0xFF, 0x15 };
		unsigned char *offset = FindSignatureOffset(sig1, sizeof(sig1), baseAddress, 0x200000);
		if (offset != 0)
		{
			*(offset + 0xD) = 0x39;
			return TRUE;
		}
		return FALSE;
	}

/*********************************************************************************************************************************************
 * Patches client.exe to remove checks for other running instances.
 *********************************************************************************************************************************************/
	void MultiUO()
	{
		if (SevenSingleCheck())
		{
			return;
		}
		else
		{
			ErrorCheck();
			SingleCheck();
			if (!TripleCheck())
				DoubleCheck();
		}
		return;
	}

/*********************************************************************************************************************************************
 * Only do REQUIRED patches in this section.  All others should be invoked on demand via IPC command.
 *********************************************************************************************************************************************/
	BOOL Initialize()
	{
		MultiUO();
		return TRUE;
	}
}