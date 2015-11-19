#pragma once

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

  string GetSymbolName() const override { return "api-result"; }

  UserMark::Type GetMarkType() const override { return UserMark::Type::API; }

  string const & GetName() const { return m_name; }

  void SetName(string const & name) { m_name = name; }

  string const & GetID() const { return m_id; }

  void SetID(string const & id) { m_id = id; }

  unique_ptr<UserMarkCopy> Copy() const override
  {
    return unique_ptr<UserMarkCopy>(
          new UserMarkCopy(new ApiMarkPoint(m_name, m_id, m_style, m_ptOrg, m_container)));
  }

  void FillLogEvent(UserMark::TEventContainer & details) const override
  {
    UserMark::FillLogEvent(details);
    details.emplace("name", GetName());
  }

  void SetStyle(string const & style) { m_style = style; }

  // StyledPoint overrides:
  string const & GetStyle() const override { return m_style; }

private:
  string m_name;
  string m_id;
  string m_style;
};
