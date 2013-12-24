#pragma once

#include "../base/thread.hpp"

#include "message_acceptor.hpp"
#include "threads_commutator.hpp"
#include "tile_info.hpp"
#include "backend_renderer.hpp"

#include "../drape/pointers.hpp"
#include "../drape/glstate.hpp"
#include "../drape/vertex_array_buffer.hpp"
#include "../drape/gpu_program_manager.hpp"

#include "../std/map.hpp"

namespace df
{
  class FrontendRenderer : public MessageAcceptor,
                           public threads::IRoutine
  {
  public:
    FrontendRenderer(RefPointer<ThreadsCommutator> commutator);
    ~FrontendRenderer();

  protected:
    virtual void AcceptMessage(RefPointer<Message> message);

  private:
    void RenderScene();

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

  private:
    typedef multimap<GLState, MasterPointer<VertexArrayBuffer> > render_data_t;
    typedef render_data_t render_data_iter;
    typedef multimap<TileKey, render_data_t::iterator> tile_data_t;
    typedef tile_data_t::iterator tile_data_iter;
    typedef pair<tile_data_iter, tile_data_iter> tile_data_range_t;
    render_data_t m_renderData;
    tile_data_t   m_tileData;
  };
}
