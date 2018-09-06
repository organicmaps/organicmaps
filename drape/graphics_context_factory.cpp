#include "drape/graphics_context_factory.hpp"

namespace dp
{
ThreadSafeFactory::ThreadSafeFactory(GraphicsContextFactory * factory, bool enableSharing)
  : m_factory(factory)
  , m_enableSharing(enableSharing)
{}

ThreadSafeFactory::~ThreadSafeFactory()
{
  delete m_factory;
}

GraphicsContext * ThreadSafeFactory::GetDrawContext()
{
  return CreateContext([this](){ return m_factory->GetDrawContext(); },
                       [this](){ return m_factory->IsUploadContextCreated(); });
}

GraphicsContext * ThreadSafeFactory::GetResourcesUploadContext()
{
  return CreateContext([this](){ return m_factory->GetResourcesUploadContext(); },
                       [this](){ return m_factory->IsDrawContextCreated(); });
}

GraphicsContext * ThreadSafeFactory::CreateContext(TCreateCtxFn const & createFn,
                                                   TIsSeparateCreatedFn const & checkFn)
{
  threads::ConditionGuard g(m_condition);
  GraphicsContext * ctx = createFn();
  if (m_enableSharing)
  {
    if (!checkFn())
      g.Wait();
    else
      g.Signal();
  }

  return ctx;
}

void ThreadSafeFactory::WaitForInitialization(GraphicsContext * context)
{
  m_factory->WaitForInitialization(context);
}
  
void ThreadSafeFactory::SetPresentAvailable(bool available)
{
  m_factory->SetPresentAvailable(available);
}
}  // namespace dp
