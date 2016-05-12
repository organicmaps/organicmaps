#pragma once

#include "map/user_mark.hpp"
#include "map/user_mark_container.hpp"

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
  ApiMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container);

  ApiMarkPoint(string const & name, string const & id, string const & style,
               m2::PointD const & ptOrg, UserMarkContainer * container);

  string GetSymbolName() const override;
  UserMark::Type GetMarkType() const override;
  m2::PointD const & GetPixelOffset() const override;

  string const & GetName() const { return m_name; }
  void SetName(string const & name) { m_name = name; }

  string const & GetID() const { return m_id; }
  void SetID(string const & id) { m_id = id; }

  void SetStyle(string const & style) { m_style = style; }
  string const & GetStyle() const { return m_style; }

private:
  string m_name;
  string m_id;
  string m_style;
};
