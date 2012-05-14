#include "bookmark.hpp"

#include "../indexer/mercator.hpp"

#include "../coding/parse_xml.hpp"  // LoadFromKML

#include "../base/stl_add.hpp"
#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"

#include <boost/algorithm/string.hpp> // boost::trim


void BookmarkCategory::AddBookmark(Bookmark const & bm)
{
  m_bookmarks.push_back(new Bookmark(bm));
}

BookmarkCategory::~BookmarkCategory()
{
  ClearBookmarks();
}

void BookmarkCategory::ClearBookmarks()
{
  for_each(m_bookmarks.begin(), m_bookmarks.end(), DeleteFunctor());
  m_bookmarks.clear();
}

void BookmarkCategory::DeleteBookmark(size_t index)
{
  if (index < m_bookmarks.size())
  {
    delete m_bookmarks[index];
    m_bookmarks.erase(m_bookmarks.begin() + index);
  }
}

Bookmark const * BookmarkCategory::GetBookmark(size_t index) const
{
  return (index < m_bookmarks.size() ? m_bookmarks[index] : 0);
}

namespace
{
  class KMLParser
  {
    BookmarkCategory & m_category;

    int m_level;

    string m_name;
    m2::PointD m_org;

    void Reset()
    {
      m_name.clear();
      m_org = m2::PointD(-1000, -1000);
    }

    void SetOrigin(string const & s)
    {
      // order in string is: lon, lat, z

      strings::SimpleTokenizer iter(s, ", ");
      if (iter)
      {
        double lon;
        if (strings::to_double(*iter, lon) && MercatorBounds::ValidLon(lon) && ++iter)
        {
          double lat;
          if (strings::to_double(*iter, lat) && MercatorBounds::ValidLat(lat))
            m_org = m2::PointD(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat));
        }
      }
    }

    bool IsValid() const
    {
      return (!m_name.empty() &&
              MercatorBounds::ValidX(m_org.x) && MercatorBounds::ValidY(m_org.y));
    }

  public:
    KMLParser(BookmarkCategory & cat) : m_category(cat), m_level(0)
    {
      Reset();
    }

    bool Push(string const & name)
    {
      switch (m_level)
      {
      case 0:
        if (name != "kml") return false;
        break;

      case 1:
        if (name != "Document") return false;
        break;

      case 2:
        if (name != "Placemark") return false;
        break;

      case 3:
        if (name != "Point" && name != "name") return false;
        break;

      case 4:
        if (name != "coordinates") return false;
      }

      ++m_level;
      return true;
    }

    void AddAttr(string const &, string const &) {}

    void Pop(string const &)
    {
      --m_level;

      if (m_level == 2 && IsValid())
      {
        m_category.AddBookmark(Bookmark(m_org, m_name));
        Reset();
      }
    }

    class IsSpace
    {
    public:
      bool operator() (char c) const
      {
        return ::isspace(c);
      }
    };

    void CharData(string value)
    {
      boost::trim(value);

      if (!value.empty())
        switch (m_level)
        {
        case 4: m_name = value; break;
        case 5: SetOrigin(value); break;
        }
    }
  };
}

void BookmarkCategory::LoadFromKML(ReaderPtr<Reader> const & reader)
{
  ReaderSource<ReaderPtr<Reader> > src(reader);
  KMLParser parser(*this);
  ParseXML(src, parser, true);
}

namespace
{
char const * kmlHeader =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<kml xmlns=\"http://earth.google.com/kml/2.2\">\n"
    "<Document>\n"
    "  <name>MapsWithMe</name>\n";

char const * kmlFooter =
    "</Document>\n"
    "</kml>\n";

string PointToString(m2::PointD const & org)
{
  double const lon = MercatorBounds::XToLon(org.x);
  double const lat = MercatorBounds::YToLat(org.y);

  ostringstream ss;
  ss.precision(8);

  ss << lon << "," << lat;
  return ss.str();
}

}

void BookmarkCategory::SaveToKML(ostream & s)
{
  s << kmlHeader;

  for (size_t i = 0; i < m_bookmarks.size(); ++i)
  {
    Bookmark const * bm = m_bookmarks[i];
    s << "  <Placemark>\n"
      << "    <name>" << bm->GetName() << "</name>\n"
      << "    <Point>\n"
      << "      <coordinates>" << PointToString(bm->GetOrg()) << "</coordinates>\n"
      << "    </Point>\n"
      << "  </Placemark>\n";
  }

  s << kmlFooter;
}
