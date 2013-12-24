#include "frontend_renderer.hpp"

#include "../base/assert.hpp"
#include "../base/stl_add.hpp"

#include "message_subclasses.hpp"
#include "render_thread.hpp"

#include "../std/bind.hpp"

namespace df
{
  FrontendRenderer::FrontendRenderer(RefPointer<ThreadsCommutator> commutator)
    : m_commutator(commutator)
    , m_gpuProgramManager(new GpuProgramManager())
  {
    StartThread();
  }

  FrontendRenderer::~FrontendRenderer()
  {
    StopThread();
  }

  void FrontendRenderer::AcceptMessage(RefPointer<Message> message)
  {
    switch (message->GetType())
    {
    case Message::FlushTile:
      {
        FlushTileMessage * msg = static_cast<FlushTileMessage *>(message.GetRaw());
        const GLState & state = msg->GetState();
        const TileKey & key = msg->GetKey();
        MasterPointer<VertexArrayBuffer> buffer(msg->GetBuffer());
        render_data_t::iterator renderIterator = m_renderData.insert(make_pair(state, buffer));
        m_tileData.insert(make_pair(key, renderIterator));
        break;
      }
    case Message::DropTile:
      {
        DropTileMessage * msg = static_cast<DropTileMessage *>(message.GetRaw());
        const TileKey & key = msg->GetKey();
        tile_data_range_t range = m_tileData.equal_range(key);
        for (tile_data_iter eraseIter = range.first; eraseIter != range.second; ++eraseIter)
        {
          eraseIter->second->second.Destroy();
          m_renderData.erase(eraseIter->second);
        }
        m_tileData.erase(range.first, range.second);
        break;
      }
    case Message::DropCoverage:
      DeleteRenderData();
      break;
    default:
      ASSERT(false, ());
    }
  }

  namespace
  {
    void render_part(RefPointer<GpuProgramManager> manager , pair<const GLState, MasterPointer<VertexArrayBuffer> > & node)
    {
      ApplyState(node.first, manager->GetProgram(node.first.GetProgramIndex()));
      node.second->Render();
    }
  }

  void FrontendRenderer::RenderScene()
  {
    for_each(m_renderData.begin(), m_renderData.end(), bind(&render_part, m_gpuProgramManager.GetRefPointer(), _1));
  }

  void FrontendRenderer::StartThread()
  {
    m_selfThread.Create(this);
  }

  void FrontendRenderer::StopThread()
  {
    IRoutine::Cancel();
    CloseQueue();
    m_selfThread.Join();
  }

  void FrontendRenderer::ThreadMain()
  {
    InitRenderThread();

    while (!IsCancelled())
    {
      ProcessSingleMessage(false);
      RenderScene();
    }

    ReleaseResources();
  }

  void FrontendRenderer::ReleaseResources()
  {
    DeleteRenderData();
    m_gpuProgramManager.Destroy();
  }

  void FrontendRenderer::Do()
  {
    ThreadMain();
  }

  void FrontendRenderer::DeleteRenderData()
  {
    m_tileData.clear();
    GetRangeDeletor(m_renderData, MasterPointerDeleter())();
  }
}
