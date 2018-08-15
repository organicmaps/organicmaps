#include "drape_frontend/arrow3d.hpp"

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/program_manager.hpp"

#include "drape/texture_manager.hpp"

#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "geometry/screenbase.hpp"

namespace df
{
double const kArrowSize = 12.0;
double const kArrow3dScaleMin = 1.0;
double const kArrow3dScaleMax = 2.2;
double const kArrow3dMinZoom = 16;

float const kOutlineScale = 1.2f;

int constexpr kComponentsInVertex = 4;
int constexpr kComponentsInNormal = 3;

df::ColorConstant const kArrow3DShadowColor = "Arrow3DShadow";
df::ColorConstant const kArrow3DObsoleteColor = "Arrow3DObsolete";
df::ColorConstant const kArrow3DColor = "Arrow3D";
df::ColorConstant const kArrow3DOutlineColor = "Arrow3DOutline";

Arrow3d::Arrow3d()
  : TBase(DrawPrimitive::Triangles)
  , m_state(CreateRenderState(gpu::Program::Arrow3d, DepthLayer::OverlayLayer))
{
  m_state.SetDepthTestEnabled(false);
  std::vector<float> vertices = {
    0.0f, 0.0f, -1.0f, 1.0f,    -1.2f, -1.0f, 0.0f, 1.0f,   0.0f, 2.0f, 0.0f, 1.0f,
    0.0f, 0.0f, -1.0f, 1.0f,    0.0f,  2.0f, 0.0f, 1.0f,    1.2f, -1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, -1.0f, 1.0f,    0.0f, -0.5f, 0.0f, 1.0f,    -1.2f, -1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, -1.0f, 1.0f,    1.2f, -1.0f, 0.0f, 1.0f,    0.0f, -0.5f, 0.0f, 1.0f,
    
    0.0f, 2.27f, 0.0f, 0.0f,    1.4f, -1.17f, 0.0f, 0.0f,   0.0f, 2.0f, 0.0f, 1.0f,
    0.0f, 2.0f, 0.0f, 1.0f,     1.4f, -1.17f, 0.0f, 0.0f,   1.2f, -1.0f, 0.0f, 1.0f,
    0.0f, 2.27f, 0.0f, 0.0f,    0.0f, 2.0f, 0.0f, 1.0f,     -1.4f, -1.17f, 0.0f, 0.0f,
    0.0f, 2.0f, 0.0f, 1.0f,     -1.2f, -1.0f, 0.0f, 1.0f,   -1.4f, -1.17f, 0.0f, 0.0f,
    
    1.2f, -1.0f, 0.0f, 1.0f,    1.4f, -1.17f, 0.0f, 0.0f,   0.0f, -0.67f, 0.0f, 0.0f,
    0.0f, -0.5f, 0.0f, 1.0f,    1.2f, -1.0f, 0.0f, 1.0f,    0.0f, -0.67f, 0.0f, 0.0f,
    -1.2f, -1.0f, 0.0f, 1.0f,   0.0f, -0.67f, 0.0f, 0.0f,   -1.4f, -1.17f, 0.0f, 0.0f,
    0.0f, -0.5f, 0.0f, 1.0f,    0.0f, -0.67f, 0.0f, 0.0f,   -1.2f, -1.0f, 0.0f, 1.0f,
  };

  std::vector<float> normals;
  GenerateNormalsForTriangles(vertices, kComponentsInVertex, normals);
  normals.reserve(vertices.size());

  auto const verticesBufferInd = 0;
  SetBuffer(verticesBufferInd, std::move(vertices), sizeof(float) * kComponentsInVertex);
  SetAttribute("a_pos", verticesBufferInd, 0 /* offset */, kComponentsInVertex);

  auto const normalsBufferInd = 1;
  SetBuffer(normalsBufferInd, std::move(normals), sizeof(float) * kComponentsInNormal);
  SetAttribute("a_normal", normalsBufferInd, 0 /* offset */, kComponentsInNormal);
}

void Arrow3d::SetPosition(const m2::PointD & position)
{
  m_position = position;
}

void Arrow3d::SetAzimuth(double azimuth)
{
  m_azimuth = azimuth;
}

void Arrow3d::SetTexture(ref_ptr<dp::TextureManager> texMng)
{
  m_state.SetColorTexture(texMng->GetSymbolsTexture());
}

void Arrow3d::SetPositionObsolete(bool obsolete)
{
  m_obsoletePosition = obsolete;
}

void Arrow3d::Render(ScreenBase const & screen, ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                     bool routingMode)
{
  // Render shadow.
  if (screen.isPerspective())
  {
    RenderArrow(screen, context, mng, gpu::Program::Arrow3dShadow,
                df::GetColorConstant(df::kArrow3DShadowColor), 0.05f /* dz */,
                routingMode ? kOutlineScale : 1.0f /* scaleFactor */, false /* hasNormals */);
  }

  dp::Color const color =
      df::GetColorConstant(m_obsoletePosition ? df::kArrow3DObsoleteColor : df::kArrow3DColor);

  // Render outline.
  if (routingMode)
  {
    dp::Color const outlineColor = df::GetColorConstant(df::kArrow3DOutlineColor);
    RenderArrow(screen, context, mng, gpu::Program::Arrow3dOutline,
                dp::Color(outlineColor.GetRed(), outlineColor.GetGreen(), outlineColor.GetBlue(), color.GetAlpha()),
                0.0f /* dz */, kOutlineScale /* scaleFactor */, false /* hasNormals */);
  }

  // Render arrow.
  RenderArrow(screen, context, mng, gpu::Program::Arrow3d, color, 0.0f /* dz */, 1.0f /* scaleFactor */,
              true /* hasNormals */);
}

void Arrow3d::RenderArrow(ScreenBase const & screen, ref_ptr<dp::GraphicsContext> context,
                          ref_ptr<gpu::ProgramManager> mng, gpu::Program program, dp::Color const & color, float dz,
                          float scaleFactor, bool hasNormals)
{
  gpu::Arrow3dProgramParams params;
  math::Matrix<float, 4, 4> const modelTransform = CalculateTransform(screen, dz, scaleFactor);
  params.m_transform = glsl::make_mat4(modelTransform.m_data);
  params.m_color = glsl::ToVec4(color);

  auto gpuProgram = mng->GetProgram(program);
  TBase::Render(context, gpuProgram, m_state, mng->GetParamsSetter(), params);
}

math::Matrix<float, 4, 4> Arrow3d::CalculateTransform(ScreenBase const & screen, float dz, float scaleFactor) const
{
  double arrowScale = VisualParams::Instance().GetVisualScale() * kArrowSize * scaleFactor;
  if (screen.isPerspective())
  {
    double const t = GetNormalizedZoomLevel(screen.GetScale(), kArrow3dMinZoom);
    arrowScale *= (kArrow3dScaleMin * (1.0 - t) + kArrow3dScaleMax * t);
  }

  double const scaleX = arrowScale * 2.0 / screen.PixelRect().SizeX();
  double const scaleY = arrowScale * 2.0 / screen.PixelRect().SizeY();
  double const scaleZ = screen.isPerspective() ? (0.002 * screen.GetDepth3d()) : 1.0;

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
  translateM(3, 2) = dz;

  math::Matrix<float, 4, 4> modelTransform = rotateM * scaleM * translateM;
  if (screen.isPerspective())
    return modelTransform * math::Matrix<float, 4, 4>(screen.Pto3dMatrix());

  return modelTransform;
}
}  // namespace df
