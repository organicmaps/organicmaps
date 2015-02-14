#include "drape/oglcontextfactory.hpp"

namespace dp
{

ThreadSafeFactory::ThreadSafeFactory(OGLContextFactory * factory, bool enableSharing)
  : m_factory(factory)
  , m_enableSharing(enableSharing)
{
}

ThreadSafeFactory::~ThreadSafeFactory()
{
  delete m_factory;
}

OGLContext * ThreadSafeFactory::getDrawContext()
{
  return CreateContext([this](){ return m_factory->getDrawContext(); },
                       [this](){ return m_factory->isUploadContextCreated(); });
}

OGLContext *ThreadSafeFactory::getResourcesUploadContext()
{
  return CreateContext([this](){ return m_factory->getResourcesUploadContext(); },
                       [this](){ return m_factory->isDrawContextCreated(); });
}

OGLContext * ThreadSafeFactory::CreateContext(TCreateCtxFn const & createFn, TIsSeparateCreatedFn const checkFn)
{
  threads::ConditionGuard g(m_contidion);
  OGLContext * ctx = createFn();
  if (m_enableSharing)
  {
    if (!checkFn())
      g.Wait();
    else
      g.Signal();
  }

  return ctx;
}

} // namespace dp
