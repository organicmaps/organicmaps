#include "../geometry/rect2d.hpp"

#include "../std/string.hpp"


namespace url_scheme
{

class Uri;

struct ApiPoint
{
  double m_lat;
  double m_lon;
  string m_name;
  string m_id;
};

class ResultPoint
{
  ApiPoint m_point;
  m2::PointD m_org;

  void Init();

public:
  void MakeFrom(double lat, double lon)
  {
    m_point.m_lat = lat;
    m_point.m_lon = lon;
    Init();
  }
  void MakeFrom(url_scheme::ApiPoint const & pt)
  {
    m_point = pt;
    Init();
  }

  /// Need to fix default name using "dropped_pin" in Framework.
  string & GetName() { return m_point.m_name; }

  m2::PointD const & GetOrg() const { return m_org; }
  string const & GetName() const { return m_point.m_name; }
  ApiPoint const & GetPoint() const { return m_point; }
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
  m2::RectD GetLatLonRect() const { return m_showRect; }
  void Reset();
  bool GoBackOnBalloonClick() const { return m_goBackOnBalloonClick; }

  /// @name Used in settings map viewport after invoking API.
  //@{
  bool GetViewport(m2::PointD & pt, double & zoom) const;
  bool GetViewportRect(m2::RectD & rect) const;
  //@}

private:
  bool Parse(Uri const & uri);
  void AddKeyValue(string key, string const & value);

  vector<ApiPoint> m_points;
  string m_globalBackUrl;
  string m_appTitle;
  int m_version;
  m2::RectD m_showRect;
  /// Zoom level in OSM format (e.g. from 1.0 to 20.0)
  /// Taken into an account when calculating viewport rect, but only if points count is == 1
  double m_zoomLevel;
  bool m_goBackOnBalloonClick;
};

}
