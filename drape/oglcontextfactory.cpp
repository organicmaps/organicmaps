#include "drape/oglcontextfactory.hpp"

namespace dp
{

ThreadSafeFactory::ThreadSafeFactory(OGLContextFactory * factory)
  : m_factory(factory)
{
}

ThreadSafeFactory::~ThreadSafeFactory()
{
  delete m_factory;
}

OGLContext *ThreadSafeFactory::getDrawContext()
{
  threads::MutexGuard lock(m_mutex);
  return m_factory->getDrawContext();
}

OGLContext *ThreadSafeFactory::getResourcesUploadContext()
{
  threads::MutexGuard lock(m_mutex);
  return m_factory->getResourcesUploadContext();
}

} // namespace dp
