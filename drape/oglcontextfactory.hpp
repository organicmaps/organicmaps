#pragma once

#include "drape/oglcontext.hpp"

#include "base/condition.hpp"
#include "base/assert.hpp"

#include "std/function.hpp"

namespace dp
{

class OGLContextFactory
{
public:
  virtual ~OGLContextFactory() {}
  virtual OGLContext * getDrawContext() = 0;
  virtual OGLContext * getResourcesUploadContext() = 0;
  virtual bool isDrawContextCreated() const { return false; }
  virtual bool isUploadContextCreated() const { return false; }
};

class ThreadSafeFactory : public OGLContextFactory
{
public:
  ThreadSafeFactory(OGLContextFactory * factory, bool enableSharing = true);
  ~ThreadSafeFactory();
  virtual OGLContext * getDrawContext();
  virtual OGLContext * getResourcesUploadContext();

  template<typename T>
  T * CastFactory()
  {
    ASSERT(dynamic_cast<T *>(m_factory) != nullptr, ());
    return static_cast<T *>(m_factory);
  }

protected:
  typedef function<OGLContext * ()> TCreateCtxFn;
  typedef function<bool()> TIsSeparateCreatedFn;
  OGLContext * CreateContext(TCreateCtxFn const & createFn, TIsSeparateCreatedFn const checkFn);

private:
  OGLContextFactory * m_factory;
  threads::Condition m_contidion;
  bool m_enableSharing;
};

} // namespace dp
