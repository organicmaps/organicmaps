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
  if (INST.m_glBindTextureCache.Assign(textureID))
    GLFunctions::glBindTextureImpl(textureID);
}

void GLFunctionsCache::glActiveTexture(glConst texBlock)
{
  if (INST.m_glActiveTextureCache.Assign(texBlock))
    GLFunctions::glActiveTextureImpl(texBlock);
}

void GLFunctionsCache::glUseProgram(uint32_t programID)
{
  if (INST.m_glUseProgramCache.Assign(programID))
    GLFunctions::glUseProgramImpl(programID);
}

void GLFunctionsCache::glEnable(glConst mode)
{
  if (INST.m_glStateCache[mode].Assign(true))
    GLFunctions::glEnableImpl(mode);
}

void GLFunctionsCache::glDisable(glConst mode)
{
  if (INST.m_glStateCache[mode].Assign(false))
    GLFunctions::glDisableImpl(mode);
}

GLFunctionsCache::UniformsCache & GLFunctionsCache::GetCacheForCurrentProgram()
{
  GLFunctionsCache & cache = INST;

  ASSERT(cache.m_glUseProgramCache.m_inited, ());
  return cache.m_uniformsCache[cache.m_glUseProgramCache.m_value];
}

void GLFunctionsCache::glUniformValuei(int8_t location, int32_t v)
{
  if (GetCacheForCurrentProgram().Assign(location, v))
    GLFunctions::glUniformValueiImpl(location, v);
}

void GLFunctionsCache::glUniformValuef(int8_t location, float v)
{
  if (GetCacheForCurrentProgram().Assign(location, v))
    GLFunctions::glUniformValuefImpl(location, v);
}
