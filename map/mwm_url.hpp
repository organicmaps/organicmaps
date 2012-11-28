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

/// Handles mapswithme://map?params - everything related to displaying info on a map
class ParsedMapApi
{
public:
  ParsedMapApi(Uri const & uri);
  bool IsValid() const;
  vector<ApiPoint> const & GetPoints() const { return m_points; }

private:
  bool Parse(Uri const & uri);
  void AddKeyValue(string const & key, string const & value);

  vector<ApiPoint> m_points;
  /*
  string m_title;
  string m_backTitle;
  string m_backUrl;
  // Calculated from zoom parameter or from m_points
  m2::RectD m_showRect;

  // vector<char> m_iconData;
  */
};

}
