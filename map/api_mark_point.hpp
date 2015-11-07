#pragma once

#include "map/user_mark.hpp"
#include "map/styled_point.hpp"

class ApiMarkPoint : public StyledPoint
{
public:
  ApiMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container)
    : StyledPoint(ptOrg, container)
  {
  }

  ApiMarkPoint(string const & name, string const & id, string const & style,
               m2::PointD const & ptOrg, UserMarkContainer * container)
    : StyledPoint(style, ptOrg, container), m_name(name), m_id(id)
  {
  }

  string const & GetName() const { return m_name; }
  void SetName(string const & name) { m_name = name; }

  string const & GetID() const { return m_id; }
  void SetID(string const & id) { m_id = id; }

  // UserMark overrides:
  UserMark::Type GetMarkType() const override { return UserMark::Type::API; }

  unique_ptr<UserMarkCopy> Copy() const override
  {
    return make_unique<UserMarkCopy>(new ApiMarkPoint(m_name, m_id, GetStyle(), m_ptOrg, m_container));
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
};
