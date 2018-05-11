#pragma once

#include "map/user_mark.hpp"
#include "map/user_mark_layer.hpp"

#include "geometry/point2d.hpp"

#include <string>

namespace style
{
// Fixes icons which are not supported by MapsWithMe.
std::string GetSupportedStyle(std::string const & style);
}  // style

class ApiMarkPoint : public UserMark
{
public:
  ApiMarkPoint(m2::PointD const & ptOrg);

  ApiMarkPoint(string const & name, std::string const & id, std::string const & style,
               m2::PointD const & ptOrg);

  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  df::ColorConstant GetColorConstant() const override;

  std::string const & GetName() const { return m_name; }
  void SetName(std::string const & name);

  std::string const & GetApiID() const { return m_id; }
  void SetApiID(std::string const & id);

  void SetStyle(std::string const & style);
  std::string const & GetStyle() const { return m_style; }

private:
  std::string m_name;
  std::string m_id;
  std::string m_style;
};
