#include "drape/instancing.hpp"

#include "drape/drape_global.hpp"
#include "drape/gl_functions.hpp"

#include "base/assert.hpp"

namespace dp
{
#if defined(OMIM_METAL_AVAILABLE)
void DrawInstancedTriangleStripMetal(ref_ptr<dp::GraphicsContext> context, uint32_t instanceCount,
                                     uint32_t verticesCount);
#endif

std::unique_ptr<InstancingImpl> CreateVulkanInstancingImpl(ref_ptr<dp::GraphicsContext> context);

class OpenGLInstancingImpl : public InstancingImpl
{
public:
  OpenGLInstancingImpl() { m_vao = GLFunctions::glGenVertexArray(); }

  ~OpenGLInstancingImpl() override { GLFunctions::glDeleteVertexArray(m_vao); }

  void DrawInstancedTriangleStrip(ref_ptr<dp::GraphicsContext> context, uint32_t instanceCount,
                                  uint32_t verticesCount) override
  {
    UNUSED_VALUE(context);
    GLFunctions::glBindVertexArray(m_vao);
    GLFunctions::glDrawArraysInstanced(gl_const::GLTriangleStrip, 0, verticesCount, instanceCount);
    GLFunctions::glBindVertexArray(0);
  }

private:
  uint32_t m_vao = 0;
};

void Instancing::DrawInstancedTriangleStrip(ref_ptr<dp::GraphicsContext> context, uint32_t instanceCount,
                                            uint32_t verticesCount)
{
  CHECK(context != nullptr, ());
  CHECK(instanceCount > 0, ());
  CHECK(verticesCount > 0, ());

  dp::ApiVersion apiVersion = context->GetApiVersion();
  switch (apiVersion)
  {
#if defined(OMIM_METAL_AVAILABLE)
  case dp::ApiVersion::Metal: DrawInstancedTriangleStripMetal(context, instanceCount, verticesCount); break;
#endif

  case dp::ApiVersion::Vulkan:
  {
    if (m_impl == nullptr)
      m_impl = CreateVulkanInstancingImpl(context);
    m_impl->DrawInstancedTriangleStrip(context, instanceCount, verticesCount);
    break;
  }

  case dp::ApiVersion::OpenGLES3:
  {
    if (m_impl == nullptr)
      m_impl = std::make_unique<OpenGLInstancingImpl>();
    m_impl->DrawInstancedTriangleStrip(context, instanceCount, verticesCount);
    break;
  }

  default: CHECK(false, ("Unsupported API version for instanced rendering"));
  }
}
}  // namespace dp
