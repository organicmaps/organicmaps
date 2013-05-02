//----------------------------------------------------------------------------------
// File:            libs\jni\nv_time\nv_time.cpp
// Samples Version: NVIDIA Android Lifecycle samples 1_0beta 
// Email:           tegradev@nvidia.com
// Web:             http://developer.nvidia.com/category/zone/mobile-development
//
// Copyright 2009-2011 NVIDIA® Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//----------------------------------------------------------------------------------

#include "nv_time.hpp"
#include "../nv_thread/nv_thread.hpp"
/*#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>*/
#include <time.h>
#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <unistd.h>

/*#ifndef EGL_NV_system_time
#define EGL_NV_system_time 1
typedef khronos_uint64_t EGLuint64NV;
#ifdef EGL_EGLEXT_PROTOTYPES
EGLAPI EGLuint64NV EGLAPIENTRY eglGetSystemTimeFrequencyNV(void);
EGLAPI EGLuint64NV EGLAPIENTRY eglGetSystemTimeNV(void);
#endif
typedef EGLuint64NV (EGLAPIENTRYP PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC)(void);
typedef EGLuint64NV (EGLAPIENTRYP PFNEGLGETSYSTEMTIMENVPROC)(void);
#endif
*/

void nvAcquireTimeExtensionJNI(JNIEnv*, jobject)
{
	nvAcquireTimeExtension();
}

jlong nvGetSystemTimeJNI(JNIEnv*, jobject)
{
	return (jlong)nvGetSystemTime();
}

/*static PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC eglGetSystemTimeFrequencyNVProc = NULL;
static PFNEGLGETSYSTEMTIMENVPROC eglGetSystemTimeNVProc = NULL;
static EGLuint64NV eglSystemTimeFrequency = 0;
static bool timeExtensionQueried = false;*/

void nvAcquireTimeExtension()
{
/*    if (timeExtensionQueried)
        return;
    timeExtensionQueried = true;

	eglGetSystemTimeFrequencyNVProc = (PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC) eglGetProcAddress("eglGetSystemTimeFrequencyNV");
	eglGetSystemTimeNVProc = (PFNEGLGETSYSTEMTIMENVPROC) eglGetProcAddress("eglGetSystemTimeNV");

	// now, we'll proceed through a series of sanity checking.
	// if they all succeed, we'll return.
	// if any fail, we fall out of conditional tests to end of function, null pointers, and return.
	if (eglGetSystemTimeFrequencyNVProc &&
		eglGetSystemTimeNVProc)
	{
		eglSystemTimeFrequency = eglGetSystemTimeFrequencyNVProc();
		if (eglSystemTimeFrequency>0) // assume okay.  quick-check it works.
		{
			EGLuint64NV time1, time2;
			time1 = eglGetSystemTimeNVProc();
			usleep(2000); // 2ms should be MORE than sufficient, right?
			time2 = eglGetSystemTimeNVProc();
			if (time1 != time2) // quick sanity only...
			{
				// we've sanity checked:
				// - fn pointers non-null
				// - freq non-zero
				// - two calls to time sep'd by sleep non-equal
				// safe to return now.
				return;
			}
		}
	}

	// fall back if we've not returned already.
	eglGetSystemTimeFrequencyNVProc = (PFNEGLGETSYSTEMTIMEFREQUENCYNVPROC) NULL;
	eglGetSystemTimeNVProc = (PFNEGLGETSYSTEMTIMENVPROC) NULL;*/
}

bool nvValidTimeExtension()
{
/*	if (NULL == eglGetSystemTimeFrequencyNVProc ||
		NULL == eglGetSystemTimeNVProc)
		return false;
	else
		return true;*/
}

long nvGetSystemTime()
{
	static struct timeval start_time, end_time;
	static int isinit = 0;
	jlong curr_time = 0;

/*	if(eglGetSystemTimeNVProc)
	{
		EGLuint64NV egltime;
		EGLuint64NV egltimequot;
		EGLuint64NV egltimerem;

		egltime = eglGetSystemTimeNVProc();

		egltimequot = egltime / eglSystemTimeFrequency;
		egltimerem = egltime - (eglSystemTimeFrequency * egltimequot);
		egltimequot *= 1000;
		egltimerem *= 1000;
		egltimerem /= eglSystemTimeFrequency;
		egltimequot += egltimerem;
		return (jlong) egltimequot;
	}
*/
	if (!isinit)
	{
		gettimeofday(&start_time, 0);
		isinit = 1;
	}
	gettimeofday(&end_time, 0);
	curr_time = (end_time.tv_sec - start_time.tv_sec) * 1000;
	curr_time += (end_time.tv_usec - start_time.tv_usec) / 1000;

	return curr_time;
}

void NVTimeInit()
{
    JNIEnv* env = NVThreadGetCurrentJNIEnv();

	JNINativeMethod methods_time[] =
    {
        {
            "nvAcquireTimeExtension",
            "()V",
            (void *) nvAcquireTimeExtension
        },
        {
            "nvGetSystemTime",
            "()J",
            (void *) nvGetSystemTime
        },
    };
    jclass k_time;
    k_time = (env)->FindClass ("com/nvidia/devtech/NvActivity");
    (env)->RegisterNatives(k_time, methods_time, 2);
}
