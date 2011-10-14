/*
 * GesturesProcessor.cpp
 *
 *  Created on: Oct 13, 2011
 *      Author: siarheirachytski
 */

#include <jni.h>
#include "Framework.hpp"
#include "../../../../../base/assert.hpp"

extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_GesturesProcessor_nativeMove(JNIEnv * env,
      jobject thiz, jint mode, jdouble x, jdouble y)
  {
    ASSERT ( g_framework, () );
    g_framework->Move(mode, x, y);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_GesturesProcessor_nativeZoom(JNIEnv * env,
      jobject thiz, jint mode, jdouble x1, jdouble y1, jdouble x2, jdouble y2)
  {
    ASSERT ( g_framework, () );
    g_framework->Zoom(mode, x1, y1, x2, y2);
  }
}
