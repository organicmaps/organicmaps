#include "drape/symbols_texture.hpp"
#include "3party/stb_image/stb_image.h"

#include "platform/platform.hpp"

#include "coding/reader.hpp"
#include "coding/parse_xml.hpp"

#include "base/string_utils.hpp"

namespace dp
{

class SymbolsTexture::DefinitionLoader
{
public:
  DefinitionLoader(SymbolsTexture::TSymDefinition & definition)
    : m_def(definition)
    , m_width(0)
    , m_height(0)
  {
  }

  bool Push(string const & /*element*/) { return true;}

  void Pop(string const & element)
  {
    if (element == "symbol")
    {
      ASSERT(!m_name.empty(), ());
      ASSERT(m_rect.IsValid(), ());
      m_def.insert(make_pair(m_name, SymbolsTexture::SymbolInfo(m_rect)));

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
        m_rect.setMinX(v / (float)m_width);
      }
      else if (attribute == "minY")
      {
        ASSERT(m_height != 0, ());
        m_rect.setMinY(v / (float)m_height);
      }
      else if (attribute == "maxX")
      {
        ASSERT(m_width != 0, ());
        m_rect.setMaxX(v / (float)m_width);
      }
      else if (attribute == "maxY")
      {
        ASSERT(m_height != 0, ());
        m_rect.setMaxY(v / (float)m_height);
      }
      else if (attribute == "height")
        m_height = v;
      else if (attribute == "width")
        m_width = v;
    }
  }

  void CharData(string const &) {}

  uint32_t GetWidth() const { return m_width; }
  uint32_t GetHeight() const { return m_height; }

private:
  SymbolsTexture::TSymDefinition & m_def;
  uint32_t m_width;
  uint32_t m_height;

  string m_name;
  m2::RectF m_rect;

};

SymbolsTexture::SymbolKey::SymbolKey(string const & symbolName)
  : m_symbolName(symbolName)
{
}

Texture::ResourceType SymbolsTexture::SymbolKey::GetType() const
{
  return Texture::Symbol;
}

string const & SymbolsTexture::SymbolKey::GetSymbolName() const
{
  return m_symbolName;
}

SymbolsTexture::SymbolInfo::SymbolInfo(const m2::RectF & texRect)
  : ResourceInfo(texRect)
{
}

Texture::ResourceType SymbolsTexture::SymbolInfo::GetType() const
{
  return Symbol;
}

SymbolsTexture::SymbolsTexture(string const & skinPathName)
{
  Load(skinPathName);
}

void SymbolsTexture::Load(string const & skinPathName)
{
  vector<unsigned char> rawData;
  uint32_t width, height;

  try
  {
    DefinitionLoader loader(m_definition);

    {
      ReaderPtr<Reader> reader = GetPlatform().GetReader(skinPathName + ".sdf");
      ReaderSource<ReaderPtr<Reader> > source(reader);
      if (!ParseXML(source, loader))
      {
        LOG(LERROR, ("Error parsing skin"));
        Fail();
        return;
      }

      width = loader.GetWidth();
      height = loader.GetHeight();
    }

    {
      ReaderPtr<Reader> reader = GetPlatform().GetReader(skinPathName + ".png");
      size_t const size = reader.Size();
      rawData.resize(size);
      reader.Read(0, &rawData[0], size);
    }
  }
  catch (RootException & e)
  {
    LOG(LERROR, (e.what()));
    Fail();
    return;
  }

  int w, h, bpp;
  unsigned char * data = stbi_png_load_from_memory(&rawData[0], rawData.size(), &w, &h, &bpp, 0);

  if (width == w && height == h)
    Create(width, height, RGBA8, make_ref(data));
  else
    Fail();

  stbi_image_free(data);
}

ref_ptr<Texture::ResourceInfo> SymbolsTexture::FindResource(Texture::Key const & key, bool & newResource)
{
  newResource = false;
  if (key.GetType() != Texture::Symbol)
    return nullptr;

  string const & symbolName = static_cast<SymbolKey const &>(key).GetSymbolName();

  TSymDefinition::iterator it = m_definition.find(symbolName);
  ASSERT(it != m_definition.end(), ());
  return make_ref(&it->second);
}

void SymbolsTexture::Fail()
{
  m_definition.clear();
  int32_t alfaTexture = 0;
  Create(1, 1, RGBA8, make_ref(&alfaTexture));
}

} // namespace dp
