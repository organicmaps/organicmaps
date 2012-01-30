/*
 * jni_thread.hpp
 *
 *  Created on: Nov 27, 2011
 *      Author: siarheirachytski
 */

#pragma once

#include <jni.h>

namespace jni
{
  void SetCurrentJVM(JavaVM * jvm);
  JavaVM * GetCurrentJVM();

  JNIEnv * GetCurrentThreadJNIEnv();
}
