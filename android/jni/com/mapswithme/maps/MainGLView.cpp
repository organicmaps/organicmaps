/*
 * MainGLView.cpp
 *
 *  Created on: Oct 13, 2011
 *      Author: siarheirachytski
 */

#include <jni.h>
#include "Framework.hpp"

extern "C"
{
  ///////////////////////////////////////////////////////////////////////////////////
  // MainGLView
  ///////////////////////////////////////////////////////////////////////////////////

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MainGLView_nativeInit(JNIEnv * env, jobject thiz)
  {
    ASSERT ( g_framework, () );
    g_framework->SetParentView(thiz);
  }

  ///////////////////////////////////////////////////////////////////////////////////
  // MainRenderer
  ///////////////////////////////////////////////////////////////////////////////////

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MainRenderer_nativeInit(JNIEnv * env, jobject thiz)
  {
    ASSERT ( g_framework, () );
    g_framework->InitRenderer();
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MainRenderer_nativeResize(JNIEnv * env, jobject thiz, jint w, jint h)
  {
    ASSERT ( g_framework, () );
    g_framework->Resize(w, h);
  }

  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_MainRenderer_nativeDraw(JNIEnv * env, jobject thiz)
  {
    ASSERT ( g_framework, () );
    g_framework->DrawFrame();
  }
}
