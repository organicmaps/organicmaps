#pragma once

#include "drape/render_state.hpp"
#include "drape/pointers.hpp"

#include <functional>

namespace dp
{
class GpuProgram;

class MeshObject
{
public:
  enum class DrawPrimitive: uint32_t
  {
    Triangles,
    TriangleStrip,
    LineStrip
  };

  using PreRenderFn = std::function<void()>;
  using PostRenderFn = std::function<void()>;

  MeshObject(DrawPrimitive drawPrimitive);
  virtual ~MeshObject();

  void SetBuffer(uint32_t bufferInd, std::vector<float> && vertices, uint32_t stride);
  void SetAttribute(std::string const & attributeName, uint32_t bufferInd, uint32_t offset, uint32_t componentsCount);

  void UpdateBuffer(uint32_t bufferInd, std::vector<float> && vertices);

  void Render(ref_ptr<dp::GpuProgram> program,
              PreRenderFn const & preRenderFn, PostRenderFn const & postRenderFn);

  uint32_t GetNextBufferIndex() const { return static_cast<uint32_t>(m_buffers.size()); }

  bool IsInitialized() const { return m_initialized; }
  void Build();
  void Reset();

private:
  struct AttributeMapping
  {
    AttributeMapping() = default;

    AttributeMapping(std::string const & attributeName, uint32_t offset, uint32_t componentsCount)
      : m_offset(offset)
      , m_componentsCount(componentsCount)
      , m_attributeName(attributeName)
    {}

    uint32_t m_offset = 0;
    uint32_t m_componentsCount = 0;
    std::string m_attributeName;
  };

  struct VertexBuffer
  {
    VertexBuffer() = default;

    VertexBuffer(std::vector<float> && data, uint32_t stride)
      : m_data(std::move(data))
      , m_stride(stride)
    {}

    std::vector<float> m_data;
    uint32_t m_stride = 0;
    uint32_t m_bufferId = 0;

    std::vector<AttributeMapping> m_attributes;
  };

  std::vector<VertexBuffer> m_buffers;
  DrawPrimitive m_drawPrimitive = DrawPrimitive::Triangles;

  uint32_t m_VAO = 0;
  bool m_initialized = false;
};
}  // namespace dp
