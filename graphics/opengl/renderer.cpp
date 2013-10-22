#include "../../base/logging.hpp"

#include "../resource_manager.hpp"
#include "../render_context.hpp"
#include "../coordinates.hpp"

#include "renderer.hpp"
#include "data_traits.hpp"
#include "utils.hpp"
#include "framebuffer.hpp"
#include "renderbuffer.hpp"
#include "base_texture.hpp"
#include "program.hpp"
#include "opengl.hpp"

namespace graphics
{
  namespace gl
  {
    const graphics::Color Renderer::s_bgColor(192, 192, 192, 255);

    Renderer::Params::Params()
      : m_isDebugging(false),
        m_doUnbindRT(false),
        m_isSynchronized(true),
        m_renderQueue(0),
        m_threadSlot(-1)
    {}

    Renderer::Renderer(Params const & params)
      : m_isDebugging(params.m_isDebugging),
        m_doUnbindRT(params.m_doUnbindRT),
        m_isSynchronized(params.m_isSynchronized),
        m_isRendering(false),
        m_width(0),
        m_height(0),
        m_env(0),
        m_threadSlot(params.m_threadSlot)
    {
      m_renderContext = params.m_renderContext;
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
      CHECK(!m_isRendering, ("beginFrame called inside beginFrame/endFrame pair!"));
      m_isRendering = true;

      math::Matrix<float, 4, 4> coordM;
      getOrthoMatrix(coordM,
                     0, m_width,
                     m_height, 0,
                     graphics::minDepth,
                     graphics::maxDepth + 100);


      if (m_frameBuffer)
      {
        m_frameBuffer->setRenderTarget(m_renderTarget);
        m_frameBuffer->setDepthBuffer(m_depthBuffer);

        processCommand(make_shared_ptr(new ChangeFrameBuffer(m_frameBuffer)));

        if (m_renderTarget != 0)
          m_renderTarget->coordMatrix(coordM);
      }

      processCommand(make_shared_ptr(new ChangeMatrix(EProjection, coordM)));
      processCommand(make_shared_ptr(new ChangeMatrix(EModelView, math::Identity<float, 4>())));
    }

    bool Renderer::isRendering() const
    {
      return m_isRendering;
    }

    Renderer::UnbindRenderTarget::UnbindRenderTarget(shared_ptr<RenderTarget> const & rt)
      : m_renderTarget(rt)
    {}

    void Renderer::UnbindRenderTarget::perform()
    {
      m_renderTarget->detachFromFrameBuffer();
    }

    void Renderer::unbindRenderTarget()
    {
      processCommand(make_shared_ptr(new UnbindRenderTarget(m_renderTarget)));
    }

    Renderer::ChangeMatrix::ChangeMatrix(EMatrix mt, math::Matrix<float, 4, 4> const & m)
      : m_matrixType(mt), m_matrix(m)
    {}

    void Renderer::ChangeMatrix::perform()
    {
      renderContext()->setMatrix(m_matrixType, m_matrix);
    }

    Renderer::DiscardFramebuffer::DiscardFramebuffer(bool doDiscardColor, bool doDiscardDepth)
        : m_doDiscardColor(doDiscardColor), m_doDiscardDepth(doDiscardDepth)
    {}

    void Renderer::DiscardFramebuffer::perform()
    {
      GLenum attachments[2];
      int numAttachments = 0;
      if (m_doDiscardColor)
        attachments[numAttachments++] = GL_COLOR_ATTACHMENT0_MWM;
      if (m_doDiscardDepth)
        attachments[numAttachments++] = GL_DEPTH_ATTACHMENT_MWM;
      glDiscardFramebufferFn(GL_FRAMEBUFFER_MWM, numAttachments, attachments);
    }

    void Renderer::discardFramebuffer(bool doDiscardColor, bool doDiscardDepth)
    {
      static bool firstReport = true;
      if (firstReport)
      {
        if (!glDiscardFramebufferFn)
          LOG(LINFO, ("GL_EXT_discard_framebuffer is unsupported"));
        firstReport = false;
      }

      if (glDiscardFramebufferFn)
        processCommand(make_shared_ptr(new DiscardFramebuffer(doDiscardColor, doDiscardDepth)));
    }

    void Renderer::copyFramebufferToImage(shared_ptr<BaseTexture> target)
    {
      processCommand(make_shared_ptr(new CopyFramebufferToImage(target)));
    }

    void Renderer::endFrame()
    {
      CHECK(m_isRendering, ("endFrame called outside beginFrame/endFrame pair!"));

      if (m_doUnbindRT && m_renderTarget)
        unbindRenderTarget();

      if (m_isSynchronized)
        finish();

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
      CHECK(!isRendering(), ("should call this function only outside beginFrame/endFrame"));
      m_renderTarget = rt;
    }

    void Renderer::resetRenderTarget()
    {
      CHECK(!isRendering(), ("should call this function only outside beginFrame/endFrame"));
      m_renderTarget.reset();
    }

    shared_ptr<RenderBuffer> const & Renderer::depthBuffer() const
    {
      return m_depthBuffer;
    }

    void Renderer::setDepthBuffer(shared_ptr<RenderBuffer> const & rt)
    {
      CHECK(!isRendering(), ("should call this function only outside beginFrame/endFrame"));
      m_depthBuffer = rt;
    }

    void Renderer::resetDepthBuffer()
    {
      CHECK(!isRendering(), ("should call this function only outside beginFrame/endFrame"));
      m_depthBuffer.reset();
    }

    Renderer::ClearCommand::ClearCommand(graphics::Color const & color,
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

    void Renderer::clear(graphics::Color const & c, bool clearRT, float depth, bool clearDepth)
    {
      shared_ptr<ClearCommand> command(new ClearCommand(c, clearRT, depth, clearDepth));
      processCommand(command);
    }

    void Renderer::FinishCommand::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing FinishCommand command"));
      OGLCHECK(glFinish());
    }

    Renderer::CopyFramebufferToImage::CopyFramebufferToImage(shared_ptr<BaseTexture> target)
      : m_target(target) {}

    void Renderer::CopyFramebufferToImage::perform()
    {
      m_target->makeCurrent();
      OGLCHECK(glCopyTexImage2D(GL_TEXTURE_2D, 0, DATA_TRAITS::gl_pixel_format_type,
                                0, 0, m_target->width(), m_target->height(), 0));
    }

    Renderer::ReadPixels::ReadPixels(m2::RectU const & r, void * data)
      : m_rect(r), m_data(data)
    {}

    void Renderer::ReadPixels::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing ReadPixels command"));

      OGLCHECK(glReadPixels(m_rect.minX(),
                            m_rect.minY(),
                            m_rect.SizeX(),
                            m_rect.SizeY(),
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            m_data
                            ));
    }

    void Renderer::readPixels(m2::RectU const & r, void * data, bool doForce)
    {
      processCommand(make_shared_ptr(new ReadPixels(r, data)), Packet::ECommand, doForce);
    }

    void Renderer::finish(bool doForce)
    {
      processCommand(make_shared_ptr(new FinishCommand()), Packet::ECommand, doForce);
    }

    Renderer::ChangeFrameBuffer::ChangeFrameBuffer(shared_ptr<FrameBuffer> const & fb)
      : m_frameBuffer(fb)
    {}

    void Renderer::ChangeFrameBuffer::perform()
    {
      if (isDebugging())
      {
        LOG(LINFO, ("performing ChangeFrameBuffer command"));
        LOG(LINFO, ("frameBufferID=", m_frameBuffer->id()));
      }

      m_frameBuffer->makeCurrent();

      OGLCHECK(glViewport(0, 0, m_frameBuffer->width(), m_frameBuffer->height()));
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

    void Renderer::processCommand(shared_ptr<Command> const & command, Packet::EType type, bool doForce)
    {
      if (command)
        command->setIsDebugging(renderQueue() && !doForce);

      if (renderQueue() && !doForce)
        renderQueue()->processPacket(Packet(command, type));
      else
        if (command)
        {
          command->setRenderContext(m_renderContext.get());
          command->perform();
        }
    }

    PacketsQueue * Renderer::renderQueue()
    {
      return m_renderQueue;
    }

    void Renderer::addFramePoint()
    {
      if (m_renderQueue)
        m_renderQueue->processPacket(Packet(Packet::EFramePoint));
    }

    void Renderer::addCheckPoint()
    {
      if (m_renderQueue)
        m_renderQueue->processPacket(Packet(Packet::ECheckPoint));
    }

    void Renderer::completeCommands()
    {
      if (m_renderQueue)
        m_renderQueue->completeCommands();
    }

    void Renderer::setEnvironment(core::CommandsQueue::Environment const * env)
    {
      m_env = env;
    }

    bool Renderer::isCancelled() const
    {
      if (m_env)
        return m_env->isCancelled();
      else
        return false;
    }

    RenderContext * Renderer::renderContext() const
    {
      return m_renderContext.get();
    }

    int Renderer::threadSlot() const
    {
      return m_threadSlot;
    }
  }
}
