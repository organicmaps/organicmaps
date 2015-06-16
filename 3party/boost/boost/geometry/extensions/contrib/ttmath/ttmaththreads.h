/*
 * This file is a part of TTMath Bignum Library
 * and is distributed under the (new) BSD licence.
 * Author: Tomasz Sowa <t.sowa@ttmath.org>
 */

/* 
 * Copyright (c) 2006-2009, Tomasz Sowa
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *    
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *    
 *  * Neither the name Tomasz Sowa nor the names of contributors to this
 *    project may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */



#ifndef headerfilettmaththreads
#define headerfilettmaththreads

#include "ttmathtypes.h"

#ifdef TTMATH_WIN32_THREADS
#include <windows.h>
#include <cstdio>
#endif

#ifdef TTMATH_POSIX_THREADS
#include <pthread.h>
#endif



/*!
	\file ttmaththreads.h
    \brief Some objects used in multithreads environment
*/


/*
	this is a simple skeleton of a program in multithreads environment:

	#define TTMATH_MULTITHREADS
	#include<ttmath/ttmath.h>
	
	TTMATH_MULTITHREADS_HELPER

	int main()
	{
	[...]
	}

	make sure that macro TTMATH_MULTITHREADS is defined and (somewhere in *.cpp file)
	use TTMATH_MULTITHREADS_HELPER macro (outside of any classes/functions/namespaces scope)
*/


namespace ttmath
{


#ifdef TTMATH_WIN32_THREADS

	/*
		we use win32 threads
	*/


	/*!
		in multithreads environment you should use TTMATH_MULTITHREADS_HELPER macro
		somewhere in *.cpp file

		(at the moment in win32 this macro does nothing)
	*/
	#define TTMATH_MULTITHREADS_HELPER


	/*!
		objects of this class are used to synchronize
	*/
	class ThreadLock
	{
		HANDLE mutex_handle;


		void CreateName(char * buffer) const
		{
			#ifdef _MSC_VER
			#pragma warning (disable : 4996)
			// warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.
			#endif

			sprintf(buffer, "TTMATH_LOCK_%ul", (unsigned long)GetCurrentProcessId());

			#ifdef _MSC_VER
			#pragma warning (default : 4996)
			#endif
		}


	public:

		bool Lock()
		{
		char buffer[50];

			CreateName(buffer);
			mutex_handle = CreateMutexA(0, false, buffer);

			if( mutex_handle == 0 )
				return false;

			WaitForSingleObject(mutex_handle, INFINITE);

		return true;
		}


		ThreadLock()
		{
			mutex_handle = 0;
		}


		~ThreadLock()
		{
			if( mutex_handle != 0 )
			{
				ReleaseMutex(mutex_handle);
				CloseHandle(mutex_handle);
			}
		}
	};

#endif  // #ifdef TTMATH_WIN32_THREADS





#ifdef TTMATH_POSIX_THREADS

	/*
		we use posix threads
	*/


	/*!
		in multithreads environment you should use TTMATH_MULTITHREADS_HELPER macro
		somewhere in *.cpp file
		(this macro defines a pthread_mutex_t object used by TTMath library)
	*/
	#define TTMATH_MULTITHREADS_HELPER                          \
	namespace ttmath                                            \
	{                                                           \
	pthread_mutex_t ttmath_mutex = PTHREAD_MUTEX_INITIALIZER;   \
	}


	/*!
		ttmath_mutex will be defined by TTMATH_MULTITHREADS_HELPER macro 
	*/
	extern pthread_mutex_t ttmath_mutex;


	/*!
		objects of this class are used to synchronize
	*/
	class ThreadLock
	{
	public:

		bool Lock()
		{
			if( pthread_mutex_lock(&ttmath_mutex) != 0 )
				return false;

		return true;
		}


		~ThreadLock()
		{
			pthread_mutex_unlock(&ttmath_mutex);
		}
	};

#endif // #ifdef TTMATH_POSIX_THREADS




#if !defined(TTMATH_POSIX_THREADS) && !defined(TTMATH_WIN32_THREADS)

	/*!
		we don't use win32 and pthreads
	*/

	/*!
	*/
	#define TTMATH_MULTITHREADS_HELPER


	/*!
		objects of this class are used to synchronize
		actually we don't synchronize, the method Lock() returns always 'false'
	*/
	class ThreadLock
	{
	public:

		bool Lock()
		{
			return false;
		}
	};


#endif // #if !defined(TTMATH_POSIX_THREADS) && !defined(TTMATH_WIN32_THREADS)





} // namespace

#endif

