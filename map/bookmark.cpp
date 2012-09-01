#include "bookmark.hpp"

#include "../indexer/mercator.hpp"

#include "../coding/parse_xml.hpp"  // LoadFromKML

#include "../base/stl_add.hpp"
#include "../base/string_utils.hpp"

#include "../std/algorithm.hpp"


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
  // Fixes icons which are not supported by MapsWithMe
  static string GetSupportedBMType(string const & s)
  {
    static char const * icons[] = {"placemark-red", "placemark-blue", "placemark-purple",
        "placemark-pink", "placemark-brown", "placemark-green", "placemark-orange"};

    // Remove leading '#' symbol
    string result = s.substr(1);
    for (size_t i = 0; i < ARRAY_SIZE(icons); ++i)
      if (result == icons[i])
        return result;
    // Not recognized symbols are replaced with default one
    LOG(LWARNING, ("Bookmark icon is not supported:", result));
    return "placemark-red";
  }

  class KMLParser
  {
    BookmarkCategory & m_category;

    vector<string> m_tags;

    string m_name;
    string m_type;

    m2::PointD m_org;

    void Reset()
    {
      m_name.clear();
      m_org = m2::PointD(-1000, -1000);
      m_type.clear();
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
          else
            LOG(LWARNING, ("Invalid coordinates while loading bookmark:", s));
        }
      }
    }

    bool IsValid() const
    {
      return (!m_name.empty() &&
              MercatorBounds::ValidX(m_org.x) && MercatorBounds::ValidY(m_org.y));
    }

  public:
    KMLParser(BookmarkCategory & cat) : m_category(cat)
    {
      Reset();
    }

    bool Push(string const & name)
    {
      m_tags.push_back(name);
      return true;
    }

    void AddAttr(string const &, string const &) {}

    void Pop(string const & tag)
    {
      ASSERT_EQUAL(m_tags.back(), tag, ());

      if (tag == "Placemark" && IsValid())
      {
        m_category.AddBookmark(Bookmark(m_org, m_name, m_type));
        Reset();
      }
      m_tags.pop_back();
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
      strings::Trim(value);
      if (m_tags.size() > 1 && !value.empty())
      {
        string const & currTag = m_tags.back();
        string const & prevTag = *(m_tags.end() - 2);

        if (currTag == "name")
        {
          if (prevTag == "Placemark")
            m_name = value;
          else if (prevTag == "Document")
            m_category.SetName(value);
        }
        else if (currTag == "coordinates" && prevTag == "Point")
          SetOrigin(value);
        else if (currTag == "visibility" && prevTag == "Document")
          m_category.SetVisible(value == "0" ? false : true);
        else if (currTag == "styleUrl" && prevTag == "Placemark")
          m_type = GetSupportedBMType(value);
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
    "  <Style id=\"placemark-blue\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://www.mapswithme.com/placemarks/placemark-blue.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-brown\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://www.mapswithme.com/placemarks/placemark-brown.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-green\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://www.mapswithme.com/placemarks/placemark-green.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-orange\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://www.mapswithme.com/placemarks/placemark-orange.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-pink\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://www.mapswithme.com/placemarks/placemark-pink.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-purple\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://www.mapswithme.com/placemarks/placemark-purple.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-red\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://www.mapswithme.com/placemarks/placemark-red.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
;

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

  s << "  <name>" << GetName() <<"</name>\n";
  s << "  <visibility>" << (IsVisible() ? "1" : "0") <<"</visibility>\n";

  for (size_t i = 0; i < m_bookmarks.size(); ++i)
  {
    Bookmark const * bm = m_bookmarks[i];
    s << "  <Placemark>\n"
      << "    <name>" << bm->GetName() << "</name>\n"
      << "    <styleUrl>#" << bm->GetType() << "</styleUrl>\n"
      << "    <Point>\n"
      << "      <coordinates>" << PointToString(bm->GetOrg()) << "</coordinates>\n"
      << "    </Point>\n"
      << "  </Placemark>\n";
  }

  s << kmlFooter;
}
