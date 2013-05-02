//----------------------------------------------------------------------------------
// File:            libs\jni\nv_thread\nv_thread.h
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

#ifndef __INCLUDED_NV_THREAD_H
#define __INCLUDED_NV_THREAD_H

#include <jni.h>
#include <pthread.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C"
{
#endif

/** @file nv_thread.h
  The NVThread library makes it easy to create native threads that can acess
  JNI objects.  By default, pthreads created in the Android NDK are NOT connected
  to the JVM and JNI calls will fail.  This library wraps thread creation in
  such a way that pthreads created using it will connect to and disconnect from
  the JVM as appropriate.  Applications creating all of their threads with these
  interfaces can use the provided NVThreadGetCurrentJNIEnv() function to
  get the current thread's JNI context at any time.

  Note that native-created threads still have JNI limitations over threads
  that are calls down to native from Java.  The JNI function FindClass will
  NOT find application-specific classes when called from native threads.
  Native code that needs to call FindClass and record the indices of Java
  class members for later access must call FindClass and Get*FieldID/Get*MethodID
  in threads calling from Java, such as JNI_OnLoad
 */

/**
  Initializes the nv_thread system by connecting it to the JVM.  This
  function must be called as early as possible in the native code's
  JNI_OnLoad function, so that the thread system is prepared for any
  JNI-dependent library initialization calls.  
  @param vm The VM pointer - should be the JavaVM pointer sent to JNI_OnLoad.
  */
void NVThreadInit(JavaVM* vm);

/**
  Retrieves the JNIEnv object associated with the current thread, allowing
  any thread that was creating with NVThreadSpawnJNIThread() to access the
  JNI at will.  This JNIEnv is NOT usable across multiple calls or threads
  The function should be called in each function that requires a JNIEnv
  @return The current thread's JNIEnv, or NULL if the thread was not created
  by NVThreadSpawnJNIThread
  @see NVThreadSpawnJNIThread
  */
JNIEnv* NVThreadGetCurrentJNIEnv();

/**
  Spwans a new native thread that is registered for use with JNI.  Threads
  created with this function will have access to JNI data via the JNIEnv
  available from NVThreadGetCurrentJNIEnv().
  @param thread is the same as in pthread_create
  @param attr is the same as in pthread_create
  @param start_routine is the same as in pthread_create
  @param arg is the same as in pthread_create
  @return 0 on success, -1 on failure
  @see NVThreadGetCurrentJNIEnv
*/
int NVThreadSpawnJNIThread(pthread_t *thread, pthread_attr_t const * attr,
    void *(*start_routine)(void *), void * arg);

/**
  Sleeps the current thread for the specified number of milliseconds
  @param millisec Sleep time in ms
  @return 0 on success, -1 on failure
*/
int NVThreadSleep(unsigned long millisec);

#if defined(__cplusplus)
}
#endif

#endif
