#pragma once

#include "drape/graphics_context.hpp"

#include "base/condition.hpp"
#include "base/assert.hpp"

#include "std/function.hpp"

namespace dp
{
class GraphicsContextFactory
{
public:
  virtual ~GraphicsContextFactory() {}
  virtual GraphicsContext * GetDrawContext() = 0;
  virtual GraphicsContext * GetResourcesUploadContext() = 0;
  virtual bool IsDrawContextCreated() const { return false; }
  virtual bool IsUploadContextCreated() const { return false; }
  virtual void WaitForInitialization(dp::GraphicsContext * context) {}
  virtual void SetPresentAvailable(bool available) {}
};

class ThreadSafeFactory : public GraphicsContextFactory
{
public:
  ThreadSafeFactory(GraphicsContextFactory * factory, bool enableSharing = true);
  ~ThreadSafeFactory();
  GraphicsContext * GetDrawContext() override;
  GraphicsContext * GetResourcesUploadContext() override;

  template<typename T>
  T * CastFactory()
  {
    ASSERT(dynamic_cast<T *>(m_factory) != nullptr, ());
    return static_cast<T *>(m_factory);
  }

  void WaitForInitialization(dp::GraphicsContext * context) override;
  void SetPresentAvailable(bool available) override;

protected:
  typedef function<GraphicsContext * ()> TCreateCtxFn;
  typedef function<bool()> TIsSeparateCreatedFn;
  GraphicsContext * CreateContext(TCreateCtxFn const & createFn, TIsSeparateCreatedFn const checkFn);

private:
  GraphicsContextFactory * m_factory;
  threads::Condition m_contidion;
  bool m_enableSharing;
};
}  // namespace dp
