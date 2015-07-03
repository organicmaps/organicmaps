#pragma once

#include "drape/glconstants.hpp"

#include "std/map.hpp"
#include "std/string.hpp"

class GLFunctionsCache
{
public:

  static void glBindTexture(uint32_t textureID);
  static void glActiveTexture(glConst texBlock);
  static void glUseProgram(uint32_t programID);
  static void glEnable(glConst mode);
  static void glDisable(glConst mode);

  static void glUniformValuei(int8_t location, int32_t v);
  static void glUniformValuef(int8_t location, float v);

private:
  static GLFunctionsCache & Instance();

  template<typename TParam>
  struct CachedParam
  {
    TParam m_param;
    bool m_initialized = false;

    CachedParam() : m_param(TParam()) {}
    explicit CachedParam(TParam const & param) : m_param(param) {}

    bool operator!=(TParam const & param)
    {
      return m_param != param;
    }

    CachedParam & operator=(TParam const & param)
    {
      m_param = param;
      m_initialized = true;
      return *this;
    }
  };

  struct EnablableParam
  {
    bool m_isEnabled = false;
    bool m_initialized = false;

    EnablableParam(){}
    explicit EnablableParam(bool enabled) : m_isEnabled(enabled) {}

    bool operator!=(bool enabled)
    {
      return m_isEnabled != enabled;
    }

    EnablableParam & operator=(bool enabled)
    {
      m_isEnabled = enabled;
      m_initialized = true;
      return *this;
    }
  };

  template<typename TParam> using UniformCache = map<int8_t, CachedParam<TParam>>;

  struct UniformsCache
  {
    UniformCache<int32_t> m_glUniform1iCache;
    UniformCache<float> m_glUniform1fCache;
  };

  template<typename TParamInternal, typename TParam>
  bool CheckAndSetValue(TParamInternal const & newValue, TParam & cachedValue)
  {
    if (!cachedValue.m_initialized || cachedValue != newValue)
    {
      cachedValue = newValue;
      return true;
    }
    return false;
  }

  CachedParam<uint32_t> m_glBindTextureCache;
  CachedParam<glConst> m_glActiveTextureCache;
  CachedParam<uint32_t> m_glUseProgramCache;
  map<glConst, EnablableParam> m_glEnableCache;

  map<uint32_t, UniformsCache> m_uniformsCache;
};
