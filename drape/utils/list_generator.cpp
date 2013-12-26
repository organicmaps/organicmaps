#include "list_generator.hpp"

#include "../../base/math.hpp"

#include "../../std/algorithm.hpp"
#include "../../std/vector.hpp"

struct Vertex
{
  Vertex() {}
  Vertex(float x , float y)
    : m_x(x)
    , m_y(y)
  {
  }

  float m_x;
  float m_y;
};

ListGenerator::ListGenerator()
  : m_depth(0.0f)
  , m_x(0.0f)
  , m_y(0.0f)
  , m_width(0.0f)
  , m_height(0.0f)
{
}

void ListGenerator::SetDepth(float depth)
{
  m_depth = depth;
}

void ListGenerator::SetViewport(float x, float y, float width, float height)
{
  m_x = x;
  m_y = y;
  m_width = width;
  m_height = height;
}

void ListGenerator::SetProgram(uint32_t program)
{
  m_programIndex = program;
}

void ListGenerator::SetUniforms(const UniformValuesStorage & uniforms)
{
  m_uniforms = uniforms;
}

void ListGenerator::Generate(int count, Batcher & batcher)
{
  float quadCount = count /2.0;
  int dimensionCount = ceil(sqrt(quadCount));
  float dx = m_width / (float)dimensionCount;
  float dy = m_height / (float)dimensionCount;
  int vertexCount = count * 3;

  vector<Vertex> vertexes;
  vertexes.reserve(vertexCount);

  int counter = 0;
  for (int x = 0; x < dimensionCount; ++x)
  {
    for (int y = 0; y < dimensionCount; ++y)
    {
      if (counter >= count)
        break;

      vertexes.push_back(Vertex(m_x + dx * (float)x,      m_y + dy * (float)y));
      vertexes.push_back(Vertex(m_x + dx * (float)x,      m_y + dy * (float)y + dy));
      vertexes.push_back(Vertex(m_x + dx * (float)x + dx, m_y + dy * (float)y));

      vertexes.push_back(Vertex(m_x + dx * (float)x + dx, m_y + dy * (float)y));
      vertexes.push_back(Vertex(m_x + dx * (float)x,      m_y + dy * (float)y + dy));
      vertexes.push_back(Vertex(m_x + dx * (float)x + dx, m_y + dy * (float)y + dy));

      counter += 2;
    }
  }

  AttributeProvider provider(2, vertexCount);
  {
    BindingInfo info(1);
    BindingDecl & decl = info.GetBindingDecl(0);
    decl.m_attributeName = "position";
    decl.m_componentCount = 2;
    decl.m_componentType = GLConst::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, info, MakeStackRefPointer(&vertexes[0]));
  }

  vector<float> depthMemory(vertexCount, m_depth);
  {
    BindingInfo info(1);
    BindingDecl & decl = info.GetBindingDecl(0);
    decl.m_attributeName = "depth";
    decl.m_componentCount = 1;
    decl.m_componentType = GLConst::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(1, info, MakeStackRefPointer(&depthMemory[0]));
  }

  TextureBinding textureBinding("", false, 0, RefPointer<Texture>());
  GLState state(m_programIndex, (int16_t)m_depth, textureBinding);
  state.GetUniformValues() = m_uniforms;
  batcher.InsertTriangleList(state, MakeStackRefPointer(&provider));
}
