#include "MethodRef.hpp"

#include <base/assert.hpp>
#include <base/logging.hpp>

MethodRef::MethodRef(char const * name, char const * signature)
 : m_name(name)
 , m_signature(signature)
 , m_methodID(nullptr)
 , m_object(nullptr)
{
  LOG(LDEBUG, ("Name = ", name));
}

MethodRef::~MethodRef()
{
  if (m_object != nullptr)
    jni::GetEnv()->DeleteGlobalRef(m_object);
}

void MethodRef::Init(JNIEnv * env, jobject obj)
{
  m_object = env->NewGlobalRef(obj);
  jclass k = env->GetObjectClass(m_object);
  m_methodID = env->GetMethodID(k, m_name, m_signature);
  ASSERT(m_object != nullptr, ());
  ASSERT(m_methodID != nullptr, ());
}

void MethodRef::CallVoid(jlong arg)
{
  JNIEnv * jniEnv = jni::GetEnv();
  ASSERT(jniEnv != nullptr, ());
  ASSERT(m_object != nullptr, ());
  ASSERT(m_methodID != nullptr, ());
  jniEnv->CallVoidMethod(m_object, m_methodID, static_cast<jlong>(arg));
}
