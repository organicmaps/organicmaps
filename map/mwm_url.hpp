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
  string m_id;
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
  int GetApiVersion() const { return m_version; }
  m2::RectD GetRect() const;
  void Reset();

private:
  bool Parse(Uri const & uri);
  void AddKeyValue(string key, string const & value);

  vector<ApiPoint> m_points;
  string m_globalBackUrl;
  string m_appTitle;
  int m_version;
  m2::RectD m_showRect;
  /// Zoom level in OSM format (e.g. from 1.0 to 20.0)
  /// Taken into an account when calculating GetRect(), but only if points count is == 1
  double m_zoomLevel;
};

}
