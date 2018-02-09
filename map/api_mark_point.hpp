#pragma once

#include "map/user_mark.hpp"
#include "map/user_mark_layer.hpp"

#include "geometry/point2d.hpp"

#include "std/string.hpp"

namespace style
{

// Fixes icons which are not supported by MapsWithMe.
string GetSupportedStyle(string const & s, string const & context, string const & fallback);
// Default icon.
string GetDefaultStyle();

} // style

class ApiMarkPoint : public UserMark
{
public:
  ApiMarkPoint(m2::PointD const & ptOrg);

  ApiMarkPoint(string const & name, string const & id, string const & style,
               m2::PointD const & ptOrg);

  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  m2::PointD GetPixelOffset() const override;

  string const & GetName() const { return m_name; }
  void SetName(string const & name);

  string const & GetApiID() const { return m_id; }
  void SetApiID(string const & id);

  void SetStyle(string const & style);
  string const & GetStyle() const { return m_style; }

private:
  string m_name;
  string m_id;
  string m_style;
};
