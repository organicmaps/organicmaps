#include "arrow3d.hpp"

#include "drape/glconstants.hpp"
#include "drape/glfunctions.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/glstate.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/shader_def.hpp"
#include "drape/texture_manager.hpp"
#include "drape/uniform_values_storage.hpp"

#include "indexer/scales.hpp"

#include "geometry/screenbase.hpp"

namespace df
{

double const kArrowSizeX = 2.0;
double const kArrowSizeY = 3.0;
double const kArrow3dScaleMin = 1.0;
double const kArrow3dScaleMax = 2.2;
double const kArrow3dMinZoom = 16;

Arrow3d::Arrow3d()
  : m_state(gpu::ARROW_3D_PROGRAM, dp::GLState::OverlayLayer)
{
  m_vertices = {
     0.0f,  0.0f,  -1.0f,
    -1.0f, -1.0f,  0.0f,
     0.0f,  2.0f,  0.0f,

     0.0f,  0.0f,  -1.0f,
     0.0f,  2.0f,  0.0f,
     1.0f, -1.0f,  0.0f,

     0.0f,  0.0f,  -1.0f,
     0.0f, -0.5f,  0.0f,
    -1.0f, -1.0f,  0.0f,

     0.0f,  0.0f,  -1.0f,
     1.0f, -1.0f,  0.0f,
     0.0f, -0.5f,  0.0f
  };

  m_normals.resize(m_vertices.size());
  for (size_t triangle = 0; triangle < m_vertices.size() / 9; ++triangle)
  {
    glsl::vec3 v[3];
    for (size_t vertex = 0; vertex < 3; ++vertex)
    {
      size_t const offset = triangle * 9 + vertex * 3;
      v[vertex] = glsl::vec3(m_vertices[offset], m_vertices[offset + 1], m_vertices[offset + 2]);
    }

    glsl::vec3 normal = glsl::cross(glsl::vec3(v[1].x - v[0].x, v[1].y - v[0].y, v[1].z - v[0].z),
                                    glsl::vec3(v[2].x - v[0].x, v[2].y - v[0].y, v[2].z - v[0].z));
    normal = glsl::normalize(normal);

    for (size_t vertex = 0; vertex < 3; ++vertex)
    {
      size_t const offset = triangle * 9 + vertex * 3;
      m_normals[offset] = normal.x;
      m_normals[offset + 1] = normal.y;
      m_normals[offset + 2] = normal.z;
    }
  }
}

Arrow3d::~Arrow3d()
{
  if (m_bufferId != 0)
    GLFunctions::glDeleteBuffer(m_bufferId);

  if (m_bufferNormalsId != 0)
    GLFunctions::glDeleteBuffer(m_bufferNormalsId);
}

void Arrow3d::SetPosition(const m2::PointD & position)
{
  m_position = position;
}

void Arrow3d::SetAzimuth(double azimuth)
{
  m_azimuth = azimuth;
}

void Arrow3d::SetSize(uint32_t width, uint32_t height)
{
  m_pixelWidth = width;
  m_pixelHeight = height;
}

void Arrow3d::SetTexture(ref_ptr<dp::TextureManager> texMng)
{
  m_state.SetColorTexture(texMng->GetSymbolsTexture());
}

void Arrow3d::Build(ref_ptr<dp::GpuProgram> prg)
{
  m_bufferId = GLFunctions::glGenBuffer();
  GLFunctions::glBindBuffer(m_bufferId, gl_const::GLArrayBuffer);

  m_attributePosition = prg->GetAttributeLocation("a_pos");
  ASSERT_NOT_EQUAL(m_attributePosition, -1, ());

  GLFunctions::glBufferData(gl_const::GLArrayBuffer, m_vertices.size() * sizeof(m_vertices[0]),
                            m_vertices.data(), gl_const::GLStaticDraw);

  m_bufferNormalsId = GLFunctions::glGenBuffer();
  GLFunctions::glBindBuffer(m_bufferNormalsId, gl_const::GLArrayBuffer);

  m_attributeNormal = prg->GetAttributeLocation("a_normal");
  ASSERT_NOT_EQUAL(m_attributeNormal, -1, ());

  GLFunctions::glBufferData(gl_const::GLArrayBuffer, m_normals.size() * sizeof(m_normals[0]),
                            m_normals.data(), gl_const::GLStaticDraw);

  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

void Arrow3d::Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng)
{
  // Unbind current VAO, because glVertexAttributePointer and glEnableVertexAttribute can affect it.
  GLFunctions::glBindVertexArray(0);

  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(gpu::ARROW_3D_PROGRAM);
  prg->Bind();

  if (!m_isInitialized)
  {
    Build(prg);
    m_isInitialized = true;
  }

  dp::ApplyState(m_state, prg);

  static double const kLog2 = log(2.0);
  double const kMaxZoom = scales::UPPER_STYLE_SCALE + 1.0;
  double const zoomLevel = my::clamp(fabs(log(screen.GetScale()) / kLog2), kArrow3dMinZoom, kMaxZoom);
  double const t = (zoomLevel - kArrow3dMinZoom) / (kMaxZoom - kArrow3dMinZoom);
  double const arrowScale = kArrow3dScaleMin * (1.0 - t) + kArrow3dScaleMax * t;

  double const scaleX = m_pixelWidth * arrowScale * 2.0 / screen.PixelRect().SizeX() / kArrowSizeX;
  double const scaleY = m_pixelHeight * arrowScale * 2.0 / screen.PixelRect().SizeY() / kArrowSizeY;
  double const scaleZ = scaleX;

  m2::PointD const pos = screen.GtoP(m_position);
  double const dX = 2.0 * pos.x / screen.PixelRect().SizeX() - 1.0;
  double const dY = 2.0 * pos.y / screen.PixelRect().SizeY() - 1.0;

  math::Matrix<float, 4, 4> scaleM = math::Identity<float, 4>();
  scaleM(0, 0) = scaleX;
  scaleM(1, 1) = scaleY;
  scaleM(2, 2) = scaleZ;

  math::Matrix<float, 4, 4> rotateM = math::Identity<float, 4>();
  rotateM(0, 0) = cos(m_azimuth + screen.GetAngle());
  rotateM(0, 1) = -sin(m_azimuth + screen.GetAngle());
  rotateM(1, 0) = -rotateM(0, 1);
  rotateM(1, 1) = rotateM(0, 0);

  math::Matrix<float, 4, 4> translateM = math::Identity<float, 4>();
  translateM(3, 0) = dX;
  translateM(3, 1) = -dY;

  math::Matrix<float, 4, 4> modelTransform = rotateM * scaleM * translateM;
  modelTransform = modelTransform * math::Matrix<float, 4, 4>(screen.Pto3dMatrix());

  dp::UniformValuesStorage uniforms;
  uniforms.SetMatrix4x4Value("m_transform", modelTransform.m_data);

  dp::ApplyUniforms(uniforms, prg);

  GLFunctions::glBindBuffer(m_bufferId, gl_const::GLArrayBuffer);
  GLFunctions::glEnableVertexAttribute(m_attributePosition);
  GLFunctions::glVertexAttributePointer(m_attributePosition, 3, gl_const::GLFloatType, false, 0, 0);

  GLFunctions::glBindBuffer(m_bufferNormalsId, gl_const::GLArrayBuffer);
  GLFunctions::glEnableVertexAttribute(m_attributeNormal);
  GLFunctions::glVertexAttributePointer(m_attributeNormal, 3, gl_const::GLFloatType, false, 0, 0);

  GLFunctions::glDrawArrays(gl_const::GLTriangles, 0, m_vertices.size() / 3);

  prg->Unbind();
  GLFunctions::glBindBuffer(0, gl_const::GLArrayBuffer);
}

}  // namespace df
