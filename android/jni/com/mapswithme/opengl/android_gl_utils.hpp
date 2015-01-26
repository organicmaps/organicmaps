#pragma once

#include "../../../../../base/src_point.hpp"

#include <EGL/egl.h>
#include <GLES2/gl2.h>

namespace android
{

class ConfigComparator
{
public:
  ConfigComparator(EGLDisplay display);

  int operator()(EGLConfig const & l, EGLConfig const & r) const;
  int configWeight(EGLConfig const & config) const;

private:
  EGLDisplay m_display;
};

void CheckEGL(my::SrcPoint const & src);

#define CHECK_EGL(x) do { (x); CheckEGL(SRC());} while(false);
#define CHECK_EGL_CALL() do { CheckEGL(SRC());} while (false);

} // namespace android

