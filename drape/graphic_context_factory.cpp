#include "drape/graphic_context_factory.hpp"

namespace dp
{
ThreadSafeFactory::ThreadSafeFactory(GraphicContextFactory * factory, bool enableSharing)
  : m_factory(factory)
  , m_enableSharing(enableSharing)
{}

ThreadSafeFactory::~ThreadSafeFactory()
{
  delete m_factory;
}

GraphicContext * ThreadSafeFactory::GetDrawContext()
{
  return CreateContext([this](){ return m_factory->GetDrawContext(); },
                       [this](){ return m_factory->IsUploadContextCreated(); });
}

GraphicContext * ThreadSafeFactory::GetResourcesUploadContext()
{
  return CreateContext([this](){ return m_factory->GetResourcesUploadContext(); },
                       [this](){ return m_factory->IsDrawContextCreated(); });
}

GraphicContext * ThreadSafeFactory::CreateContext(TCreateCtxFn const & createFn, TIsSeparateCreatedFn const checkFn)
{
  threads::ConditionGuard g(m_contidion);
  GraphicContext * ctx = createFn();
  if (m_enableSharing)
  {
    if (!checkFn())
      g.Wait();
    else
      g.Signal();
  }

  return ctx;
}

void ThreadSafeFactory::WaitForInitialization(GraphicContext * context)
{
  m_factory->WaitForInitialization(context);
}
  
void ThreadSafeFactory::SetPresentAvailable(bool available)
{
  m_factory->SetPresentAvailable(available);
}
}  // namespace dp
