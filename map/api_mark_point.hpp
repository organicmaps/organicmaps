#pragma once

#include "map/user_mark.hpp"
#include "map/styled_point.hpp"

class ApiMarkPoint : public style::StyledPoint
{
public:
  ApiMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
    : StyledPoint(ptOrg, container)
  {
  }

  ApiMarkPoint(string const & name, string const & id, string const & style,
               m2::PointD const & ptOrg, UserMarkContainer * container)
    : StyledPoint(ptOrg, container), m_name(name), m_id(id), m_style(style)
  {
  }

  string const & GetName() const { return m_name; }
  void SetName(string const & name) { m_name = name; }

  string const & GetID() const { return m_id; }
  void SetID(string const & id) { m_id = id; }

  void SetStyle(string const & style) { m_style = style; }

  // StyledPoint overrides:
  string const & GetStyle() const override { return m_style; }

  // UserMark overrides:
  UserMark::Type GetMarkType() const override { return UserMark::Type::API; }

  unique_ptr<UserMarkCopy> Copy() const override
  {
    return make_unique<UserMarkCopy>(
        new ApiMarkPoint(m_name, m_id, GetStyle(), m_ptOrg, m_container));
  }

  void FillLogEvent(TEventContainer & details) const override
  {
    UserMark::FillLogEvent(details);
    details["markType"] = "API";
    details["name"] = GetName();
  }

private:
  string m_name;
  string m_id;
  string m_style;
};
