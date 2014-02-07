#pragma once

#include "../base/thread.hpp"

#ifdef DRAW_INFO
  #include "../base/timer.hpp"
  #include "../std/vector.hpp"
  #include "../std/numeric.hpp"
#endif

#include "message_acceptor.hpp"
#include "threads_commutator.hpp"
#include "tile_info.hpp"
#include "backend_renderer.hpp"

#include "../drape/pointers.hpp"
#include "../drape/glstate.hpp"
#include "../drape/vertex_array_buffer.hpp"
#include "../drape/gpu_program_manager.hpp"
#include "../drape/oglcontextfactory.hpp"

#include "../drape/uniform_values_storage.hpp"

#include "../geometry/screenbase.hpp"

#include "../std/map.hpp"

namespace df
{
  class FrontendRenderer : public MessageAcceptor,
                           public threads::IRoutine
  {
  public:
    FrontendRenderer(RefPointer<ThreadsCommutator> commutator,
                     RefPointer<OGLContextFactory> oglcontextfactory,
                     Viewport viewport);

    ~FrontendRenderer();

#ifdef DRAW_INFO
    double m_tpf;
    double m_fps;

    my::Timer m_timer;
    double m_frameStartTime;
    vector<double> m_tpfs;
    int m_drawedFrames;

    void BeforeDrawFrame();
    void AfterDrawFrame();
#endif

  protected:
    virtual void AcceptMessage(RefPointer<Message> message);

  private:
    void RenderScene();
    void RefreshProjection();
    void RefreshModelView();

    void RenderPartImpl(pair<const GLState, MasterPointer<VertexArrayBuffer> > & node);

  private:
    void StartThread();
    void StopThread();
    void ThreadMain();
    void ReleaseResources();

    virtual void Do();

  private:
    void DeleteRenderData();

  private:
    RefPointer<ThreadsCommutator> m_commutator;
    MasterPointer<GpuProgramManager> m_gpuProgramManager;
    threads::Thread m_selfThread;
    RefPointer<OGLContextFactory> m_contextFactory;

  private:
    typedef multimap<GLState, MasterPointer<VertexArrayBuffer> > render_data_t;
    typedef render_data_t::iterator render_data_iter;
    typedef multimap<TileKey, render_data_iter> tile_data_t;
    typedef tile_data_t::iterator tile_data_iter;
    typedef pair<tile_data_iter, tile_data_iter> tile_data_range_t;
    render_data_t m_renderData;
    tile_data_t   m_tileData;

    UniformValuesStorage m_generalUniforms;
    Viewport m_viewport;
    ScreenBase m_view;
  };
}
