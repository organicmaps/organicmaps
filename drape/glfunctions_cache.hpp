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

  GLFunctionsCache() = default;

  template<typename TValue>
  struct CachedParam
  {
    TValue m_value;
    bool m_inited;

    CachedParam()
      : m_value(TValue())
      , m_inited(false)
    {
    }

    explicit CachedParam(TValue const & value)
      : m_value(value)
      , m_inited(true)
    {
    }

    bool Assign(TValue const & newValue)
    {
      if (m_inited && newValue == m_value)
        return false;

      m_value = newValue;
      m_inited = true;
      return true;
    }

    bool operator!=(TValue const & value) const
    {
      return m_value != value;
    }

    CachedParam & operator=(TValue const & param)
    {
      m_value = param;
      m_inited = true;
      return *this;
    }
  };

  template<typename TValue> using UniformCache = map<int8_t, CachedParam<TValue>>;
  using StateParams = map<glConst, CachedParam<bool>>;

  struct UniformsCache
  {
    UniformCache<int32_t> m_glUniform1iCache;
    UniformCache<float> m_glUniform1fCache;

    bool Assign(int8_t location, int32_t value) { return Assign(location, value, m_glUniform1iCache); }
    bool Assign(int8_t location, float value) { return Assign(location, value, m_glUniform1fCache); }

    template<typename TValue>
    bool Assign(int8_t location, TValue const & value, UniformCache<TValue> & cache)
    {
      return cache[location].Assign(value);
    }
  };

  static UniformsCache & GetCacheForCurrentProgram();

  CachedParam<uint32_t> m_glBindTextureCache;
  CachedParam<glConst> m_glActiveTextureCache;
  CachedParam<uint32_t> m_glUseProgramCache;
  StateParams m_glStateCache;

  map<uint32_t, UniformsCache> m_uniformsCache;
};
