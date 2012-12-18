#pragma once

#include "../std/shared_ptr.hpp"
#include "../std/function.hpp"
#include "../std/vector.hpp"
#include "../std/queue.hpp"

#include "../geometry/rect2d.hpp"

#include "opengl/vertex.hpp"

#include "display_list_renderer.hpp"
#include "resource.hpp"

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

    struct GeometryPipeline
    {
      size_t m_verticesDrawn;
      size_t m_indicesDrawn;

      size_t m_currentVertex;
      size_t m_currentIndex;

      /// made mutable to implement lazy reservation of m_storage
      /// @{
      mutable size_t m_maxVertices;
      mutable size_t m_maxIndices;

      mutable bool m_hasStorage;
      mutable gl::Storage m_storage;

      mutable gl::Vertex * m_vertices;
      mutable unsigned short * m_indices;
      /// @}

      ETextureType m_textureType;
      EStorageType m_storageType;

      int  verticesLeft();
      int  indicesLeft();

      ETextureType textureType() const;
      void setTextureType(ETextureType type);
      void setStorageType(EStorageType type);

      shared_ptr<ResourceCache> m_cache;

      void checkStorage(shared_ptr<ResourceManager> const & resourceManager) const;
    };

  private:

    vector<GeometryPipeline> m_pipelines;

    uint8_t m_startStaticPage;
    uint8_t m_staticPagesCount;

    uint8_t m_startDynamicPage;
    uint8_t m_dynamicPage;
    uint8_t m_dynamicPagesCount;

    typedef pair<uint8_t, uint32_t> id_pair_t;
    id_pair_t unpackID(uint32_t id) const;
    uint32_t packID(uint8_t, uint32_t) const;

    typedef priority_queue<pair<size_t, clearPageFn>,
                           vector<pair<size_t, clearPageFn> >,
                           first_less<pair<size_t, clearPageFn> >
                           > clearPageFns;

    vector<clearPageFns> m_clearPageFns;
    void callClearPageFns(int pipelineID);

    typedef priority_queue<pair<size_t, overflowFn>,
                           vector<pair<size_t, overflowFn> >,
                           first_less<pair<size_t, overflowFn> >
                           > overflowFns;

    vector<overflowFns> m_overflowFns;
    void callOverflowFns(uint8_t pipelineID);

    void clearPageHandles(int pipelineID);

  public:

    struct Params : public base_t::Params
    {
      string m_skinName;
    };

    PipelinesManager(Params const & params);
    ~PipelinesManager();

    /// reserve static pipelines
    unsigned reservePipelines(vector<shared_ptr<ResourceCache> > const & caches,
                              EStorageType storageType);
    /// reserve dynamic pipelines
    unsigned reservePipelines(unsigned count,
                              ETextureType textureType,
                              EStorageType storageType);

    bool isDynamicPage(int i) const;
    void flushDynamicPage();
    int  nextDynamicPage() const;
    void changeDynamicPage();

    void onDynamicOverflow(int pipelineID);
    void freePipeline(int pipelineID);
    void freeTexture(int pipelineID);

    bool flushPipeline(int pipelineID);
    void unlockPipeline(int pipelineID);
    void discardPipeline(int pipelineID);
    void reset(int pipelineID);

  public:

    /// obtain Resource from id
    Resource const * fromID(uint32_t id);

    /// map Resource::Info on skin
    /// if found - return id.
    /// if not - pack and return id.
    uint32_t mapInfo(Resource::Info const & info);
    /// map array of Resource::Info's on skin
    bool mapInfo(Resource::Info const * const * infos,
             uint32_t * ids,
             size_t count);

    uint32_t findInfo(Resource::Info const & info);

    /// adding function which will be called, when some SkinPage
    /// is getting cleared.
    void addClearPageFn(int pipelineID, clearPageFn fn, int priority);

    GeometryPipeline const & pipeline(int i) const;
    GeometryPipeline & pipeline(int i);
    unsigned pipelinesCount() const;

    uint32_t invalidHandle() const;
    uint32_t invalidPageHandle() const;

    uint8_t dynamicPage() const;

    /// change page for its "backbuffer" counterpart.
    /// this function is called after any rendering command
    /// issued to avoid the "GPU is waiting on texture used in
    /// rendering call" issue.
    /// @warning does nothing for static pages
    /// (pages loaded at skin creation time)
    /// and text pages.
    void changePage(int i);
    int nextPage(int i) const;

    void clearHandles();

    void memoryWarning();
    void enterBackground();
    void enterForeground();

    void beginFrame();
    void endFrame();
  };
}
