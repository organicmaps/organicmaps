#include "Framework.hpp"

#include "../core/jni_helper.hpp"

extern "C"
{

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_RenderFragment_createEngine(JNIEnv * env, jobject thiz, jobject surface, jint destiny)
{
  return static_cast<jboolean>(g_framework->CreateDrapeEngine(env, surface, static_cast<int>(destiny)));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_RenderFragment_surfaceResized(JNIEnv * env, jobject thiz, jint w, jint h)
{
  g_framework->Resize(static_cast<int>(w), static_cast<int>(h));
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_RenderFragment_destroyEngine(JNIEnv * env, jobject thiz)
{
  g_framework->DeleteDrapeEngine();
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_RenderFragment_isEngineCreated(JNIEnv * env, jobject thiz)
{
  return static_cast<jboolean>(g_framework->IsDrapeEngineCreated());
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_RenderFragment_detachSurface(JNIEnv * env, jobject thiz)
{
  g_framework->DetachSurface();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_RenderFragment_attachSurface(JNIEnv * env, jobject thiz, jobject surface)
{
  g_framework->AttachSurface(env, surface);
}

JNIEXPORT jboolean JNICALL
Java_com_mapswithme_maps_RenderFragment_onTouch(JNIEnv * env, jobject thiz, jint action,
                                                jint id1, jfloat x1, jfloat y1,
                                                jint id2, jfloat x2, jfloat y2,
                                                jint maskedPointer)
{
  g_framework->Touch(static_cast<int>(action), android::Framework::Finger(id1, x1, y1),
                     android::Framework::Finger(id2, x2, y2), maskedPointer);

  return JNI_TRUE;
}

}
