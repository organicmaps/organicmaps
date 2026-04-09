#pragma once

#include "map/user_mark.hpp"

#include "geometry/point2d.hpp"

#include <string>

namespace style
{
// Fixes icons which are not supported by Organic Maps.
df::ColorConstant GetSupportedStyle(std::string_view style);
}  // namespace style

class ApiMarkPoint : public UserMark
{
public:
  explicit ApiMarkPoint(m2::PointD const & ptOrg);

  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  df::ColorConstant GetColorConstant() const override;

  std::string const & GetName() const { return m_name; }
  void SetName(std::string const & name);

  std::string const & GetApiID() const { return m_id; }
  void SetApiID(std::string const & id);

  void SetStyle(df::ColorConstant style);
  df::ColorConstant GetStyle() const { return m_style; }

private:
  std::string m_name;
  std::string m_id;

  /// @todo Replace ColorConstant with std::string for possible future custom styles.
  df::ColorConstant m_style;
};
