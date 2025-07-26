#include "drape/vertex_array_buffer.hpp"

#include "drape/drape_global.hpp"
#include "drape/gl_functions.hpp"
#include "drape/gl_gpu_program.hpp"
#include "drape/index_storage.hpp"
#include "drape/support_manager.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/target_os.hpp"

#include <algorithm>

namespace dp
{
namespace
{
std::pair<uint32_t, uint32_t> CalculateMappingPart(std::vector<dp::MutateNode> const & nodes)
{
  uint32_t minOffset = std::numeric_limits<uint32_t>::max();
  uint32_t maxOffset = std::numeric_limits<uint32_t>::min();
  for (size_t i = 0; i < nodes.size(); ++i)
  {
    MutateNode const & node = nodes[i];
    ASSERT_GREATER(node.m_region.m_count, 0, ());
    if (node.m_region.m_offset < minOffset)
      minOffset = node.m_region.m_offset;
    uint32_t const endOffset = node.m_region.m_offset + node.m_region.m_count;
    if (endOffset > maxOffset)
      maxOffset = endOffset;
  }
  ASSERT_LESS(minOffset, maxOffset, ());
  return std::make_pair(minOffset, maxOffset - minOffset);
}

class GLVertexArrayBufferImpl : public VertexArrayBufferImpl
{
public:
  ~GLVertexArrayBufferImpl() override
  {
    if (m_VAO != 0)
    {
      // Build is called only when VertexArrayBuffer is full and transferred to FrontendRenderer.
      // If user move screen before all geometry read from MWM we delete VertexArrayBuffer on
      // BackendRenderer. In this case m_VAO will be equal 0 also m_VAO == 0 is on devices
      // that do not support OES_vertex_array_object extension.
      GLFunctions::glDeleteVertexArray(m_VAO);
    }
  }

  bool Build(ref_ptr<GpuProgram> program) override
  {
    if (m_VAO != 0 && m_program.get() == program.get())
      return false;

    m_program = program;

    if (m_VAO != 0)
      GLFunctions::glDeleteVertexArray(m_VAO);
    m_VAO = GLFunctions::glGenVertexArray();
    return true;
  }

  bool Bind() override
  {
    ASSERT(m_VAO != 0, ("You need to call Build method before bind it and render."));
    GLFunctions::glBindVertexArray(m_VAO);
    return true;
  }

  void Unbind() override { GLFunctions::glBindVertexArray(0); }

  void BindBuffers(BuffersMap const & buffers) const override
  {
    for (auto it = buffers.begin(); it != buffers.end(); ++it)
    {
      BindingInfo const & binding = it->first;
      ref_ptr<DataBuffer> buffer = make_ref(it->second);
      buffer->GetBuffer()->Bind();

      for (uint16_t i = 0; i < binding.GetCount(); ++i)
      {
        BindingDecl const & decl = binding.GetBindingDecl(i);
        int8_t attributeLocation = m_program->GetAttributeLocation(decl.m_attributeName);
        assert(attributeLocation != -1);
        GLFunctions::glEnableVertexAttribute(attributeLocation);
        GLFunctions::glVertexAttributePointer(attributeLocation, decl.m_componentCount, decl.m_componentType, false,
                                              decl.m_stride, decl.m_offset);
      }
    }
  }

  void RenderRange(ref_ptr<GraphicsContext> context, bool drawAsLine, IndicesRange const & range) override
  {
    UNUSED_VALUE(context);
    ASSERT(m_program != nullptr, ("Build must be called before RenderRange"));
    GLFunctions::glDrawElements(drawAsLine ? gl_const::GLLines : gl_const::GLTriangles, dp::IndexStorage::SizeOfIndex(),
                                range.m_idxCount, range.m_idxStart);
  }

private:
  int m_VAO = 0;
  ref_ptr<GLGpuProgram> m_program;
};
}  // namespace

VertexArrayBuffer::VertexArrayBuffer(uint32_t indexBufferSize, uint32_t dataBufferSize, uint64_t batcherHash)
  : m_dataBufferSize(dataBufferSize)
  , m_batcherHash(batcherHash)
{
  m_indexBuffer = make_unique_dp<IndexBuffer>(indexBufferSize);
}

VertexArrayBuffer::~VertexArrayBuffer()
{
  m_indexBuffer.reset();
  m_staticBuffers.clear();
  m_dynamicBuffers.clear();
  m_impl.reset();
}

void VertexArrayBuffer::Preflush(ref_ptr<GraphicsContext> context)
{
  PreflushImpl(context);
}

void VertexArrayBuffer::PreflushImpl(ref_ptr<GraphicsContext> context)
{
  ASSERT(!m_isPreflushed, ());

  // Buffers are ready, so moving them from CPU to GPU.
  for (auto & buffer : m_staticBuffers)
    buffer.second->MoveToGPU(context, GPUBuffer::ElementBuffer, m_batcherHash);

  for (auto & buffer : m_dynamicBuffers)
    buffer.second->MoveToGPU(context, GPUBuffer::ElementBuffer, m_batcherHash);

  ASSERT(m_indexBuffer != nullptr, ());
  m_indexBuffer->MoveToGPU(context, GPUBuffer::IndexBuffer, m_batcherHash);

  // Preflush can be called on BR, where impl is not initialized.
  // For Metal rendering this code has no meaning.
  auto const apiVersion = context->GetApiVersion();
  if (apiVersion == dp::ApiVersion::OpenGLES3)
  {
    GLFunctions::glBindBuffer(0, gl_const::GLElementArrayBuffer);
    GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
  }

  m_isPreflushed = true;
}

void VertexArrayBuffer::Render(ref_ptr<GraphicsContext> context, bool drawAsLine)
{
  RenderRange(context, drawAsLine, IndicesRange(0, GetIndexBuffer()->GetCurrentSize()));
}

void VertexArrayBuffer::RenderRange(ref_ptr<GraphicsContext> context, bool drawAsLine, IndicesRange const & range)
{
  if (HasBuffers() && GetIndexCount() > 0)
  {
    // If OES_vertex_array_object is supported than all bindings have already saved in VAO
    // and we need only bind VAO.
    if (!Bind())
      BindStaticBuffers();

    BindDynamicBuffers();
    GetIndexBuffer()->Bind();

    CHECK(m_impl != nullptr, ());
    m_impl->RenderRange(context, drawAsLine, range);

    Unbind();
  }
}

void VertexArrayBuffer::Build(ref_ptr<GraphicsContext> context, ref_ptr<GpuProgram> program)
{
  if (!m_isPreflushed)
    PreflushImpl(context);

  if (!HasBuffers())
    return;

  if (!m_impl)
  {
    auto const apiVersion = context->GetApiVersion();
    if (apiVersion == dp::ApiVersion::OpenGLES3)
    {
      m_impl = make_unique_dp<GLVertexArrayBufferImpl>();
    }
    else if (apiVersion == dp::ApiVersion::Metal)
    {
#if defined(OMIM_METAL_AVAILABLE)
      m_impl = CreateImplForMetal(make_ref(this));
#endif
    }
    else if (apiVersion == dp::ApiVersion::Vulkan)
    {
      CHECK_NOT_EQUAL(m_bindingInfoCount, 0, ());
      m_impl = CreateImplForVulkan(context, make_ref(this), std::move(m_bindingInfo), m_bindingInfoCount);
    }
    else
    {
      CHECK(false, ("Unsupported API version."));
    }
  }

  CHECK(m_impl != nullptr, ());
  if (!m_impl->Build(program))
    return;

  Bind();
  BindStaticBuffers();
  Unbind();
}

void VertexArrayBuffer::UploadData(ref_ptr<GraphicsContext> context, BindingInfo const & bindingInfo, void const * data,
                                   uint32_t count)
{
  ref_ptr<DataBuffer> buffer;
  if (!bindingInfo.IsDynamic())
    buffer = GetOrCreateStaticBuffer(bindingInfo);
  else
    buffer = GetOrCreateDynamicBuffer(bindingInfo);

  // For Vulkan rendering we have to know the whole collection of binding info.
  if (context->GetApiVersion() == dp::ApiVersion::Vulkan && !m_impl)
    CollectBindingInfo(bindingInfo);

  if (count > 0)
    m_isChanged = true;
  buffer->GetBuffer()->UploadData(context, data, count);
}

ref_ptr<DataBuffer> VertexArrayBuffer::GetOrCreateDynamicBuffer(BindingInfo const & bindingInfo)
{
  return GetOrCreateBuffer(bindingInfo, true);
}

ref_ptr<DataBuffer> VertexArrayBuffer::GetDynamicBuffer(BindingInfo const & bindingInfo) const
{
  return GetBuffer(bindingInfo, true);
}

ref_ptr<DataBuffer> VertexArrayBuffer::GetOrCreateStaticBuffer(BindingInfo const & bindingInfo)
{
  return GetOrCreateBuffer(bindingInfo, false);
}

ref_ptr<DataBuffer> VertexArrayBuffer::GetBuffer(BindingInfo const & bindingInfo, bool isDynamic) const
{
  BuffersMap const * buffers = nullptr;
  if (isDynamic)
    buffers = &m_dynamicBuffers;
  else
    buffers = &m_staticBuffers;

  auto it = buffers->find(bindingInfo);
  if (it == buffers->end())
    return nullptr;

  return make_ref(it->second);
}

ref_ptr<DataBuffer> VertexArrayBuffer::GetOrCreateBuffer(BindingInfo const & bindingInfo, bool isDynamic)
{
  BuffersMap * buffers = nullptr;
  if (isDynamic)
    buffers = &m_dynamicBuffers;
  else
    buffers = &m_staticBuffers;

  auto it = buffers->find(bindingInfo);
  if (it == buffers->end())
  {
    drape_ptr<DataBuffer> dataBuffer = make_unique_dp<DataBuffer>(bindingInfo.GetElementSize(), m_dataBufferSize);
    ref_ptr<DataBuffer> result = make_ref(dataBuffer);
    (*buffers).insert(std::make_pair(bindingInfo, std::move(dataBuffer)));
    return result;
  }

  return make_ref(it->second);
}

uint32_t VertexArrayBuffer::GetAvailableIndexCount() const
{
  return GetIndexBuffer()->GetAvailableSize();
}

uint32_t VertexArrayBuffer::GetAvailableVertexCount() const
{
  if (m_staticBuffers.empty())
    return m_dataBufferSize;

#ifdef DEBUG
  auto it = m_staticBuffers.begin();
  uint32_t const prev = it->second->GetBuffer()->GetAvailableSize();
  for (; it != m_staticBuffers.end(); ++it)
    ASSERT_EQUAL(prev, it->second->GetBuffer()->GetAvailableSize(), ());
#endif

  return m_staticBuffers.begin()->second->GetBuffer()->GetAvailableSize();
}

uint32_t VertexArrayBuffer::GetStartIndexValue() const
{
  if (m_staticBuffers.empty())
    return 0;

#ifdef DEBUG
  auto it = m_staticBuffers.begin();
  uint32_t const prev = it->second->GetBuffer()->GetCurrentSize();
  for (; it != m_staticBuffers.end(); ++it)
    ASSERT(prev == it->second->GetBuffer()->GetCurrentSize(), ());
#endif

  return m_staticBuffers.begin()->second->GetBuffer()->GetCurrentSize();
}

uint32_t VertexArrayBuffer::GetDynamicBufferOffset(BindingInfo const & bindingInfo)
{
  return GetOrCreateDynamicBuffer(bindingInfo)->GetBuffer()->GetCurrentSize();
}

uint32_t VertexArrayBuffer::GetIndexCount() const
{
  return GetIndexBuffer()->GetCurrentSize();
}

void VertexArrayBuffer::UploadIndices(ref_ptr<GraphicsContext> context, void const * data, uint32_t count)
{
  CHECK_LESS_OR_EQUAL(count, GetIndexBuffer()->GetAvailableSize(), ());
  GetIndexBuffer()->UploadData(context, data, count);
}

void VertexArrayBuffer::ApplyMutation(ref_ptr<GraphicsContext> context, ref_ptr<IndexBufferMutator> indexMutator,
                                      ref_ptr<AttributeBufferMutator> attrMutator)
{
  // We need to bind current VAO before calling glBindBuffer if OES_vertex_array_object is
  // supported. Otherwise we risk affecting a previously bound VAO.
  Bind();

  if (indexMutator != nullptr)
  {
    ASSERT(m_indexBuffer != nullptr, ());
    if (indexMutator->GetCapacity() > m_indexBuffer->GetBuffer()->GetCapacity())
    {
      m_indexBuffer = make_unique_dp<IndexBuffer>(indexMutator->GetCapacity());
      m_indexBuffer->MoveToGPU(context, GPUBuffer::IndexBuffer, m_batcherHash);
    }
    m_indexBuffer->UpdateData(context, indexMutator->GetIndexes(), indexMutator->GetIndexCount());
  }

  if (attrMutator == nullptr)
  {
    Unbind();
    return;
  }

  auto const & data = attrMutator->GetMutateData();
  for (auto it = data.begin(); it != data.end(); ++it)
  {
    auto const & nodes = it->second;
    if (nodes.empty())
      continue;

    auto const offsets = CalculateMappingPart(nodes);

    ref_ptr<DataBuffer> buffer = GetDynamicBuffer(it->first);
    ASSERT(buffer != nullptr, ());
    DataBufferMapper mapper(context, buffer, offsets.first, offsets.second);
    for (size_t i = 0; i < nodes.size(); ++i)
    {
      MutateNode const & node = nodes[i];
      ASSERT_GREATER(node.m_region.m_count, 0, ());
      mapper.UpdateData(node.m_data.get(), node.m_region.m_offset - offsets.first, node.m_region.m_count);
    }
  }

  Unbind();
}

bool VertexArrayBuffer::Bind() const
{
  if (m_impl == nullptr)
    return false;

  return m_impl->Bind();
}

void VertexArrayBuffer::Unbind() const
{
  if (m_impl == nullptr)
    return;

  m_impl->Unbind();
}

void VertexArrayBuffer::BindStaticBuffers() const
{
  BindBuffers(m_staticBuffers);
}

void VertexArrayBuffer::BindDynamicBuffers() const
{
  BindBuffers(m_dynamicBuffers);
}

void VertexArrayBuffer::BindBuffers(BuffersMap const & buffers) const
{
  CHECK(m_impl != nullptr, ());
  m_impl->BindBuffers(buffers);
}

ref_ptr<DataBufferBase> VertexArrayBuffer::GetIndexBuffer() const
{
  CHECK(m_indexBuffer != nullptr, ());
  return m_indexBuffer->GetBuffer();
}

void VertexArrayBuffer::CollectBindingInfo(dp::BindingInfo const & bindingInfo)
{
  for (size_t i = 0; i < m_bindingInfoCount; ++i)
  {
    if (m_bindingInfo[i].GetID() == bindingInfo.GetID())
    {
      CHECK(m_bindingInfo[i] == bindingInfo, ("Incorrect binding info."));
      return;
    }
  }

  CHECK_LESS(m_bindingInfoCount, kMaxBindingInfo, ());
  m_bindingInfo[m_bindingInfoCount++] = bindingInfo;
  std::sort(m_bindingInfo.begin(), m_bindingInfo.begin() + m_bindingInfoCount,
            [](dp::BindingInfo const & info1, dp::BindingInfo const & info2) { return info1.GetID() < info2.GetID(); });
}
}  // namespace dp
