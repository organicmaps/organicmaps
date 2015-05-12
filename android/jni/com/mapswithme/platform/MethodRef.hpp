#pragma once

#include "../core/jni_helper.hpp"

class MethodRef
{
public:
  MethodRef(char const * name, char const * signature);
  ~MethodRef();

  void Init(jobject obj);
  void CallVoid(jlong arg);

private:
  char const* m_name;
  char const * m_signature;
  jmethodID m_methodID;
  jobject m_object;
};
