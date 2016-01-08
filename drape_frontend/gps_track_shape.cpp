#include "drape_frontend/gps_track_shape.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/shader_def.hpp"
#include "drape/texture_manager.hpp"

#include "base/logging.hpp"

namespace df
{

namespace
{

uint32_t const kDynamicStreamID = 0x7F;

struct GpsTrackStaticVertex
{
  using TNormal = glsl::vec3;

  GpsTrackStaticVertex() = default;
  GpsTrackStaticVertex(TNormal const & normal) : m_normal(normal) {}

  TNormal m_normal;
};

dp::GLState GetGpsTrackState(ref_ptr<dp::TextureManager> texMng)
{
  dp::GLState state(gpu::TRACK_POINT_PROGRAM, dp::GLState::OverlayLayer);
  state.SetColorTexture(texMng->GetSymbolsTexture());
  return state;
}

dp::BindingInfo const & GetGpsTrackStaticBindingInfo()
{
  static unique_ptr<dp::BindingInfo> s_info;
  if (s_info == nullptr)
  {
    dp::BindingFiller<GpsTrackStaticVertex> filler(1);
    filler.FillDecl<GpsTrackStaticVertex::TNormal>("a_normal");
    s_info.reset(new dp::BindingInfo(filler.m_info));
  }
  return *s_info;
}

dp::BindingInfo const & GetGpsTrackDynamicBindingInfo()
{
  static unique_ptr<dp::BindingInfo> s_info;
  if (s_info == nullptr)
  {
    dp::BindingFiller<GpsTrackDynamicVertex> filler(2, kDynamicStreamID);
    filler.FillDecl<GpsTrackDynamicVertex::TPosition>("a_position");
    filler.FillDecl<GpsTrackDynamicVertex::TColor>("a_color");
    s_info.reset(new dp::BindingInfo(filler.m_info));
  }
  return *s_info;
}

} // namespace

GpsTrackHandle::GpsTrackHandle(size_t pointsCount)
  : OverlayHandle(FeatureID(), dp::Anchor::Center, 0, false)
  , m_needUpdate(false)
{
  m_buffer.resize(pointsCount * dp::Batcher::VertexPerQuad);
}

void GpsTrackHandle::GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator,
                                          ScreenBase const & screen) const
{
  UNUSED_VALUE(screen);

  if (!m_needUpdate)
    return;

  TOffsetNode const & node = GetOffsetNode(kDynamicStreamID);
  ASSERT(node.first.GetElementSize() == sizeof(GpsTrackDynamicVertex), ());
  ASSERT(node.second.m_count == m_buffer.size(), ());

  uint32_t const byteCount = m_buffer.size() * sizeof(GpsTrackDynamicVertex);
  void * buffer = mutator->AllocateMutationBuffer(byteCount);
  memcpy(buffer, m_buffer.data(), byteCount);

  dp::MutateNode mutateNode;
  mutateNode.m_region = node.second;
  mutateNode.m_data = make_ref(buffer);
  mutator->AddMutation(node.first, mutateNode);
}

bool GpsTrackHandle::Update(ScreenBase const & screen)
{
  UNUSED_VALUE(screen);
  return true;
}

bool GpsTrackHandle::IndexesRequired() const
{
  return false;
}

m2::RectD GpsTrackHandle::GetPixelRect(ScreenBase const & screen, bool perspective) const
{
  UNUSED_VALUE(screen);
  UNUSED_VALUE(perspective);
  return m2::RectD();
}

void GpsTrackHandle::GetPixelShape(ScreenBase const & screen, Rects & rects, bool perspective) const
{
  UNUSED_VALUE(screen);
  UNUSED_VALUE(perspective);
}

void GpsTrackHandle::SetPoint(size_t index, m2::PointD const & position,
                              float radius, dp::Color const & color)
{
  size_t bufferIndex = index * dp::Batcher::VertexPerQuad;
  ASSERT_GREATER_OR_EQUAL(bufferIndex, 0, ());
  ASSERT_LESS(bufferIndex, m_buffer.size(), ());

  for (size_t i = 0; i < dp::Batcher::VertexPerQuad; i++)
  {
    m_buffer[bufferIndex + i].m_position = glsl::vec3(position.x, position.y, radius);
    m_buffer[bufferIndex + i].m_color = glsl::ToVec4(color);
  }
  m_needUpdate = true;
}

void GpsTrackHandle::Clear()
{
  memset(m_buffer.data(), 0, m_buffer.size() * sizeof(GpsTrackDynamicVertex));
  m_needUpdate = true;
}

size_t GpsTrackHandle::GetPointsCount() const
{
  return m_buffer.size() / dp::Batcher::VertexPerQuad;
}

void GpsTrackShape::Draw(ref_ptr<dp::TextureManager> texMng, GpsTrackRenderData & data)
{
  ASSERT_NOT_EQUAL(data.m_pointsCount, 0, ());

  size_t const kVerticesInPoint = dp::Batcher::VertexPerQuad;
  size_t const kIndicesInPoint = dp::Batcher::IndexPerQuad;
  vector<GpsTrackStaticVertex> staticVertexData;
  staticVertexData.reserve(data.m_pointsCount * kVerticesInPoint);
  for (size_t i = 0; i < data.m_pointsCount; i++)
  {
    staticVertexData.push_back(GpsTrackStaticVertex(GpsTrackStaticVertex::TNormal(-1.0f, 1.0f, 1.0f)));
    staticVertexData.push_back(GpsTrackStaticVertex(GpsTrackStaticVertex::TNormal(-1.0f, -1.0f, 1.0f)));
    staticVertexData.push_back(GpsTrackStaticVertex(GpsTrackStaticVertex::TNormal(1.0f, 1.0f, 1.0f)));
    staticVertexData.push_back(GpsTrackStaticVertex(GpsTrackStaticVertex::TNormal(1.0f, -1.0f, 1.0f)));
  }

  vector<GpsTrackDynamicVertex> dynamicVertexData;
  dynamicVertexData.resize(data.m_pointsCount * kVerticesInPoint);

  dp::Batcher batcher(data.m_pointsCount * kIndicesInPoint, data.m_pointsCount * kVerticesInPoint);
  dp::SessionGuard guard(batcher, [&data](dp::GLState const & state, drape_ptr<dp::RenderBucket> && b)
  {
    data.m_bucket = move(b);
    data.m_state = state;
  });

  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<GpsTrackHandle>(data.m_pointsCount);

  dp::AttributeProvider provider(2 /* stream count */, staticVertexData.size());
  provider.InitStream(0 /* stream index */, GetGpsTrackStaticBindingInfo(), make_ref(staticVertexData.data()));
  provider.InitStream(1 /* stream index */, GetGpsTrackDynamicBindingInfo(), make_ref(dynamicVertexData.data()));
  batcher.InsertListOfStrip(GetGpsTrackState(texMng), make_ref(&provider), move(handle), kVerticesInPoint);

  GLFunctions::glFlush();
}

} // namespace df
