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
#include "AtomicLock.h"

// Scoped lock, used to help prevent potential deadlocks
AtomicLock::AtomicLock(volatile long *lock, long comparand)
{
	_lock = lock;
	while (comparand != _InterlockedCompareExchange(lock, 1, comparand))
				Sleep(1);
	return;
}


AtomicLock::~AtomicLock(void)
{
	_InterlockedExchange(_lock, 0);
	return;
}
