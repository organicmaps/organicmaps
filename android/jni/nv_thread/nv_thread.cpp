//----------------------------------------------------------------------------------
// File:            libs\jni\nv_thread\nv_thread.c
// Samples Version: NVIDIA Android Lifecycle samples 1_0beta 
// Email:           tegradev@nvidia.com
// Web:             http://developer.nvidia.com/category/zone/mobile-development
//
// Copyright 2009-2011 NVIDIA� Corporation 
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

#include <jni.h>

#include "nv_thread.hpp"
#include <android/log.h>
#include <pthread.h>

static JavaVM* s_vm = NULL;
static pthread_key_t s_jniEnvKey = 0;

#define MODULE "NVThread"

#include "../nv_debug/nv_debug.hpp"
#include "../com/mapswithme/core/jni_helper.hpp"

void NVThreadInit(JavaVM* vm)
{
  s_vm = vm;
}

JNIEnv* NVThreadGetCurrentJNIEnv()
{
  return jni::GetEnv();

//  JNIEnv* env = NULL;
//  if (s_jniEnvKey)
//  {
//    env = (JNIEnv*)pthread_getspecific(s_jniEnvKey);
//  }
//  else
//  {
//    pthread_key_create(&s_jniEnvKey, NULL);
//  }
//
//  if (!env)
//  {
//    // do we have a VM cached?
//    if (!s_vm)
//    {
//      __android_log_print(ANDROID_LOG_DEBUG, MODULE,  "Error - could not find JVM!");
//      return NULL;
//    }
//
//    // Hmm - no env for this thread cached yet
//    int error = s_vm->AttachCurrentThread(&env, NULL);
//    __android_log_print(ANDROID_LOG_DEBUG, MODULE,  "AttachCurrentThread: %d, 0x%p", error, env);
//    if (error || !env)
//    {
//      __android_log_print(ANDROID_LOG_DEBUG, MODULE,  "Error - could not attach thread to JVM!");
//      return NULL;
//    }
//
//    pthread_setspecific(s_jniEnvKey, env);
//  }
//
//  return env;
}

typedef struct NVThreadInitStruct
{
  void* m_arg;
  void *(*m_startRoutine)(void *);
} NVThreadInitStruct;

// Implementations are in PThreadImpl.cpp
// They're used automatically if thread is created with base/thread.hpp
// @TODO: refactor and remove
void AndroidThreadAttachToJVM();
void AndroidThreadDetachFromJVM();

static void* NVThreadSpawnProc(void* arg)
{
  NVThreadInitStruct* init = (NVThreadInitStruct*)arg;
  void *(*start_routine)(void *) = init->m_startRoutine;
  void* data = init->m_arg;
  void* ret;

  free(arg);

  AndroidThreadAttachToJVM();

  ret = start_routine(data);

  AndroidThreadDetachFromJVM();

  return ret;
}

int NVThreadSpawnJNIThread(pthread_t *thread, pthread_attr_t const * attr,
    void *(*start_routine)(void *), void * arg)
{
  if (!start_routine)
    return -1;

  NVThreadInitStruct * initData = new NVThreadInitStruct;

  initData->m_startRoutine = start_routine;
  initData->m_arg = arg;

  int err = pthread_create(thread, attr, NVThreadSpawnProc, initData);

  // If the thread was not started, then we need to delete the init data ourselves
  if (err)
    free(initData);

  return err;
}

// on linuces, signals can interrupt sleep functions, so you might need to 
// retry to get the full sleep going. I'm not entirely sure this is necessary
// *here* clients could retry themselves when the exposed function returns
// nonzero
inline int __sleep(const struct timespec *req, struct timespec *rem)
{
  int ret = 1;
  int i;
  static const int sleepTries = 2;

  struct timespec req_tmp={0}, rem_tmp={0};

  rem_tmp = *req;
  for(i = 0; i < sleepTries; ++i)
  {
    req_tmp = rem_tmp;
    int ret = nanosleep(&req_tmp, &rem_tmp);
    if(ret == 0)
    {
        ret = 0;
        break;
    }
  }
  if(rem)
    *rem = rem_tmp;
  return ret;
}

int NVThreadSleep(unsigned long millisec)
{
  struct timespec req={0},rem={0};
  time_t sec  =(int)(millisec/1000);

  millisec     = millisec-(sec*1000);
  req.tv_sec  = sec;
  req.tv_nsec = millisec*1000000L;
  return __sleep(&req,&rem);
}
