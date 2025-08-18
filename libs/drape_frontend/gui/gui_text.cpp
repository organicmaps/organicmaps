#include "drape_frontend/gui/gui_text.hpp"

#include "drape_frontend/batcher_bucket.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/programs.hpp"

#include "base/stl_helpers.hpp"

#include "drape/font_constants.hpp"

#include <algorithm>
#include <array>
#include <memory>

namespace gui
{
namespace
{
glsl::vec2 GetNormalsAndMask(dp::TextureManager::GlyphRegion const & glyph, float xOffset, float yOffset,
                             float textRatio, std::array<glsl::vec2, 4> & normals,
                             std::array<glsl::vec2, 4> & maskTexCoord)
{
  m2::PointF const pixelSize = glyph.GetPixelSize() * textRatio;
  m2::RectF const & r = glyph.GetTexRect();

  xOffset *= textRatio;
  yOffset *= textRatio;

  float const upVector = -pixelSize.y - yOffset;
  float const bottomVector = -yOffset;

  normals[0] = glsl::vec2(xOffset, bottomVector);
  normals[1] = glsl::vec2(xOffset, upVector);
  normals[2] = glsl::vec2(pixelSize.x + xOffset, bottomVector);
  normals[3] = glsl::vec2(pixelSize.x + xOffset, upVector);

  maskTexCoord[0] = glsl::ToVec2(r.LeftTop());
  maskTexCoord[1] = glsl::ToVec2(r.LeftBottom());
  maskTexCoord[2] = glsl::ToVec2(r.RightTop());
  maskTexCoord[3] = glsl::ToVec2(r.RightBottom());

  return {xOffset, yOffset};
}

void FillCommonDecl(dp::BindingDecl & decl, std::string const & name, uint8_t compCount, uint8_t stride, uint8_t offset)
{
  decl.m_attributeName = name;
  decl.m_componentCount = compCount;
  decl.m_componentType = gl_const::GLFloatType;
  decl.m_stride = stride;
  decl.m_offset = offset;
}

void FillPositionDecl(dp::BindingDecl & decl, uint8_t stride, uint8_t offset)
{
  FillCommonDecl(decl, "a_position", 3, stride, offset);
}

void FillNormalDecl(dp::BindingDecl & decl, uint8_t stride, uint8_t offset)
{
  FillCommonDecl(decl, "a_normal", 2, stride, offset);
}

void FillColorDecl(dp::BindingDecl & decl, uint8_t stride, uint8_t offset)
{
  FillCommonDecl(decl, "a_colorTexCoord", 2, stride, offset);
}

void FillOutlineDecl(dp::BindingDecl & decl, uint8_t stride, uint8_t offset)
{
  FillCommonDecl(decl, "a_outlineColorTexCoord", 2, stride, offset);
}

void FillMaskDecl(dp::BindingDecl & decl, uint8_t stride, uint8_t offset)
{
  FillCommonDecl(decl, "a_maskTexCoord", 2, stride, offset);
}
}  // namespace

dp::BindingInfo const & StaticLabel::Vertex::GetBindingInfo()
{
  static std::unique_ptr<dp::BindingInfo> info;

  if (info == nullptr)
  {
    info = std::make_unique<dp::BindingInfo>(5);
    uint8_t constexpr stride = sizeof(Vertex);
    uint8_t offset = 0;

    FillPositionDecl(info->GetBindingDecl(0), stride, offset);
    offset += sizeof(glsl::vec3);
    FillColorDecl(info->GetBindingDecl(1), stride, offset);
    offset += sizeof(glsl::vec2);
    FillOutlineDecl(info->GetBindingDecl(2), stride, offset);
    offset += sizeof(glsl::vec2);
    FillNormalDecl(info->GetBindingDecl(3), stride, offset);
    offset += sizeof(glsl::vec2);
    FillMaskDecl(info->GetBindingDecl(4), stride, offset);
    ASSERT_EQUAL(offset + sizeof(glsl::vec2), stride, ());
  }

  return *info;
}

StaticLabel::LabelResult::LabelResult()
  : m_state(df::CreateRenderState(gpu::Program::TextStaticOutlinedGui, df::DepthLayer::GuiLayer))
{
  m_state.SetDepthTestEnabled(false);
}

dp::TGlyphs StaticLabel::CacheStaticText(std::string const & text, char const * delimiters, dp::Anchor anchor,
                                         dp::FontDecl const & font, ref_ptr<dp::TextureManager> mng,
                                         LabelResult & result)
{
  ASSERT(!text.empty(), ());

  auto const textRatio =
      font.m_size * static_cast<float>(df::VisualParams::Instance().GetVisualScale() / dp::kBaseFontSizePixels);

  dp::TextureManager::TMultilineGlyphsBuffer buffers;
  auto const shapedLines = mng->ShapeMultilineText(dp::kBaseFontSizePixels, text, delimiters, buffers);

  ASSERT_EQUAL(shapedLines.size(), buffers.size(), ());

#ifdef DEBUG
  for (size_t i = 0; i < buffers.size(); ++i)
  {
    ASSERT(!buffers[i].empty(), ());
    ASSERT_EQUAL(buffers[i].size(), shapedLines[i].m_glyphs.size(), ());
  }

  ref_ptr<dp::Texture> texture = buffers[0][0].GetTexture();
  for (dp::TextureManager::TGlyphsBuffer const & b : buffers)
    for (dp::TextureManager::GlyphRegion const & reg : b)
      ASSERT(texture == reg.GetTexture(), ());
#endif

  dp::TextureManager::ColorRegion color;
  dp::TextureManager::ColorRegion outline;
  mng->GetColorRegion(font.m_color, color);
  mng->GetColorRegion(font.m_outlineColor, outline);
  ASSERT(color.GetTexture() == outline.GetTexture(), ());

  glsl::vec2 colorTex = glsl::ToVec2(color.GetTexRect().Center());
  glsl::vec2 outlineTex = glsl::ToVec2(outline.GetTexRect().Center());

  buffer_vector<float, 4> lineLengths;
  lineLengths.reserve(buffers.size());

  buffer_vector<size_t, 4> ranges;
  ranges.reserve(buffers.size());

  float fullHeight = 0.0;

  buffer_vector<Vertex, 128> & rb = result.m_buffer;
  for (int i = static_cast<int>(buffers.size()) - 1; i >= 0; --i)
  {
    auto const & glyphs = shapedLines[i].m_glyphs;

    dp::TextureManager::TGlyphsBuffer & regions = buffers[i];
    lineLengths.push_back(0.0f);
    float & currentLineLength = lineLengths.back();

    float depth = 0.0;
    glsl::vec2 pen(0.0, -fullHeight);
    float prevLineHeight = 0.0;
    for (size_t j = 0; j < regions.size(); ++j)
    {
      auto const & glyphMetrics = glyphs[j];

      std::array<glsl::vec2, 4> normals, maskTex;

      dp::TextureManager::GlyphRegion const & glyph = regions[j];
      glsl::vec2 offsets =
          GetNormalsAndMask(glyph, glyphMetrics.m_xOffset, glyphMetrics.m_yOffset, textRatio, normals, maskTex);

      glsl::vec3 position = glsl::vec3(0.0, 0.0, depth);

      for (size_t v = 0; v < normals.size(); ++v)
        rb.emplace_back(position, colorTex, outlineTex, pen + normals[v], maskTex[v]);

      float const advance = glyphMetrics.m_xAdvance * textRatio;
      prevLineHeight = std::max(prevLineHeight, offsets.y + glyph.GetPixelHeight() * textRatio);

      pen += glsl::vec2(advance, glyphMetrics.m_yAdvance * textRatio);

      depth += 10.0f;
      if (j == 0)
        currentLineLength += (glyph.GetPixelSize().x * textRatio + offsets.x);
      else
        currentLineLength += advance;

      if (j == regions.size() - 1)
        currentLineLength += offsets.x;
    }

    ranges.push_back(rb.size());

    fullHeight += prevLineHeight;
  }

  float const halfHeight = 0.5f * fullHeight;

  float yOffset = halfHeight;
  if (anchor & dp::Top)
    yOffset = fullHeight;
  else if (anchor & dp::Bottom)
    yOffset = 0.0f;

  float maxLineLength = 0.0;
  size_t startIndex = 0;
  for (size_t i = 0; i < ranges.size(); ++i)
  {
    maxLineLength = std::max(lineLengths[i], maxLineLength);
    float xOffset = -lineLengths[i] / 2.0f;
    if (anchor & dp::Left)
      xOffset = 0;
    else if (anchor & dp::Right)
      xOffset += xOffset;

    size_t endIndex = ranges[i];
    for (size_t j = startIndex; j < endIndex; ++j)
    {
      rb[j].m_normal = rb[j].m_normal + glsl::vec2(xOffset, yOffset);
      result.m_boundRect.Add(glsl::ToPoint(rb[j].m_normal));
    }

    startIndex = endIndex;
  }

  result.m_state.SetColorTexture(color.GetTexture());
  result.m_state.SetMaskTexture(buffers[0][0].GetTexture());

  dp::TGlyphs glyphs;
  for (auto const & line : shapedLines)
    for (auto const & glyph : line.m_glyphs)
      glyphs.emplace_back(glyph.m_key);

  base::SortUnique(glyphs);
  return glyphs;
}

dp::BindingInfo const & MutableLabel::StaticVertex::GetBindingInfo()
{
  static std::unique_ptr<dp::BindingInfo> info;

  if (info == nullptr)
  {
    info = std::make_unique<dp::BindingInfo>(3);

    uint8_t constexpr stride = sizeof(StaticVertex);
    uint8_t offset = 0;

    FillPositionDecl(info->GetBindingDecl(0), stride, offset);
    offset += sizeof(glsl::vec3);
    FillColorDecl(info->GetBindingDecl(1), stride, offset);
    offset += sizeof(glsl::vec2);
    FillOutlineDecl(info->GetBindingDecl(2), stride, offset);
    ASSERT_EQUAL(offset + sizeof(glsl::vec2), stride, ());
  }

  return *info;
}

dp::BindingInfo const & MutableLabel::DynamicVertex::GetBindingInfo()
{
  static std::unique_ptr<dp::BindingInfo> info;

  if (info == nullptr)
  {
    info = std::make_unique<dp::BindingInfo>(2, 1);
    uint8_t constexpr stride = sizeof(DynamicVertex);
    uint8_t offset = 0;

    FillNormalDecl(info->GetBindingDecl(0), stride, offset);
    offset += sizeof(glsl::vec2);
    FillMaskDecl(info->GetBindingDecl(1), stride, offset);
    ASSERT_EQUAL(offset + sizeof(glsl::vec2), stride, ());
  }

  return *info;
}

MutableLabel::PrecacheResult::PrecacheResult()
  : m_state(CreateRenderState(gpu::Program::TextOutlinedGui, df::DepthLayer::GuiLayer))
{
  m_state.SetDepthTestEnabled(false);
}

MutableLabel::MutableLabel(dp::Anchor anchor) : m_anchor(anchor) {}

void MutableLabel::SetMaxLength(uint16_t maxLength)
{
  m_maxLength = maxLength;
}

dp::TGlyphs MutableLabel::GetGlyphs() const
{
  dp::TGlyphs glyphs;
  glyphs.reserve(m_shapedText.m_glyphs.size());

  for (auto const & glyph : m_shapedText.m_glyphs)
    glyphs.emplace_back(glyph.m_key);
  return glyphs;
}

void MutableLabel::Precache(PrecacheParams const & params, PrecacheResult & result, ref_ptr<dp::TextureManager> mng)
{
  SetMaxLength(static_cast<uint16_t>(params.m_maxLength));

  m_textRatio = params.m_font.m_size * static_cast<float>(df::VisualParams::Instance().GetVisualScale()) /
                dp::kBaseFontSizePixels;

  // TODO(AB): Is this shaping/precaching really needed if the text changes every frame?
  m_shapedText = mng->ShapeSingleTextLine(dp::kBaseFontSizePixels, params.m_alphabet, &m_glyphRegions);

  auto const firstTexture = m_glyphRegions.front().GetTexture();
#ifdef DEBUG
  for (auto const & region : m_glyphRegions)
    ASSERT_EQUAL(firstTexture, region.GetTexture(), ());
#endif
  result.m_state.SetMaskTexture(firstTexture);

  dp::TextureManager::ColorRegion color;
  dp::TextureManager::ColorRegion outlineColor;

  mng->GetColorRegion(params.m_font.m_color, color);
  mng->GetColorRegion(params.m_font.m_outlineColor, outlineColor);
  result.m_state.SetColorTexture(color.GetTexture());

  glsl::vec2 colorTex = glsl::ToVec2(color.GetTexRect().Center());
  glsl::vec2 outlineTex = glsl::ToVec2(outlineColor.GetTexRect().Center());

  auto const vertexCount = m_maxLength * dp::Batcher::VertexPerQuad;
  result.m_buffer.resize(vertexCount, StaticVertex(glsl::vec3(0.0, 0.0, 0.0), colorTex, outlineTex));

  float depth = 0.0f;
  for (size_t i = 0; i < vertexCount; i += 4)
  {
    result.m_buffer[i + 0].m_position.z = depth;
    result.m_buffer[i + 1].m_position.z = depth;
    result.m_buffer[i + 2].m_position.z = depth;
    result.m_buffer[i + 3].m_position.z = depth;
    depth += 10.0f;
  }

  result.m_maxPixelSize = m2::PointF(m_shapedText.m_lineWidthInPixels, m_shapedText.m_maxLineHeightInPixels);
}

void MutableLabel::SetText(LabelResult & result, std::string text, ref_ptr<dp::TextureManager> mng)
{
  if (size_t const sz = text.size(); sz < m_maxLength)
  {
    /// @todo I don't see a better way to clear cached vertices from the previous frame (text value).
    text.append(m_maxLength - sz, ' ');
  }
  else if (sz > m_maxLength)
  {
    text.erase(m_maxLength - 3);
    text.append("...");
  }

  // TODO(AB): Calculate only the length for pre-cached glyphs in a simpler way?
  m_glyphRegions.clear();
  m_shapedText = mng->ShapeSingleTextLine(dp::kBaseFontSizePixels, text, &m_glyphRegions);

  // TODO(AB): Reuse pre-calculated width and height?
  // float maxHeight = m_shapedText.m_maxLineHeightInPixels;
  // float length = m_shapedText.m_lineWidthInPixels;

  float maxHeight = 0.0f;

  std::pair minMaxXPos = {std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest()};
  float offsetLeft = 0;

  glsl::vec2 pen = glsl::vec2(0.0, 0.0);

  ASSERT_EQUAL(m_glyphRegions.size(), m_shapedText.m_glyphs.size(), ());
  for (size_t i = 0; i < m_glyphRegions.size(); ++i)
  {
    std::array<glsl::vec2, 4> normals, maskTex;
    dp::TextureManager::GlyphRegion const & glyph = m_glyphRegions[i];
    auto const & metrics = m_shapedText.m_glyphs[i];
    glsl::vec2 const offsets =
        GetNormalsAndMask(glyph, metrics.m_xOffset, metrics.m_yOffset, m_textRatio, normals, maskTex);

    ASSERT_EQUAL(normals.size(), maskTex.size(), ());

    for (size_t j = 0; j < normals.size(); ++j)
    {
      result.m_buffer.emplace_back(pen + normals[j], maskTex[j]);
      auto const & back = result.m_buffer.back();
      if (back.m_normal.x < minMaxXPos.first)
      {
        minMaxXPos.first = back.m_normal.x;
        offsetLeft = offsets.x;
      }
      minMaxXPos.second = std::max(minMaxXPos.second, back.m_normal.x);
    }

    // TODO(AB): yAdvance is always zero for horizontal layouts.
    pen += glsl::vec2(metrics.m_xAdvance * m_textRatio, metrics.m_yAdvance * m_textRatio);
    maxHeight = std::max(maxHeight, offsets.y + glyph.GetPixelHeight() * m_textRatio);
  }

  float const length = minMaxXPos.second - minMaxXPos.first;
  // "- offset_left" is an approximation
  // A correct version should be
  // "- (offset_first_symbol_from_left + offset_last_symbol_from_right) / 2"
  // But there is no possibility to determine the offset of the last symbol from the right.
  // We only have x_offset which is "offset from left"
  glsl::vec2 anchorModifier = glsl::vec2(-length / 2.0f - offsetLeft, maxHeight / 2.0f);
  if (m_anchor & dp::Right)
    anchorModifier.x = -length;
  else if (m_anchor & dp::Left)
    anchorModifier.x = 0;

  if (m_anchor & dp::Top)
    anchorModifier.y = maxHeight;
  else if (m_anchor & dp::Bottom)
    anchorModifier.y = 0;

  for (DynamicVertex & v : result.m_buffer)
  {
    v.m_normal += anchorModifier;
    result.m_boundRect.Add(glsl::ToPoint(v.m_normal));
  }
}

MutableLabelHandle::MutableLabelHandle(uint32_t id, dp::Anchor anchor, m2::PointF const & pivot)
  : TBase(id, anchor, pivot)
  , m_textView(make_unique_dp<MutableLabel>(anchor))
  , m_isContentDirty(true)
  , m_glyphsReady(false)
{}

MutableLabelHandle::MutableLabelHandle(uint32_t id, dp::Anchor anchor, m2::PointF const & pivot,
                                       ref_ptr<dp::TextureManager> textures)
  : MutableLabelHandle(id, anchor, pivot)
{
  m_textureManager = std::move(textures);
}

void MutableLabelHandle::GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator) const
{
  if (!m_isContentDirty)
    return;

  m_isContentDirty = false;
  MutableLabel::LabelResult result;
  m_textView->SetText(result, m_content, m_textureManager);

  size_t const byteCount = result.m_buffer.size() * sizeof(MutableLabel::DynamicVertex);
  auto const dataPointer = static_cast<MutableLabel::DynamicVertex *>(mutator->AllocateMutationBuffer(byteCount));
  std::copy(result.m_buffer.begin(), result.m_buffer.end(), dataPointer);

  dp::BindingInfo const & binding = MutableLabel::DynamicVertex::GetBindingInfo();
  auto const & node = GetOffsetNode(binding.GetID());
  ASSERT_EQUAL(node.first.GetElementSize(), sizeof(MutableLabel::DynamicVertex), ());
  ASSERT_EQUAL(node.second.m_count, result.m_buffer.size(), ());

  dp::MutateNode mutateNode;
  mutateNode.m_data = make_ref(dataPointer);
  mutateNode.m_region = node.second;
  mutator->AddMutation(node.first, mutateNode);
}

bool MutableLabelHandle::Update(ScreenBase const & screen)
{
  if (!m_glyphsReady)
    m_glyphsReady = m_textureManager->AreGlyphsReady(m_textView->GetGlyphs());

  if (!m_glyphsReady)
    return false;

  return TBase::Update(screen);
}

void MutableLabelHandle::SetTextureManager(ref_ptr<dp::TextureManager> textures)
{
  m_textureManager = textures;
}

ref_ptr<MutableLabel> MutableLabelHandle::GetTextView() const
{
  return make_ref(m_textView);
}

void MutableLabelHandle::SetContent(std::string && content)
{
  if (m_content != content)
  {
    m_isContentDirty = true;
    m_content = std::move(content);
  }
}

void MutableLabelHandle::SetContent(std::string const & content)
{
  if (m_content != content)
  {
    m_isContentDirty = true;
    m_content = content;
  }
}

m2::PointF MutableLabelDrawer::Draw(ref_ptr<dp::GraphicsContext> context, Params const & params,
                                    ref_ptr<dp::TextureManager> mng, dp::Batcher::TFlushFn && flushFn)
{
  uint32_t const vertexCount = dp::Batcher::VertexPerQuad * params.m_maxLength;
  uint32_t const indexCount = dp::Batcher::IndexPerQuad * params.m_maxLength;

  ASSERT(params.m_handleCreator != nullptr, ());
  drape_ptr<MutableLabelHandle> handle = params.m_handleCreator(params.m_anchor, params.m_pivot);

  MutableLabel::PrecacheParams preCacheP;
  preCacheP.m_alphabet = params.m_alphabet;
  preCacheP.m_font = params.m_font;
  preCacheP.m_maxLength = params.m_maxLength;

  MutableLabel::PrecacheResult staticData;

  handle->GetTextView()->Precache(preCacheP, staticData, mng);

  ASSERT_EQUAL(vertexCount, staticData.m_buffer.size(), ());
  buffer_vector<MutableLabel::DynamicVertex, 128> dynData;
  dynData.resize(staticData.m_buffer.size());

  dp::BindingInfo const & sBinding = MutableLabel::StaticVertex::GetBindingInfo();
  dp::BindingInfo const & dBinding = MutableLabel::DynamicVertex::GetBindingInfo();
  dp::AttributeProvider provider(2 /*stream count*/, static_cast<uint32_t>(staticData.m_buffer.size()));
  provider.InitStream(0 /*stream index*/, sBinding, make_ref(staticData.m_buffer.data()));
  provider.InitStream(1 /*stream index*/, dBinding, make_ref(dynData.data()));

  {
    dp::Batcher batcher(indexCount, vertexCount);
    batcher.SetBatcherHash(static_cast<uint64_t>(df::BatcherBucket::Default));
    dp::SessionGuard const guard(context, batcher, std::move(flushFn));
    batcher.InsertListOfStrip(context, staticData.m_state, make_ref(&provider), std::move(handle),
                              dp::Batcher::VertexPerQuad);
  }

  return staticData.m_maxPixelSize;
}

StaticLabelHandle::StaticLabelHandle(uint32_t id, ref_ptr<dp::TextureManager> textureManager, dp::Anchor anchor,
                                     m2::PointF const & pivot, dp::TGlyphs && glyphs)
  : TBase(id, anchor, pivot)
  , m_glyphs(std::move(glyphs))
  , m_textureManager(std::move(textureManager))
  , m_glyphsReady(false)
{}

bool StaticLabelHandle::Update(ScreenBase const & screen)
{
  if (!m_glyphsReady)
    m_glyphsReady = m_textureManager->AreGlyphsReady(m_glyphs);

  if (!m_glyphsReady)
    return false;

  return TBase::Update(screen);
}
}  // namespace gui
