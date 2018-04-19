/*
 * This file was created on Wed Mar 28 2018
 * Copyright 2018 Romain CADILHAC
 *
 * This file is a part of HaoudOS.
 *
 * HaoudOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HaoudOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with HaoudOS. If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <driver/bochs/bochs.h>

#define AssertFatal(condition)						\
	if(!(condition))								\
	{												\
		BochsPrintf("\nAssertion %s failed (Function %s() at line %u)\nKernel stoped!\n", #condition, __FUNCTION__, __LINE__);	\
		asm("jmp $");									\
	}

#define AssertError(condition)						\
	if(!(condition))								\
	{												\
		BochsPrintf("\nError: Assertion %s failed (Function %s() at line %u)\n", #condition, __FUNCTION__, __LINE__);	\
	}

#define AssertReturn(condition, value)				\
	if(!(condition))								\
	{												\
		BochsPrintf("\nError: Assertion %s failed (Function %s() at line %u)\n", #condition, __FUNCTION__, __LINE__);	\
		return #value;																									\
	}

#define AssertWarning(condition)					\
	if(!(condition))								\
	{												\
		BochsPrintf("\nWarning: Assertion %s failed (Function %s() at line %u)\n", #condition, __FUNCTION__, __LINE__);	\
	}
