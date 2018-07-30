#pragma once

#include "drape/graphic_context.hpp"

#include "base/condition.hpp"
#include "base/assert.hpp"

#include "std/function.hpp"

namespace dp
{
class GraphicContextFactory
{
public:
  virtual ~GraphicContextFactory() {}
  virtual GraphicContext * GetDrawContext() = 0;
  virtual GraphicContext * GetResourcesUploadContext() = 0;
  virtual bool IsDrawContextCreated() const { return false; }
  virtual bool IsUploadContextCreated() const { return false; }
  virtual void WaitForInitialization(dp::GraphicContext * context) {}
  virtual void SetPresentAvailable(bool available) {}
};

class ThreadSafeFactory : public GraphicContextFactory
{
public:
  ThreadSafeFactory(GraphicContextFactory * factory, bool enableSharing = true);
  ~ThreadSafeFactory();
  GraphicContext * GetDrawContext() override;
  GraphicContext * GetResourcesUploadContext() override;

  template<typename T>
  T * CastFactory()
  {
    ASSERT(dynamic_cast<T *>(m_factory) != nullptr, ());
    return static_cast<T *>(m_factory);
  }

  void WaitForInitialization(dp::GraphicContext * context) override;
  void SetPresentAvailable(bool available) override;

protected:
  typedef function<GraphicContext * ()> TCreateCtxFn;
  typedef function<bool()> TIsSeparateCreatedFn;
  GraphicContext * CreateContext(TCreateCtxFn const & createFn, TIsSeparateCreatedFn const checkFn);

private:
  GraphicContextFactory * m_factory;
  threads::Condition m_contidion;
  bool m_enableSharing;
};
}  // namespace dp
