#include "../base/SRC_FIRST.hpp"
#include "renderer.hpp"
#include "utils.hpp"
#include "framebuffer.hpp"
#include "renderbuffer.hpp"
#include "resource_manager.hpp"
#include "internal/opengl.hpp"
#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    const yg::Color Renderer::s_bgColor(192, 192, 192, 255);

    Renderer::BaseState::BaseState()
      : m_isDebugging(false)
    {}

    Renderer::BaseState::~BaseState()
    {}

    bool Renderer::Command::isDebugging() const
    {
      return m_isDebugging;
    }

    void Renderer::Command::setIsDebugging(bool flag)
    {
      m_isDebugging = flag;
    }

    Renderer::Command::Command()
      : m_isDebugging(false)
    {}

    Renderer::Command::~Command()
    {}

    void Renderer::State::apply(BaseState const * prevBase)
    {
      State const * prevState = static_cast<State const *>(prevBase);

      if (m_frameBuffer)
      {
        bool shouldApply = false;

        if (m_frameBuffer == prevState->m_frameBuffer)
        {
          if (m_isDebugging)
          {
            std::ostringstream out;
            out << "equal frameBuffers, ";
            if (m_frameBuffer)
              out << m_frameBuffer->id() << ", " << prevState->m_frameBuffer->id();
            else
              out << "(null), (null)";

            LOG(LINFO, (out.str().c_str()));
          }

          if (m_renderTarget != prevState->m_renderTarget)
          {
            if (m_isDebugging)
            {
              std::ostringstream out;
              out << "non-equal renderBuffers, ";

              if (prevState->m_renderTarget)
                out << prevState->m_renderTarget->id();
              else
                out << "(null)";

              out << " => ";

              if (m_renderTarget)
                out << m_renderTarget->id();
              else
                out << "(null)";

              LOG(LINFO, (out.str().c_str()));
            }

            m_frameBuffer->setRenderTarget(m_renderTarget);
            shouldApply = true;
          }
          else
          {
            if (m_isDebugging)
            {
              std::ostringstream out;
              out << "equal renderBuffers, ";

              if (m_renderTarget)
                out << m_renderTarget->id() << ", " << m_renderTarget->id();
              else
                out << "(null), (null)";
              LOG(LINFO, (out.str().c_str()));
            }
          }

          if (m_depthBuffer != prevState->m_depthBuffer)
          {
            if (m_isDebugging)
            {
              std::ostringstream out;
              out << "non-equal depthBuffers, ";

              if (prevState->m_depthBuffer)
                out << prevState->m_depthBuffer->id();
              else
                out << "(null)";

              out << " => ";

              if (m_depthBuffer)
                out << m_depthBuffer->id();
              else
                out << "(null)";

              LOG(LINFO, (out.str().c_str()));
            }

            m_frameBuffer->setDepthBuffer(m_depthBuffer);
            shouldApply = true;
          }
          else
          {
            if (m_isDebugging)
            {
              std::ostringstream out;
              out << "equal depthBuffers, ";
              if (m_depthBuffer)
                out << m_depthBuffer->id() << ", " << m_depthBuffer->id();
              else
                out << "(null), (null)";
              LOG(LINFO, (out.str().c_str()));
            }
          }

        }
        else
        {
          if (m_isDebugging)
          {
            ostringstream out;
            out << "non-equal frameBuffers, ";
            if (prevState->m_frameBuffer)
              out << prevState->m_frameBuffer->id() << ", ";
            else
              out << "(null)";

            out << " => ";

            if (m_frameBuffer)
              out << m_frameBuffer->id() << ", ";
            else
              out << "(null)";

            LOG(LINFO, (out.str().c_str()));

            out.str("");
            out << "renderTarget=";
            if (m_renderTarget)
              out << m_renderTarget->id();
            else
              out << "(null)";

            out << ", depthBuffer=";
            if (m_depthBuffer)
              out << m_depthBuffer->id();
            else
              out << "(null)";

            LOG(LINFO, (out.str().c_str()));
          }

          m_frameBuffer->setRenderTarget(m_renderTarget);
          m_frameBuffer->setDepthBuffer(m_depthBuffer);
          shouldApply = true;
        }

        if (shouldApply)
          m_frameBuffer->makeCurrent();
      }
      else
        CHECK(false, ());
    }

    Renderer::Packet::Packet()
    {}

    Renderer::Packet::Packet(shared_ptr<Command> const & command)
      : m_command(command)
    {}

    Renderer::Packet::Packet(shared_ptr<BaseState> const & state,
                             shared_ptr<Command> const & command)
      : m_state(state), m_command(command)
    {
      if (m_state && m_command)
        m_state->m_isDebugging = m_command->isDebugging();
    }

    Renderer::Params::Params()
      : m_isDebugging(false),
        m_renderQueue(0)
    {}

    Renderer::Renderer(Params const & params)
      : m_isDebugging(params.m_isDebugging),
        m_isRendering(false)
    {
      m_frameBuffer = params.m_frameBuffer;
      m_resourceManager = params.m_resourceManager;

      if (m_frameBuffer)
      {
        m_renderTarget = m_frameBuffer->renderTarget();
        m_depthBuffer = m_frameBuffer->depthBuffer();
      }

      m_renderQueue = params.m_renderQueue;
    }

    Renderer::~Renderer()
    {}

    shared_ptr<ResourceManager> const & Renderer::resourceManager() const
    {
      return m_resourceManager;
    }

    void Renderer::beginFrame()
    {
      m_isRendering = true;

      if (m_renderQueue)
        return;

      if (m_frameBuffer)
        m_frameBuffer->makeCurrent();
    }

    bool Renderer::isRendering() const
    {
      return m_isRendering;
    }

    void Renderer::endFrame()
    {
        m_isRendering = false;
    }

    shared_ptr<FrameBuffer> const & Renderer::frameBuffer() const
    {
      return m_frameBuffer;
    }

    shared_ptr<RenderTarget> const & Renderer::renderTarget() const
    {
      return m_renderTarget;
    }

    void Renderer::setRenderTarget(shared_ptr<RenderTarget> const & rt)
    {
      m_renderTarget = rt;

      if (!m_renderQueue)
      {
        m_frameBuffer->setRenderTarget(rt);
        m_frameBuffer->makeCurrent(); //< to attach renderTarget
      }
    }

    shared_ptr<RenderBuffer> const & Renderer::depthBuffer() const
    {
      return m_depthBuffer;
    }

    void Renderer::setDepthBuffer(shared_ptr<RenderBuffer> const & rt)
    {
      m_depthBuffer = rt;

      if (!m_renderQueue)
        m_frameBuffer->setDepthBuffer(rt);
    }

    Renderer::ClearCommand::ClearCommand(yg::Color const & color,
                                         bool clearRT,
                                         float depth,
                                         bool clearDepth)
      : m_color(color),
        m_clearRT(clearRT),
        m_depth(depth),
        m_clearDepth(clearDepth)
    {}

    void Renderer::ClearCommand::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing clear command"));
      OGLCHECK(glClearColor(m_color.r / 255.0f,
                            m_color.g / 255.0f,
                            m_color.b / 255.0f,
                            m_color.a / 255.0f));
 #ifdef OMIM_GL_ES
      OGLCHECK(glClearDepthf(m_depth));
 #else
      OGLCHECK(glClearDepth(m_depth));
 #endif

      GLbitfield mask = 0;
      if (m_clearRT)
        mask |= GL_COLOR_BUFFER_BIT;
      if (m_clearDepth)
        mask |= GL_DEPTH_BUFFER_BIT;

      OGLCHECK(glDepthMask(GL_TRUE));

      OGLCHECK(glClear(mask));
    }

    void Renderer::clear(yg::Color const & c, bool clearRT, float depth, bool clearDepth)
    {
      shared_ptr<ClearCommand> command(new ClearCommand(c, clearRT, depth, clearDepth));
      processCommand(command);
    }

    shared_ptr<Renderer::BaseState> const Renderer::createState() const
    {
      return shared_ptr<BaseState>(new State());
    }

    void Renderer::getState(BaseState * baseState)
    {
      State * state = static_cast<State *>(baseState);

      state->m_frameBuffer = m_frameBuffer;
      state->m_renderTarget = m_renderTarget;
      state->m_depthBuffer = m_depthBuffer;
      state->m_resourceManager = m_resourceManager;
    }

    void Renderer::FinishCommand::perform()
    {
      if (m_isDebugging)
        LOG(LINFO, ("performing FinishCommand command"));
      OGLCHECK(glFinish());
    }

    void Renderer::finish()
    {
      shared_ptr<Command> command(new FinishCommand());
      processCommand(command);
    }

    void Renderer::onSize(unsigned int width, unsigned int height)
    {
      if (width < 2) width = 2;
      if (height < 2) height = 2;

      m_width = width;
      m_height = height;

      if (m_frameBuffer)
        m_frameBuffer->onSize(width, height);
    }

    unsigned int Renderer::width() const
    {
      return m_width;
    }

    unsigned int Renderer::height() const
    {
      return m_height;
    }

    bool Renderer::isDebugging() const
    {
      return m_isDebugging;
    }

    void Renderer::processCommand(shared_ptr<Command> const & command)
    {
//      command->m_isDebugging = false;
      command->m_isDebugging = renderQueue();

      if (renderQueue())
      {
        shared_ptr<BaseState> state = createState();
        getState(state.get());
        m_renderQueue->PushBack(Packet(state, command));
      }
      else
        command->perform();
    }

    ThreadedList<Renderer::Packet> * Renderer::renderQueue()
    {
      return m_renderQueue;
    }
  }
}
