#include "drape/glfunctions_cache.hpp"
#include "drape/glfunctions.hpp"

#include "base/assert.hpp"

#define INST GLFunctionsCache::Instance()

GLFunctionsCache & GLFunctionsCache::Instance()
{
  static GLFunctionsCache instance;
  return instance;
}

void GLFunctionsCache::glBindTexture(uint32_t textureID)
{
  if (INST.CheckAndSetValue(textureID, INST.m_glBindTextureCache))
    GLFunctions::glBindTextureImpl(textureID);
}

void GLFunctionsCache::glActiveTexture(glConst texBlock)
{
  if (INST.CheckAndSetValue(texBlock, INST.m_glActiveTextureCache))
    GLFunctions::glActiveTextureImpl(texBlock);
}

void GLFunctionsCache::glUseProgram(uint32_t programID)
{
  if (INST.CheckAndSetValue(programID, INST.m_glUseProgramCache))
    GLFunctions::glUseProgramImpl(programID);
}

void GLFunctionsCache::glEnable(glConst mode)
{
  EnablableParam & param = INST.m_glEnableCache[mode];
  bool const initialized = param.m_initialized;
  if (!initialized || (initialized && !param.m_isEnabled))
  {
    param = true;
    GLFunctions::glEnableImpl(mode);
  }
}

void GLFunctionsCache::glDisable(glConst mode)
{
  EnablableParam & param = INST.m_glEnableCache[mode];
  bool const initialized = param.m_initialized;
  if (!initialized || (initialized && param.m_isEnabled))
  {
    param = false;
    GLFunctions::glDisableImpl(mode);
  }
}

void GLFunctionsCache::glUniformValuei(int8_t location, int32_t v)
{
  ASSERT(INST.m_glUseProgramCache.m_initialized, ());
  UniformsCache & cache = INST.m_uniformsCache[INST.m_glUseProgramCache.m_param];

  CachedParam<int32_t> & param = cache.m_glUniform1iCache[location];
  bool const initialized = param.m_initialized;
  if (!initialized || (initialized && param != v))
  {
    param = v;
    GLFunctions::glUniformValueiImpl(location, v);
  }
}

void GLFunctionsCache::glUniformValuef(int8_t location, float v)
{
  ASSERT(INST.m_glUseProgramCache.m_initialized, ());
  UniformsCache & cache = INST.m_uniformsCache[INST.m_glUseProgramCache.m_param];

  CachedParam<float> & param = cache.m_glUniform1fCache[location];
  bool const initialized = param.m_initialized;
  if (!initialized || (initialized && param != v))
  {
    param = v;
    GLFunctions::glUniformValuefImpl(location, v);
  }
}
