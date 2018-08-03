#include "drape/symbols_texture.hpp"
#include "3party/stb_image/stb_image.h"

#include "indexer/map_style_reader.hpp"

#include "platform/platform.hpp"

#include "coding/reader.hpp"
#include "coding/parse_xml.hpp"

#include "base/string_utils.hpp"

#ifdef DEBUG
#include "3party/glm/glm/gtx/bit.hpp"
#endif

namespace dp
{
namespace
{
using TDefinitionInserter = function<void(string const &, m2::RectF const &)>;
using TSymbolsLoadingCompletion = function<void(unsigned char *, uint32_t, uint32_t)>;
using TSymbolsLoadingFailure = function<void(string const &)>;

class DefinitionLoader
{
public:
  DefinitionLoader(TDefinitionInserter const & definitionInserter, bool convertToUV)
    : m_definitionInserter(definitionInserter)
    , m_convertToUV(convertToUV)
    , m_width(0)
    , m_height(0)
  {}

  bool Push(string const & /*element*/) { return true; }

  void Pop(string const & element)
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

  void AddAttr(string const & attribute, string const & value)
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

  void CharData(string const &) {}

  uint32_t GetWidth() const { return m_width; }
  uint32_t GetHeight() const { return m_height; }

private:
  TDefinitionInserter m_definitionInserter;
  bool m_convertToUV;

  uint32_t m_width;
  uint32_t m_height;

  string m_name;
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

  vector<unsigned char> rawData;
  uint32_t width, height;

  try
  {
    DefinitionLoader loader(definitionInserter, convertToUV);

    {
      ReaderPtr<Reader> reader = GetStyleReader().GetResourceReader(textureName + ".sdf", skinPathName);
      ReaderSource<ReaderPtr<Reader> > source(reader);
      if (!ParseXML(source, loader))
      {
        failureHandler("Error parsing skin");
        return;
      }

      width = loader.GetWidth();
      height = loader.GetHeight();
    }

    {
      ReaderPtr<Reader> reader = GetStyleReader().GetResourceReader(textureName + ".png", skinPathName);
      size_t const size = static_cast<size_t>(reader.Size());
      rawData.resize(size);
      reader.Read(0, &rawData[0], size);
    }
  }
  catch (RootException & e)
  {
    failureHandler(e.what());
    return;
  }

  int w, h, bpp;
  unsigned char * data = stbi_load_from_memory(&rawData[0], static_cast<int>(rawData.size()), &w, &h, &bpp, 0);
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

SymbolsTexture::SymbolKey::SymbolKey(string const & symbolName)
  : m_symbolName(symbolName)
{}

Texture::ResourceType SymbolsTexture::SymbolKey::GetType() const
{
  return Texture::ResourceType::Symbol;
}

string const & SymbolsTexture::SymbolKey::GetSymbolName() const
{
  return m_symbolName;
}

SymbolsTexture::SymbolInfo::SymbolInfo(const m2::RectF & texRect)
  : ResourceInfo(texRect)
{}

Texture::ResourceType SymbolsTexture::SymbolInfo::GetType() const
{
  return Texture::ResourceType::Symbol;
}

SymbolsTexture::SymbolsTexture(std::string const & skinPathName, std::string const & textureName,
                               ref_ptr<HWTextureAllocator> allocator)
  : m_name(textureName)
{
  Load(skinPathName, allocator);
}

void SymbolsTexture::Load(std::string const & skinPathName, ref_ptr<HWTextureAllocator> allocator)
{
  auto definitionInserter = [this](string const & name, m2::RectF const & rect)
  {
    m_definition.insert(std::make_pair(name, SymbolsTexture::SymbolInfo(rect)));
  };

  auto completionHandler = [this, &allocator](unsigned char * data, uint32_t width, uint32_t height)
  {
    Texture::Params p;
    p.m_allocator = allocator;
    p.m_format = dp::TextureFormat::RGBA8;
    p.m_width = width;
    p.m_height = height;

    Create(p, make_ref(data));
  };

  auto failureHandler = [this](string const & reason)
  {
    LOG(LERROR, (reason));
    Fail();
  };

  LoadSymbols(skinPathName, m_name, true /* convertToUV */, definitionInserter,
              completionHandler, failureHandler);
}

void SymbolsTexture::Invalidate(string const & skinPathName, ref_ptr<HWTextureAllocator> allocator)
{
  Destroy();
  m_definition.clear();

  Load(skinPathName, allocator);
}

ref_ptr<Texture::ResourceInfo> SymbolsTexture::FindResource(Texture::Key const & key, bool & newResource)
{
  newResource = false;
  if (key.GetType() != Texture::ResourceType::Symbol)
    return nullptr;

  string const & symbolName = static_cast<SymbolKey const &>(key).GetSymbolName();

  auto it = m_definition.find(symbolName);
  ASSERT(it != m_definition.end(), (symbolName));
  return make_ref(&it->second);
}

void SymbolsTexture::Fail()
{
  m_definition.clear();
  int32_t alfaTexture = 0;
  Texture::Params p;
  p.m_allocator = GetDefaultAllocator();
  p.m_format = dp::TextureFormat::RGBA8;
  p.m_width = 1;
  p.m_height = 1;

  Create(p, make_ref(&alfaTexture));
}

bool SymbolsTexture::IsSymbolContained(std::string const & symbolName) const
{
  return m_definition.find(symbolName) != m_definition.end();
}

bool SymbolsTexture::DecodeToMemory(std::string const & skinPathName, std::string const & textureName,
                                    vector<uint8_t> & symbolsSkin,
                                    std::map<string, m2::RectU> & symbolsIndex,
                                    uint32_t & skinWidth, uint32_t & skinHeight)
{
  auto definitionInserter = [&symbolsIndex](string const & name, m2::RectF const & rect)
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

  auto failureHandler = [&result](string const & reason)
  {
    LOG(LERROR, (reason));
    result = false;
  };

  LoadSymbols(skinPathName, textureName, false /* convertToUV */,
              definitionInserter, completionHandler, failureHandler);
  return result;
}
}  // namespace dp
