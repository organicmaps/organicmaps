#include "drape/mesh_object.hpp"

#include "drape/glconstants.hpp"
#include "drape/glextensions_list.hpp"
#include "drape/glfunctions.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/gpu_program.hpp"
#include "drape/texture_manager.hpp"

namespace
{
glConst GetGLDrawPrimitive(dp::MeshObject::DrawPrimitive drawPrimitive)
{
  switch (drawPrimitive)
  {
  case dp::MeshObject::DrawPrimitive::Triangles: return gl_const::GLTriangles;
  case dp::MeshObject::DrawPrimitive::TriangleStrip: return gl_const::GLTriangleStrip;
  case dp::MeshObject::DrawPrimitive::LineStrip: return gl_const::GLLineStrip;
  }
}
}  // namespace

namespace dp
{
MeshObject::MeshObject(DrawPrimitive drawPrimitive)
  : m_drawPrimitive(drawPrimitive)
{}

MeshObject::~MeshObject()
{
  Reset();
}

void MeshObject::SetBuffer(uint32_t bufferInd, std::vector<float> && vertices, uint32_t stride)
{
  CHECK_LESS_OR_EQUAL(bufferInd, GetNextBufferIndex(), ());

  if (bufferInd == GetNextBufferIndex())
    m_buffers.emplace_back(std::move(vertices), stride);
  else
    m_buffers[bufferInd] = VertexBuffer(std::move(vertices), stride);

  Reset();
}

void MeshObject::SetAttribute(std::string const & attributeName, uint32_t bufferInd, uint32_t offset,
                              uint32_t componentsCount)
{
  CHECK_LESS(bufferInd, m_buffers.size(), ());
  m_buffers[bufferInd].m_attributes.emplace_back(attributeName, offset, componentsCount);

  Reset();
}

void MeshObject::Reset()
{
  for (auto & buffer : m_buffers)
  {
    if (buffer.m_bufferId != 0)
    {
      GLFunctions::glDeleteBuffer(buffer.m_bufferId);
      buffer.m_bufferId = 0;
    }
  }

  if (m_VAO != 0)
    GLFunctions::glDeleteVertexArray(m_VAO);

  m_VAO = 0;
  m_initialized = false;
}

void MeshObject::UpdateBuffer(uint32_t bufferInd, std::vector<float> && vertices)
{
  CHECK(m_initialized, ());
  CHECK_LESS(bufferInd, static_cast<uint32_t>(m_buffers.size()), ());
  CHECK(m_buffers[bufferInd].m_bufferId != 0, ());
  CHECK(!vertices.empty(), ());

  auto & buffer = m_buffers[bufferInd];
  buffer.m_data = std::move(vertices);
  GLFunctions::glBindBuffer(buffer.m_bufferId, gl_const::GLArrayBuffer);
  GLFunctions::glBufferData(gl_const::GLArrayBuffer,
                            static_cast<uint32_t>(buffer.m_data.size()) * sizeof(buffer.m_data[0]),
                            buffer.m_data.data(), gl_const::GLStaticDraw);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

void MeshObject::Build(ref_ptr<dp::GpuProgram> program)
{
  Reset();

  bool const isVAOSupported = dp::GLExtensionsList::Instance().IsSupported(dp::GLExtensionsList::VertexArrayObject);
  if (isVAOSupported)
  {
    m_VAO = GLFunctions::glGenVertexArray();
    GLFunctions::glBindVertexArray(m_VAO);
  }

  for (auto & buffer : m_buffers)
  {
    buffer.m_bufferId = GLFunctions::glGenBuffer();
    GLFunctions::glBindBuffer(buffer.m_bufferId, gl_const::GLArrayBuffer);

    if (!buffer.m_data.empty())
    {
      GLFunctions::glBufferData(gl_const::GLArrayBuffer,
                                static_cast<uint32_t>(buffer.m_data.size()) * sizeof(buffer.m_data[0]),
                                buffer.m_data.data(), gl_const::GLStaticDraw);
    }

    if (isVAOSupported)
    {
      for (auto const & attribute : buffer.m_attributes)
      {
        int8_t const attributePosition = program->GetAttributeLocation(attribute.m_attributeName);
        ASSERT_NOT_EQUAL(attributePosition, -1, ());
        GLFunctions::glEnableVertexAttribute(attributePosition);
        GLFunctions::glVertexAttributePointer(attributePosition, attribute.m_componentsCount,
                                              gl_const::GLFloatType, false,
                                              buffer.m_stride, attribute.m_offset);
      }
    }
  }

  if (isVAOSupported)
    GLFunctions::glBindVertexArray(0);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);

  m_initialized = true;
}

void MeshObject::Bind(ref_ptr<dp::GpuProgram> program)
{
  program->Bind();

  if (!m_initialized)
    Build(program);

  if (dp::GLExtensionsList::Instance().IsSupported(dp::GLExtensionsList::VertexArrayObject))
  {
    GLFunctions::glBindVertexArray(m_VAO);
  }
  else
  {
    for (auto const & buffer : m_buffers)
    {
      GLFunctions::glBindBuffer(buffer.m_bufferId, gl_const::GLArrayBuffer);
      for (auto const & attribute : buffer.m_attributes)
      {
        int8_t const attributePosition = program->GetAttributeLocation(attribute.m_attributeName);
        ASSERT_NOT_EQUAL(attributePosition, -1, ());
        GLFunctions::glEnableVertexAttribute(attributePosition);
        GLFunctions::glVertexAttributePointer(attributePosition, attribute.m_componentsCount,
                                              gl_const::GLFloatType, false,
                                              buffer.m_stride, attribute.m_offset);
      }
    }
  }
}

void MeshObject::DrawPrimitives()
{
  if (m_buffers.empty())
    return;

  auto const & buffer = m_buffers[0];
  auto const verticesCount = static_cast<uint32_t>(buffer.m_data.size() * sizeof(buffer.m_data[0]) / buffer.m_stride);

  GLFunctions::glDrawArrays(GetGLDrawPrimitive(m_drawPrimitive), 0, verticesCount);
}

void MeshObject::Unbind(ref_ptr<dp::GpuProgram> program)
{
  program->Unbind();

  if (dp::GLExtensionsList::Instance().IsSupported(dp::GLExtensionsList::VertexArrayObject))
    GLFunctions::glBindVertexArray(0);
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

// static
std::vector<float> MeshObject::GenerateNormalsForTriangles(std::vector<float> const & vertices,
                                                              size_t componentsCount)
{
  auto const trianglesCount = vertices.size() / (3 * componentsCount);
  std::vector<float> normals;
  normals.reserve(trianglesCount * 9);
  for (size_t triangle = 0; triangle < trianglesCount; ++triangle)
  {
    glsl::vec3 v[3];
    for (size_t vertex = 0; vertex < 3; ++vertex)
    {
      size_t const offset = triangle * componentsCount * 3 + vertex * componentsCount;
      v[vertex] = glsl::vec3(vertices[offset], vertices[offset + 1], vertices[offset + 2]);
    }

    glsl::vec3 normal = glsl::cross(glsl::vec3(v[1].x - v[0].x, v[1].y - v[0].y, v[1].z - v[0].z),
                                    glsl::vec3(v[2].x - v[0].x, v[2].y - v[0].y, v[2].z - v[0].z));
    normal = glsl::normalize(normal);

    for (size_t vertex = 0; vertex < 3; ++vertex)
    {
      normals.push_back(normal.x);
      normals.push_back(normal.y);
      normals.push_back(normal.z);
    }
  }
  return normals;
}
}  // namespace dp

