#pragma once

#include "pointers.hpp"
#include "glstate.hpp"
#include "render_bucket.hpp"
#include "attribute_provider.hpp"
#include "overlay_handle.hpp"

#include "../std/map.hpp"
#include "../std/function.hpp"

class RenderBucket;
class AttributeProvider;
class OverlayHandle;

class Batcher
{
public:
  Batcher();
  ~Batcher();

  void InsertTriangleList(GLState const & state, RefPointer<AttributeProvider> params);
  void InsertTriangleList(GLState const & state, RefPointer<AttributeProvider> params,
                          TransferPointer<OverlayHandle> handle);

  void InsertTriangleStrip(GLState const & state, RefPointer<AttributeProvider> params);
  void InsertTriangleStrip(GLState const & state, RefPointer<AttributeProvider> params,
                           TransferPointer<OverlayHandle> handle);

  void InsertTriangleFan(GLState const & state, RefPointer<AttributeProvider> params);
  void InsertTriangleFan(GLState const & state, RefPointer<AttributeProvider> params,
                         TransferPointer<OverlayHandle> handle);

  typedef function<void (GLState const &, TransferPointer<RenderBucket> )> flush_fn;
  void StartSession(flush_fn const & flusher);
  void EndSession();

private:

  template<typename TBacher>
  void InsertTriangles(GLState const & state,
                       RefPointer<AttributeProvider> params,
                       TransferPointer<OverlayHandle> handle);

  class CallbacksWrapper;
  void ChangeBuffer(RefPointer<CallbacksWrapper> wrapper);
  RefPointer<RenderBucket> GetBucket(GLState const & state);

  void FinalizeBucket(GLState const & state);
  void Flush();

private:
  flush_fn m_flushInterface;

private:
  typedef map<GLState, MasterPointer<RenderBucket> > buckets_t;
  buckets_t m_buckets;
};
