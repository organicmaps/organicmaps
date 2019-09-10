#include "drape/texture_manager.hpp"
#include "drape/symbols_texture.hpp"
#include "drape/font_texture.hpp"
#include "drape/static_texture.hpp"
#include "drape/stipple_pen_resource.hpp"
#include "drape/texture_of_colors.hpp"
#include "drape/gl_functions.hpp"
#include "drape/support_manager.hpp"
#include "drape/utils/glyph_usage_tracker.hpp"

#include "platform/platform.hpp"

#include "coding/reader.hpp"

#include "base/buffer_vector.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace dp
{
uint32_t const kMaxTextureSize = 1024;
uint32_t const kStippleTextureWidth = 512;
uint32_t const kMinStippleTextureHeight = 64;
uint32_t const kMinColorTextureSize = 32;
uint32_t const kGlyphsTextureSize = 1024;
size_t const kInvalidGlyphGroup = std::numeric_limits<size_t>::max();

uint32_t const kReservedPatterns = 10;
size_t const kReservedColors = 20;

float const kGlyphAreaMultiplier = 1.2f;
float const kGlyphAreaCoverage = 0.9f;

std::string const kDefaultSymbolsTexture = "symbols";
std::string const kSymbolTextures[] = { kDefaultSymbolsTexture, "symbols-ad" };
uint32_t const kDefaultSymbolsIndex = 0;

namespace
{
void MultilineTextToUniString(TextureManager::TMultilineText const & text, strings::UniString & outString)
{
  size_t cnt = 0;
  for (strings::UniString const & str : text)
    cnt += str.size();

  outString.clear();
  outString.reserve(cnt);
  for (strings::UniString const & str : text)
    outString.append(str.begin(), str.end());
}

std::string ReadFileToString(std::string const & filename)
{
  std::string result;
  try
  {
    ReaderPtr<Reader>(GetPlatform().GetReader(filename)).ReadAsString(result);
  }
  catch(RootException const & e)
  {
    LOG(LWARNING, ("Error reading file ", filename, " : ", e.what()));
    return "";
  }
  return result;
}

template <typename ToDo>
void ParseColorsList(std::string const & colorsFile, ToDo toDo)
{
  std::istringstream fin(ReadFileToString(colorsFile));
  while (true)
  {
    uint32_t color;
    fin >> color;
    if (!fin)
      break;

    toDo(dp::Extract(color));
  }
}

template <typename ToDo>
void ParsePatternsList(std::string const & patternsFile, ToDo toDo)
{
  strings::Tokenize(ReadFileToString(patternsFile), "\n", [&](std::string const & patternStr)
  {
    if (patternStr.empty())
      return;

    buffer_vector<double, 8> pattern;
    strings::Tokenize(patternStr, " ", [&](std::string const & token)
    {
      double d = 0.0;
      VERIFY(strings::to_double(token, d), ());
      pattern.push_back(d);
    });

    bool isValid = true;
    for (size_t i = 0; i < pattern.size(); i++)
    {
      if (fabs(pattern[i]) < 1e-5)
      {
        LOG(LWARNING, ("Pattern was skipped", patternStr));
        isValid = false;
        break;
      }
    }

    if (isValid)
      toDo(pattern);
  });
}
m2::PointU StipplePenTextureSize(size_t patternsCount, uint32_t maxTextureSize)
{
  uint32_t const sz = base::NextPowOf2(static_cast<uint32_t>(patternsCount) + kReservedPatterns);
  uint32_t const stippleTextureHeight =
      std::min(maxTextureSize, std::max(sz, kMinStippleTextureHeight));
  return m2::PointU(kStippleTextureWidth, stippleTextureHeight);
}

m2::PointU ColorTextureSize(size_t colorsCount, uint32_t maxTextureSize)
{
  uint32_t const sz = static_cast<uint32_t>(floor(sqrt(colorsCount + kReservedColors)));
  uint32_t colorTextureSize = std::max(base::NextPowOf2(sz), kMinColorTextureSize);
  colorTextureSize *= ColorTexture::GetColorSizeInPixels();
  colorTextureSize = std::min(maxTextureSize, colorTextureSize);
  return m2::PointU(colorTextureSize, colorTextureSize);
}
}  // namespace

TextureManager::TextureManager(ref_ptr<GlyphGenerator> glyphGenerator)
  : m_maxTextureSize(0)
  , m_maxGlypsCount(0)
  , m_glyphGenerator(glyphGenerator)
{
  m_nothingToUpload.test_and_set();
}

TextureManager::BaseRegion::BaseRegion()
  : m_info(nullptr)
  , m_texture(nullptr)
{}

bool TextureManager::BaseRegion::IsValid() const
{
  return m_info != nullptr && m_texture != nullptr;
}

void TextureManager::BaseRegion::SetResourceInfo(ref_ptr<Texture::ResourceInfo> info)
{
  m_info = info;
}

void TextureManager::BaseRegion::SetTexture(ref_ptr<Texture> texture)
{
  m_texture = texture;
}

m2::PointF TextureManager::BaseRegion::GetPixelSize() const
{
  if (!IsValid())
    return m2::PointF(0.0f, 0.0f);

  m2::RectF const & texRect = m_info->GetTexRect();
  return m2::PointF(texRect.SizeX() * m_texture->GetWidth(),
                    texRect.SizeY() * m_texture->GetHeight());
}

float TextureManager::BaseRegion::GetPixelHeight() const
{
  if (!IsValid())
    return 0.0f;

  return m_info->GetTexRect().SizeY() * m_texture->GetHeight();
}

m2::RectF const & TextureManager::BaseRegion::GetTexRect() const
{
  if (!IsValid())
  {
    static m2::RectF nilRect(0.0f, 0.0f, 0.0f, 0.0f);
    return nilRect;
  }

  return m_info->GetTexRect();
}

TextureManager::GlyphRegion::GlyphRegion()
  : BaseRegion()
{}

float TextureManager::GlyphRegion::GetOffsetX() const
{
  ASSERT(m_info->GetType() == Texture::ResourceType::Glyph, ());
  return ref_ptr<GlyphInfo>(m_info)->GetMetrics().m_xOffset;
}

float TextureManager::GlyphRegion::GetOffsetY() const
{
  ASSERT(m_info->GetType() == Texture::ResourceType::Glyph, ());
  return ref_ptr<GlyphInfo>(m_info)->GetMetrics().m_yOffset;
}

float TextureManager::GlyphRegion::GetAdvanceX() const
{
  ASSERT(m_info->GetType() == Texture::ResourceType::Glyph, ());
  return ref_ptr<GlyphInfo>(m_info)->GetMetrics().m_xAdvance;
}

float TextureManager::GlyphRegion::GetAdvanceY() const
{
  ASSERT(m_info->GetType() == Texture::ResourceType::Glyph, ());
  return ref_ptr<GlyphInfo>(m_info)->GetMetrics().m_yAdvance;
}

uint32_t TextureManager::StippleRegion::GetMaskPixelLength() const
{
  ASSERT(m_info->GetType() == Texture::ResourceType::StipplePen, ());
  return ref_ptr<StipplePenResourceInfo>(m_info)->GetMaskPixelLength();
}

uint32_t TextureManager::StippleRegion::GetPatternPixelLength() const
{
  ASSERT(m_info->GetType() == Texture::ResourceType::StipplePen, ());
  return ref_ptr<StipplePenResourceInfo>(m_info)->GetPatternPixelLength();
}

void TextureManager::Release()
{
  m_hybridGlyphGroups.clear();

  m_symbolTextures.clear();
  m_stipplePenTexture.reset();
  m_colorTexture.reset();

  m_trafficArrowTexture.reset();
  m_hatchingTexture.reset();
  m_smaaAreaTexture.reset();
  m_smaaSearchTexture.reset();

  m_glyphTextures.clear();

  m_glyphManager.reset();

  m_glyphGenerator->FinishGeneration();
  
  m_isInitialized = false;
  m_nothingToUpload.test_and_set();
}

bool TextureManager::UpdateDynamicTextures(ref_ptr<dp::GraphicsContext> context)
{
  if (!HasAsyncRoutines() && m_nothingToUpload.test_and_set())
  {
    auto const apiVersion = context->GetApiVersion();
    if (apiVersion == dp::ApiVersion::OpenGLES2 || apiVersion == dp::ApiVersion::OpenGLES3)
    {
      // For some reasons OpenGL can not update textures immediately.
      // Here we use some timeout to prevent rendering frozening.
      double const kUploadTimeoutInSeconds = 2.0;
      return m_uploadTimer.ElapsedSeconds() < kUploadTimeoutInSeconds;
    }

    if (apiVersion == dp::ApiVersion::Metal || apiVersion == dp::ApiVersion::Vulkan)
      return false;

    CHECK(false, ("Unsupported API version."));
  }
  
  CHECK(m_isInitialized, ());

  m_uploadTimer.Reset();

  CHECK(m_colorTexture != nullptr, ());
  m_colorTexture->UpdateState(context);

  CHECK(m_stipplePenTexture != nullptr, ());
  m_stipplePenTexture->UpdateState(context);

  UpdateGlyphTextures(context);

  CHECK(m_textureAllocator != nullptr, ());
  m_textureAllocator->Flush();

  return true;
}

void TextureManager::UpdateGlyphTextures(ref_ptr<dp::GraphicsContext> context)
{
  std::lock_guard<std::mutex> lock(m_glyphTexturesMutex);
  for (auto & texture : m_glyphTextures)
    texture->UpdateState(context);
}

bool TextureManager::HasAsyncRoutines() const
{
  CHECK(m_glyphGenerator != nullptr, ());
  return !m_glyphGenerator->IsSuspended();
}

ref_ptr<Texture> TextureManager::AllocateGlyphTexture()
{
  std::lock_guard<std::mutex> lock(m_glyphTexturesMutex);
  m2::PointU size(kGlyphsTextureSize, kGlyphsTextureSize);
  m_glyphTextures.push_back(make_unique_dp<FontTexture>(size, make_ref(m_glyphManager),
                                                        m_glyphGenerator,
                                                        make_ref(m_textureAllocator)));
  return make_ref(m_glyphTextures.back());
}

void TextureManager::GetRegionBase(ref_ptr<Texture> tex, TextureManager::BaseRegion & region,
                                   Texture::Key const & key)
{
  bool isNew = false;
  region.SetResourceInfo(tex != nullptr ? tex->FindResource(key, isNew) : nullptr);
  region.SetTexture(tex);
  ASSERT(region.IsValid(), ());
  if (isNew)
    m_nothingToUpload.clear();
}

void TextureManager::GetGlyphsRegions(ref_ptr<FontTexture> tex, strings::UniString const & text,
                                      int fixedHeight, TGlyphsBuffer & regions)
{
  ASSERT(tex != nullptr, ());

  std::vector<GlyphKey> keys;
  keys.reserve(text.size());
  for (auto const & c : text)
    keys.emplace_back(GlyphKey(c, fixedHeight));

  bool hasNew = false;
  auto resourcesInfo = tex->FindResources(keys, hasNew);
  ASSERT_EQUAL(text.size(), resourcesInfo.size(), ());

  regions.reserve(resourcesInfo.size());
  for (auto const & info : resourcesInfo)
  {
    GlyphRegion reg;
    reg.SetResourceInfo(info);
    reg.SetTexture(tex);
    ASSERT(reg.IsValid(), ());

    regions.push_back(std::move(reg));
  }

  if (hasNew)
    m_nothingToUpload.clear();
}

uint32_t TextureManager::GetNumberOfUnfoundCharacters(strings::UniString const & text, int fixedHeight,
                                                      HybridGlyphGroup const & group) const
{
  uint32_t cnt = 0;
  for (auto const & c : text)
  {
    if (group.m_glyphs.find(std::make_pair(c, fixedHeight)) == group.m_glyphs.end())
      cnt++;
  }

  return cnt;
}

void TextureManager::MarkCharactersUsage(strings::UniString const & text, int fixedHeight,
                                         HybridGlyphGroup & group)
{
  for (auto const & c : text)
    group.m_glyphs.emplace(std::make_pair(c, fixedHeight));
}

size_t TextureManager::FindHybridGlyphsGroup(strings::UniString const & text, int fixedHeight)
{
  if (m_hybridGlyphGroups.empty())
  {
    m_hybridGlyphGroups.push_back(HybridGlyphGroup());
    return 0;
  }

  HybridGlyphGroup & group = m_hybridGlyphGroups.back();
  bool hasEnoughSpace = true;
  if (group.m_texture != nullptr)
    hasEnoughSpace = group.m_texture->HasEnoughSpace(static_cast<uint32_t>(text.size()));

  // If we have got the only hybrid texture (in most cases it is)
  // we can omit checking of glyphs usage.
  if (hasEnoughSpace)
  {
    size_t const glyphsCount = group.m_glyphs.size() + text.size();
    if (m_hybridGlyphGroups.size() == 1 && glyphsCount < m_maxGlypsCount)
      return 0;
  }

  // Looking for a hybrid texture which contains text entirely.
  for (size_t i = 0; i < m_hybridGlyphGroups.size() - 1; i++)
  {
    if (GetNumberOfUnfoundCharacters(text, fixedHeight, m_hybridGlyphGroups[i]) == 0)
      return i;
  }

  // Check if we can contain text in the last hybrid texture.
  uint32_t const unfoundChars = GetNumberOfUnfoundCharacters(text, fixedHeight, group);
  uint32_t const newCharsCount = static_cast<uint32_t>(group.m_glyphs.size()) + unfoundChars;
  if (newCharsCount >= m_maxGlypsCount || !group.m_texture->HasEnoughSpace(unfoundChars))
    m_hybridGlyphGroups.push_back(HybridGlyphGroup());

  return m_hybridGlyphGroups.size() - 1;
}

size_t TextureManager::FindHybridGlyphsGroup(TMultilineText const & text, int fixedHeight)
{
  strings::UniString combinedString;
  MultilineTextToUniString(text, combinedString);

  return FindHybridGlyphsGroup(combinedString, fixedHeight);
}

void TextureManager::Init(ref_ptr<dp::GraphicsContext> context, Params const & params)
{
  CHECK(!m_isInitialized, ());
  
  m_resPostfix = params.m_resPostfix;
  m_textureAllocator = CreateAllocator(context);

  m_maxTextureSize = std::min(kMaxTextureSize, dp::SupportManager::Instance().GetMaxTextureSize());
  auto const apiVersion = context->GetApiVersion();
  if (apiVersion == dp::ApiVersion::OpenGLES2 || apiVersion == dp::ApiVersion::OpenGLES3)
    GLFunctions::glPixelStore(gl_const::GLUnpackAlignment, 1);

  // Initialize symbols.
  for (size_t i = 0; i < ARRAY_SIZE(kSymbolTextures); ++i)
  {
    m_symbolTextures.push_back(make_unique_dp<SymbolsTexture>(context, m_resPostfix, kSymbolTextures[i],
                                                              make_ref(m_textureAllocator)));
  }

  // Initialize static textures.
  m_trafficArrowTexture = make_unique_dp<StaticTexture>(context, "traffic-arrow", m_resPostfix,
                                                        dp::TextureFormat::RGBA8, make_ref(m_textureAllocator));
  m_hatchingTexture = make_unique_dp<StaticTexture>(context, "area-hatching", m_resPostfix,
                                                    dp::TextureFormat::RGBA8, make_ref(m_textureAllocator));

  // SMAA is not supported on OpenGL ES2.
  if (apiVersion != dp::ApiVersion::OpenGLES2)
  {
    m_smaaAreaTexture = make_unique_dp<StaticTexture>(context, "smaa-area", StaticTexture::kDefaultResource,
                                                      dp::TextureFormat::RedGreen, make_ref(m_textureAllocator));
    m_smaaSearchTexture = make_unique_dp<StaticTexture>(context, "smaa-search", StaticTexture::kDefaultResource,
                                                        dp::TextureFormat::Alpha, make_ref(m_textureAllocator));
  }

  // Initialize patterns.
  buffer_vector<buffer_vector<uint8_t, 8>, 64> patterns;
  double const visualScale = params.m_visualScale;
  ParsePatternsList(params.m_patterns, [&patterns, visualScale](buffer_vector<double, 8>  const & pattern)
  {
    buffer_vector<uint8_t, 8> p;
    for (size_t i = 0; i < pattern.size(); i++)
      p.push_back(static_cast<uint8_t>(pattern[i] * visualScale));
    patterns.push_back(std::move(p));
  });
  m_stipplePenTexture = make_unique_dp<StipplePenTexture>(StipplePenTextureSize(patterns.size(), m_maxTextureSize),
                                                          make_ref(m_textureAllocator));
  LOG(LDEBUG, ("Patterns texture size =", m_stipplePenTexture->GetWidth(), m_stipplePenTexture->GetHeight()));

  ref_ptr<StipplePenTexture> stipplePenTextureTex = make_ref(m_stipplePenTexture);
  for (auto it = patterns.begin(); it != patterns.end(); ++it)
    stipplePenTextureTex->ReservePattern(*it);

  // Initialize colors.
  buffer_vector<dp::Color, 256> colors;
  ParseColorsList(params.m_colors, [&colors](dp::Color const & color)
  {
    colors.push_back(color);
  });
  m_colorTexture = make_unique_dp<ColorTexture>(ColorTextureSize(colors.size(), m_maxTextureSize),
                                                make_ref(m_textureAllocator));
  LOG(LDEBUG, ("Colors texture size =", m_colorTexture->GetWidth(), m_colorTexture->GetHeight()));

  ref_ptr<ColorTexture> colorTex = make_ref(m_colorTexture);
  for (auto it = colors.begin(); it != colors.end(); ++it)
    colorTex->ReserveColor(*it);

  // Initialize glyphs.
  m_glyphManager = make_unique_dp<GlyphManager>(params.m_glyphMngParams);
  uint32_t const textureSquare = kGlyphsTextureSize * kGlyphsTextureSize;
  uint32_t const baseGlyphHeight =
      static_cast<uint32_t>(params.m_glyphMngParams.m_baseGlyphHeight * kGlyphAreaMultiplier);
  uint32_t const averageGlyphSquare = baseGlyphHeight * baseGlyphHeight;
  m_maxGlypsCount = static_cast<uint32_t>(ceil(kGlyphAreaCoverage * textureSquare / averageGlyphSquare));

  m_isInitialized = true;
  m_nothingToUpload.clear();
}

void TextureManager::OnSwitchMapStyle(ref_ptr<dp::GraphicsContext> context)
{
  CHECK(m_isInitialized, ());
  
  // Here we need invalidate only textures which can be changed in map style switch.
  // Now we update only symbol textures, if we need update other textures they must be added here.
  // For Vulkan we use m_texturesToCleanup to defer textures destroying.
  for (size_t i = 0; i < m_symbolTextures.size(); ++i)
  {
    ASSERT(m_symbolTextures[i] != nullptr, ());
    ASSERT(dynamic_cast<SymbolsTexture *>(m_symbolTextures[i].get()) != nullptr, ());
    ref_ptr<SymbolsTexture> symbolsTexture = make_ref(m_symbolTextures[i]);

    if (context->GetApiVersion() != dp::ApiVersion::Vulkan)
      symbolsTexture->Invalidate(context, m_resPostfix, make_ref(m_textureAllocator));
    else
      symbolsTexture->Invalidate(context, m_resPostfix, make_ref(m_textureAllocator), m_texturesToCleanup);
  }
}

void TextureManager::GetTexturesToCleanup(std::vector<drape_ptr<HWTexture>> & textures)
{
  CHECK(m_isInitialized, ());
  std::swap(textures, m_texturesToCleanup);
}

void TextureManager::GetSymbolRegion(std::string const & symbolName, SymbolRegion & region)
{
  CHECK(m_isInitialized, ());
  for (size_t i = 0; i < m_symbolTextures.size(); ++i)
  {
    ASSERT(m_symbolTextures[i] != nullptr, ());
    ref_ptr<SymbolsTexture> symbolsTexture = make_ref(m_symbolTextures[i]);
    if (symbolsTexture->IsSymbolContained(symbolName))
    {
      GetRegionBase(symbolsTexture, region, SymbolsTexture::SymbolKey(symbolName));
      region.SetTextureIndex(static_cast<uint32_t>(i));
      return;
    }
  }
  LOG(LWARNING, ("Detected using of unknown symbol ", symbolName));
}

bool TextureManager::HasSymbolRegion(std::string const & symbolName) const
{
  CHECK(m_isInitialized, ());
  for (size_t i = 0; i < m_symbolTextures.size(); ++i)
  {
    ASSERT(m_symbolTextures[i] != nullptr, ());
    ref_ptr<SymbolsTexture> symbolsTexture = make_ref(m_symbolTextures[i]);
    if (symbolsTexture->IsSymbolContained(symbolName))
      return true;
  }
  return false;
}

void TextureManager::GetStippleRegion(TStipplePattern const & pen, StippleRegion & region)
{
  CHECK(m_isInitialized, ());
  GetRegionBase(make_ref(m_stipplePenTexture), region, StipplePenKey(pen));
}

void TextureManager::GetColorRegion(Color const & color, ColorRegion & region)
{
  CHECK(m_isInitialized, ());
  GetRegionBase(make_ref(m_colorTexture), region, ColorKey(color));
}

void TextureManager::GetGlyphRegions(TMultilineText const & text, int fixedHeight,
                                     TMultilineGlyphsBuffer & buffers)
{
  std::lock_guard<std::mutex> lock(m_calcGlyphsMutex);
  CalcGlyphRegions<TMultilineText, TMultilineGlyphsBuffer>(text, fixedHeight, buffers);
}

void TextureManager::GetGlyphRegions(strings::UniString const & text, int fixedHeight,
                                     TGlyphsBuffer & regions)
{
  std::lock_guard<std::mutex> lock(m_calcGlyphsMutex);
  CalcGlyphRegions<strings::UniString, TGlyphsBuffer>(text, fixedHeight, regions);
}

uint32_t TextureManager::GetAbsentGlyphsCount(ref_ptr<Texture> texture,
                                              strings::UniString const & text,
                                              int fixedHeight) const
{
  if (texture == nullptr)
    return 0;

  ASSERT(dynamic_cast<FontTexture *>(texture.get()) != nullptr, ());
  return static_cast<FontTexture *>(texture.get())->GetAbsentGlyphsCount(text, fixedHeight);
}

uint32_t TextureManager::GetAbsentGlyphsCount(ref_ptr<Texture> texture, TMultilineText const & text,
                                              int fixedHeight) const
{
  if (texture == nullptr)
    return 0;

  uint32_t count = 0;
  for (size_t i = 0; i < text.size(); ++i)
    count += GetAbsentGlyphsCount(texture, text[i], fixedHeight);
  return count;
}

bool TextureManager::AreGlyphsReady(strings::UniString const & str, int fixedHeight) const
{
  CHECK(m_isInitialized, ());
  return m_glyphManager->AreGlyphsReady(str, fixedHeight);
}

ref_ptr<Texture> TextureManager::GetSymbolsTexture() const
{
  CHECK(m_isInitialized, ());
  ASSERT(!m_symbolTextures.empty(), ());
  return make_ref(m_symbolTextures[kDefaultSymbolsIndex]);
}

ref_ptr<Texture> TextureManager::GetTrafficArrowTexture() const
{
  CHECK(m_isInitialized, ());
  return make_ref(m_trafficArrowTexture);
}

ref_ptr<Texture> TextureManager::GetHatchingTexture() const
{
  CHECK(m_isInitialized, ());
  return make_ref(m_hatchingTexture);
}

ref_ptr<Texture> TextureManager::GetSMAAAreaTexture() const
{
  CHECK(m_isInitialized, ());
  return make_ref(m_smaaAreaTexture);
}

ref_ptr<Texture> TextureManager::GetSMAASearchTexture() const
{
  CHECK(m_isInitialized, ());
  return make_ref(m_smaaSearchTexture);
}

constexpr size_t TextureManager::GetInvalidGlyphGroup()
{
  return kInvalidGlyphGroup;
}
}  // namespace dp
