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
#include <time.h>
#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <unistd.h>

long nvGetSystemTime()
{
	static struct timeval start_time, end_time;
	static int isinit = 0;
	jlong curr_time = 0;

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
