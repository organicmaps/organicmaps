#include "drape/symbols_texture.hpp"

#include "indexer/map_style_reader.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/parse_xml.hpp"

#include "base/shared_buffer_manager.hpp"
#include "base/string_utils.hpp"

#include "3party/stb_image/stb_image.h"

#ifdef DEBUG
#include <glm/gtc/round.hpp>  // glm::isPowerOfTwo
#endif

#include <functional>
#include <utility>
#include <vector>

namespace dp
{
namespace
{
using TDefinitionInserter = std::function<void(std::string const &, m2::RectF const &)>;
using TSymbolsLoadingCompletion = std::function<void(unsigned char *, uint32_t, uint32_t)>;
using TSymbolsLoadingFailure = std::function<void(std::string const &)>;

class DefinitionLoader
{
public:
  DefinitionLoader(TDefinitionInserter const & definitionInserter, bool convertToUV)
    : m_definitionInserter(definitionInserter)
    , m_convertToUV(convertToUV)
    , m_width(0)
    , m_height(0)
  {}

  bool Push(std::string const & /*element*/) { return true; }

  void Pop(std::string const & element)
  {
    if (element == "symbol")
    {
      ASSERT(!m_name.empty(), ());
      ASSERT(m_rect.IsValid(), ());
      ASSERT(m_definitionInserter != nullptr, ());
      m_definitionInserter(m_name, m_rect);

      m_name = "";
      m_rect.MakeEmpty();
    }
  }

  void AddAttr(std::string const & attribute, std::string const & value)
  {
    if (attribute == "name")
      m_name = value;
    else
    {
      int v;
      if (!strings::to_int(value, v))
        return;

      if (attribute == "minX")
      {
        ASSERT(m_width != 0, ());
        float const scalar = m_convertToUV ? static_cast<float>(m_width) : 1.0f;
        m_rect.setMinX(v / scalar);
      }
      else if (attribute == "minY")
      {
        ASSERT(m_height != 0, ());
        float const scalar = m_convertToUV ? static_cast<float>(m_height) : 1.0f;
        m_rect.setMinY(v / scalar);
      }
      else if (attribute == "maxX")
      {
        ASSERT(m_width != 0, ());
        float const scalar = m_convertToUV ? static_cast<float>(m_width) : 1.0f;
        m_rect.setMaxX(v / scalar);
      }
      else if (attribute == "maxY")
      {
        ASSERT(m_height != 0, ());
        float const scalar = m_convertToUV ? static_cast<float>(m_height) : 1.0f;
        m_rect.setMaxY(v / scalar);
      }
      else if (attribute == "height")
      {
        m_height = v;
      }
      else if (attribute == "width")
      {
        m_width = v;
      }
    }
  }

  void CharData(std::string const &) {}

  uint32_t GetWidth() const { return m_width; }
  uint32_t GetHeight() const { return m_height; }

private:
  TDefinitionInserter m_definitionInserter;
  bool m_convertToUV;

  uint32_t m_width;
  uint32_t m_height;

  std::string m_name;
  m2::RectF m_rect;
};

void LoadSymbols(std::string const & skinPathName, std::string const & textureName,
                 bool convertToUV, TDefinitionInserter const & definitionInserter,
                 TSymbolsLoadingCompletion const & completionHandler,
                 TSymbolsLoadingFailure const & failureHandler)
{
  ASSERT(definitionInserter != nullptr, ());
  ASSERT(completionHandler != nullptr, ());
  ASSERT(failureHandler != nullptr, ());

  std::vector<unsigned char> rawData;
  uint32_t width, height;

  try
  {
    DefinitionLoader loader(definitionInserter, convertToUV);

    {
      auto reader = GetStyleReader().GetResourceReader(textureName + ".sdf", skinPathName);
      ReaderSource source(reader);
      if (!ParseXML(source, loader))
      {
        failureHandler("Error parsing skin");
        return;
      }

      width = loader.GetWidth();
      height = loader.GetHeight();
    }

    {
      auto reader = GetStyleReader().GetResourceReader(textureName + ".png", skinPathName);
      size_t const size = static_cast<size_t>(reader.Size());
      rawData.resize(size);
      reader.Read(0, rawData.data(), size);
    }
  }
  catch (RootException & e)
  {
    failureHandler(e.what());
    return;
  }

  int w, h, bpp;
  unsigned char * data = stbi_load_from_memory(rawData.data(), static_cast<int>(rawData.size()), &w, &h, &bpp, 0);
  ASSERT_EQUAL(bpp, 4, ("Incorrect symbols texture format"));
  ASSERT(glm::isPowerOfTwo(w), (w));
  ASSERT(glm::isPowerOfTwo(h), (h));

  if (width == static_cast<uint32_t>(w) && height == static_cast<uint32_t>(h))
  {
    completionHandler(data, width, height);
  }
  else
  {
    failureHandler("Error symbols texture creation");
  }

  stbi_image_free(data);
}
}  // namespace

SymbolsTexture::SymbolsTexture(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
                               std::string const & textureName, ref_ptr<HWTextureAllocator> allocator)
  : m_name(textureName)
{
  Load(context, skinPathName, allocator);
}

void SymbolsTexture::Load(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
                          ref_ptr<HWTextureAllocator> allocator)
{
  auto definitionInserter = [this](std::string const & name, m2::RectF const & rect)
  {
    m_definition.emplace(name, SymbolInfo(rect));
  };

  auto completionHandler = [this, &allocator, context](unsigned char * data, uint32_t width, uint32_t height)
  {
    Texture::Params p;
    p.m_allocator = allocator;
    p.m_format = dp::TextureFormat::RGBA8;
    p.m_width = width;
    p.m_height = height;

    Create(context, p, make_ref(data));
  };

  auto failureHandler = [this, context](std::string const & reason)
  {
    LOG(LERROR, (reason));
    Fail(context);
  };

  LoadSymbols(skinPathName, m_name, true /* convertToUV */, definitionInserter,
              completionHandler, failureHandler);
}

void SymbolsTexture::Invalidate(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
                                ref_ptr<HWTextureAllocator> allocator)
{
  Destroy();
  m_definition.clear();

  Load(context, skinPathName, allocator);
}

void SymbolsTexture::Invalidate(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
                                ref_ptr<HWTextureAllocator> allocator,
                                std::vector<drape_ptr<HWTexture>> & internalTextures)
{
  internalTextures.push_back(std::move(m_hwTexture));
  Invalidate(context, skinPathName, allocator);
}

ref_ptr<Texture::ResourceInfo> SymbolsTexture::FindResource(Texture::Key const & key, bool & newResource)
{
  ASSERT(key.GetType() == Texture::ResourceType::Symbol, ());

  newResource = false;
  std::string const & symbolName = static_cast<SymbolKey const &>(key).GetSymbolName();

  auto it = m_definition.find(symbolName);
  return (it != m_definition.end() ? make_ref(&it->second) : nullptr);
}

void SymbolsTexture::Fail(ref_ptr<dp::GraphicsContext> context)
{
  m_definition.clear();
  int32_t alphaTexture = 0;
  Texture::Params p;
  p.m_allocator = GetDefaultAllocator(context);
  p.m_format = dp::TextureFormat::RGBA8;
  p.m_width = 1;
  p.m_height = 1;

  Create(context, p, make_ref(&alphaTexture));
}

bool SymbolsTexture::DecodeToMemory(std::string const & skinPathName, std::string const & textureName,
                                    std::vector<uint8_t> & symbolsSkin,
                                    std::map<std::string, m2::RectU> & symbolsIndex,
                                    uint32_t & skinWidth, uint32_t & skinHeight)
{
  auto definitionInserter = [&symbolsIndex](std::string const & name, m2::RectF const & rect)
  {
    symbolsIndex.insert(make_pair(name, m2::RectU(rect)));
  };

  bool result = true;
  auto completionHandler = [&result, &symbolsSkin, &skinWidth, &skinHeight](unsigned char * data,
      uint32_t width, uint32_t height)
  {
    size_t size = 4 * width * height;
    symbolsSkin.resize(size);
    memcpy(symbolsSkin.data(), data, size);
    skinWidth = width;
    skinHeight = height;
    result = true;
  };

  auto failureHandler = [&result](std::string const & reason)
  {
    LOG(LERROR, (reason));
    result = false;
  };

  LoadSymbols(skinPathName, textureName, false /* convertToUV */,
              definitionInserter, completionHandler, failureHandler);
  return result;
}

bool LoadedSymbol::FromPngFile(std::string const & filePath)
{
  std::vector<uint8_t> buffer;
  try
  {
    FileReader reader(filePath);
    size_t const size = static_cast<size_t>(reader.Size());
    buffer.resize(size);
    reader.Read(0, buffer.data(), size);
  }
  catch (RootException & e)
  {
    return false;
  }

  int bpp;
  m_data = stbi_load_from_memory(buffer.data(), static_cast<int>(buffer.size()), &m_width, &m_height, &bpp, 0);
  if (m_data && bpp == 4) // only this fits TextureFormat::RGBA8
    return true;

  LOG(LWARNING, ("Error loading PNG for path:", filePath, "Result:", bool(m_data != nullptr), bpp));
  return false;
}

void LoadedSymbol::Free()
{
  if (m_data)
  {
    stbi_image_free(m_data);
    m_data = nullptr;
  }
}

ref_ptr<Texture::ResourceInfo> SymbolsIndex::MapResource(SymbolKey const & key, bool & newResource)
{
  auto const & symbolName = key.GetSymbolName();

  std::lock_guard guard(m_mapping);
  auto it = m_index.find(symbolName);
  if (it != m_index.end())
  {
    newResource = false;
    return make_ref(&it->second);
  }

  LoadedSymbol symbol;
  if (!symbol.FromPngFile(symbolName))
    return {};

  newResource = true;

  m2::RectU pixelRect;
  m_packer.Pack(symbol.m_width, symbol.m_height, pixelRect);

  {
    std::lock_guard<std::mutex> guard(m_upload);
    m_pendingNodes.emplace_back(pixelRect, std::move(symbol));
  }

  auto res = m_index.emplace(symbolName, SymbolInfo(m_packer.MapTextureCoords(pixelRect)));
  ASSERT(res.second, ());
  return make_ref(&res.first->second);
}

void SymbolsIndex::UploadResources(ref_ptr<dp::GraphicsContext> context, ref_ptr<Texture> texture)
{
  TPendingNodes pendingNodes;
  {
    std::lock_guard<std::mutex> upload(m_upload);
    if (m_pendingNodes.empty())
      return;
    m_pendingNodes.swap(pendingNodes);
  }

  for (auto const & [rect, symbol] : pendingNodes)
  {
    m2::PointU const zeroPoint = rect.LeftBottom();
    texture->UploadData(context, zeroPoint.x, zeroPoint.y, rect.SizeX(), rect.SizeY(), make_ref(symbol.m_data));
  }
}

}  // namespace dp
