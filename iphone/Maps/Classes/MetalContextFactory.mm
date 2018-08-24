#import "MetalContextFactory.h"

MetalContextFactory::MetalContextFactory(MetalView * view): m_view(view)
{}

MetalContextFactory::~MetalContextFactory()
{}

dp::GraphicsContext * MetalContextFactory::GetDrawContext()
{
  if (m_context == nullptr)
    m_context = make_unique_dp<MetalContext>(m_view);
  return m_context.get();
}

dp::GraphicsContext * MetalContextFactory::GetResourcesUploadContext()
{
  if (m_context == nullptr)
    m_context = make_unique_dp<MetalContext>(m_view);
  return m_context.get();
}

bool MetalContextFactory::IsDrawContextCreated() const
{
  return m_context != nullptr;
}

bool MetalContextFactory::IsUploadContextCreated() const
{
  return m_context != nullptr;
}

void MetalContextFactory::WaitForInitialization(dp::GraphicsContext * context)
{}

void MetalContextFactory::SetPresentAvailable(bool available)
{
  m_context->SetPresentAvailable(available);
}
