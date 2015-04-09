#pragma once

#include "drape/oglcontext.hpp"

#include "base/mutex.hpp"

namespace dp
{

class OGLContextFactory
{
public:
  virtual ~OGLContextFactory() {}
  virtual OGLContext * getDrawContext() = 0;
  virtual OGLContext * getResourcesUploadContext() = 0;
};

class ThreadSafeFactory : public OGLContextFactory
{
public:
  ThreadSafeFactory(OGLContextFactory * factory);
  ~ThreadSafeFactory();
  virtual OGLContext * getDrawContext();
  virtual OGLContext * getResourcesUploadContext();

private:
  OGLContextFactory * m_factory;
  threads::Mutex m_mutex;
};

} // namespace dp
