#include "texture_structure_desc.hpp"

#include "../platform/platform.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/reader.hpp"
#include "../coding/parse_xml.hpp"

#include "../base/string_utils.hpp"

namespace
{
  class SknLoad
  {
  public:
    SknLoad(map<string, m2::RectU> & skn)
      : m_skn(skn)
    {
    }

    bool Push(string const & /*element*/) { return true;}

    void Pop(string const & element)
    {
      if (element == "symbol")
      {
        ASSERT(!m_name.empty(), ());
        ASSERT(m_rect.IsValid(), ());
        m_skn.insert(make_pair(m_name, m_rect));

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
          m_rect.setMinX(v);
        else if (attribute == "minY")
          m_rect.setMinY(v);
        else if (attribute == "maxX")
          m_rect.setMaxX(v);
        else if (attribute == "maxY")
          m_rect.setMaxY(v);
        else if (attribute == "height")
          m_height = v;
        else if (attribute == "width")
          m_width = v;
      }
    }

    void CharData(string const &) {}

    uint32_t m_width;
    uint32_t m_height;

  private:
    string m_name;
    m2::RectU m_rect;

    map<string, m2::RectU> & m_skn;
  };
}

void TextureStructureDesc::Load(const string & descFilePath, uint32_t & width, uint32_t & height)
{
  SknLoad loader(m_structure);

  ReaderSource<ReaderPtr<Reader> > source(ReaderPtr<Reader>(GetPlatform().GetReader(descFilePath)));
  if (!ParseXML(source, loader))
    LOG(LERROR, ("Error parsing skin"));

  width = loader.m_width;
  height = loader.m_height;
}

bool TextureStructureDesc::GetResource(const string & name, m2::RectU & rect) const
{
  structure_t::const_iterator it = m_structure.find(name);
  if (it == m_structure.end())
    return false;

  rect = it->second;
  return true;
}
