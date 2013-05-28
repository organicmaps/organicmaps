#include "../geometry/rect2d.hpp"
#include "../std/string.hpp"

namespace url_scheme
{

class Uri;

struct ApiPoint
{
  double m_lat;
  double m_lon;
  string m_title;
  string m_url;
};

/// Handles [mapswithme|mwm]://map?params - everything related to displaying info on a map
class ParsedMapApi
{
public:
  ParsedMapApi(Uri const & uri);
  ParsedMapApi();
  bool SetUriAndParse(string const & url);
  bool IsValid() const;
  vector<ApiPoint> const & GetPoints() const { return m_points; }
  string const & GetGlobalBackUrl() const { return m_globalBackUrl; }
  string const & GetAppTitle() const { return m_appTitle; }
  int GetApiversion() const { return m_id; }
  m2::RectD GetRect() const { return m_showRect; }
  void Clear();

private:
  bool Parse(Uri const & uri);
  void AddKeyValue(string const & key, string const & value);

  vector<ApiPoint> m_points;
  string m_globalBackUrl;
  string m_appTitle;
  int m_id;
  //Lon Lat coordinates
  m2::RectD m_showRect;
};

}
