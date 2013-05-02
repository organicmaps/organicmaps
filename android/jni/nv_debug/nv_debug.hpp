//----------------------------------------------------------------------------------
// File:            libs\jni\nv_debug.h
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
#ifndef __INCLUDED_NV_DEBUG_H
#define __INCLUDED_NV_DEBUG_H

#define CT_ASSERT(tag,cond) \
enum { COMPILE_TIME_ASSERT__ ## tag = 1/(cond) }

#define dimof( x ) ( sizeof(x) / sizeof(x[0]) )
#include <android/log.h>

#define DBG_DETAILED 0

#if 0

	// the detailed prefix can be customised by setting DBG_DETAILED_PREFIX. See
	// below as a reference.
	// NOTE: fmt is the desired format string and must be in the prefix.
	//#ifndef DBG_DETAILED_PREFIX
	//	#define DBG_DETAILED_PREFIX "%s, %s, line %d: " fmt, __FILE__, __FUNCTION__, __LINE__,
	//#endif 
	//#define DEBUG_D_(fmt, args...) 
	//#define DEBUG_D(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, MODULE, (DBG_DETAILED_PREFIX) ## args)

#else

	#ifdef STRINGIFY
		#pragma push_macro("STRINGIFY")
		#undef STRINGIFY
		#define STRINGIFYPUSHED_____
	#endif
	#define STRINGIFY(x) #x

	// debug macro, includes file name function name and line number
	#define TO(x) typeof(x)
	#define DEBUG_D_(file, line, fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, MODULE, file ", %s, line(" STRINGIFY(line) "): " fmt, __FUNCTION__, ## args)
	#define DEBUG_D(fmt, args...) DEBUG_D_( __FILE__ , __LINE__ , fmt, ## args)

	#ifdef STRINGIFYPUSHED_____
		#undef STRINGIFYPUSHED_____
		#pragma pop_macro("STRINGIFY")
	#endif

#endif

// basic debug macro
#define NVDEBUG_(fmt, args...) (__android_log_print(ANDROID_LOG_DEBUG, MODULE, fmt, ## args))

// Debug macro that can be switched to spew a file name,
// function and line number using DEBUG_DETAILED
#if DBG_DETAILED == 1
	#define NVDEBUG(fmt, args...) NVDEBUG_D(fmt, ## args)
#else
	#define NVDEBUG(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, MODULE, fmt, ## args)
#endif


#endif
