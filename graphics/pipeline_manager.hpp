#pragma once

#include "std/shared_ptr.hpp"
#include "std/function.hpp"
#include "std/vector.hpp"
#include "std/queue.hpp"

#include "geometry/rect2d.hpp"

#include "graphics/opengl/vertex.hpp"

#include "graphics/display_list_renderer.hpp"
#include "graphics/resource.hpp"
#include "graphics/geometry_pipeline.hpp"

namespace graphics
{
  template <typename pair_t>
  struct first_less
  {
    bool operator()(pair_t const & first, pair_t const & second)
    {
      return first.first < second.first;
    }
  };

  class PipelinesManager : public DisplayListRenderer
  {
  public:

    typedef DisplayListRenderer base_t;

    typedef function<void()> clearPageFn;
    typedef function<void()> overflowFn;

  private:

    vector<GeometryPipeline> m_pipelines;

    typedef pair<uint8_t, uint32_t> id_pair_t;

    typedef priority_queue<pair<size_t, clearPageFn>,
                           vector<pair<size_t, clearPageFn> >,
                           first_less<pair<size_t, clearPageFn> >
                           > clearPageFns;

    vector<clearPageFns> m_clearPageFns;

    typedef priority_queue<pair<size_t, overflowFn>,
                           vector<pair<size_t, overflowFn> >,
                           first_less<pair<size_t, overflowFn> >
                           > overflowFns;

    vector<overflowFns> m_overflowFns;
    void callOverflowFns(uint8_t pipelineID);

    void clearPageHandles(int pipelineID);

  public:

    PipelinesManager(base_t::Params const & params);
    ~PipelinesManager();

    id_pair_t unpackID(uint32_t id) const;
    uint32_t packID(uint8_t, uint32_t) const;

    /// obtain Resource from id
    Resource const * fromID(uint32_t id);

    void addClearPageFn(int pipelineID, clearPageFn fn, int priority);
    void callClearPageFns(int pipelineID);

    /// reserve static pipelines
    unsigned reservePipelines(vector<shared_ptr<ResourceCache> > const & caches,
                              EStorageType storageType,
                              VertexDecl const * decl);

    /// reserve dynamic pipelines
    unsigned reservePipelines(unsigned count,
                              ETextureType textureType,
                              EStorageType storageType,
                              VertexDecl const * decl);

    void freePipeline(int pipelineID);
    void freeTexture(int pipelineID);

    bool flushPipeline(int pipelineID);
    void unlockPipeline(int pipelineID);
    void discardPipeline(int pipelineID);
    void resetPipeline(int pipelineID);
    void clearPipeline(int pipelineID);

    GeometryPipeline const & pipeline(int i) const;
    GeometryPipeline & pipeline(int i);
    unsigned pipelinesCount() const;

    uint32_t invalidHandle() const;
    uint32_t invalidPageHandle() const;

    void clearHandles();

    void memoryWarning();
    void enterBackground();
    void enterForeground();

    void beginFrame();
    void endFrame();
  };
}
