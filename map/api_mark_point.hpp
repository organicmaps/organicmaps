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

  ApiMarkPoint(string const & name,
           string const & id,
           string const & style,
           m2::PointD const & ptOrg,
           UserMarkContainer * container)
    : StyledPoint(style, ptOrg, container)
    , m_name(name)
    , m_id(id)
  {
  }

  UserMark::Type GetMarkType() const override { return UserMark::Type::API; }

  string const & GetName() const { return m_name; }
  void SetName(string const & name) { m_name = name; }

  string const & GetID() const   { return m_id; }
  void SetID(string const & id)  { m_id = id; }

  unique_ptr<UserMarkCopy> Copy() const override
  {
    return unique_ptr<UserMarkCopy>(
        new UserMarkCopy(new ApiMarkPoint(m_name, m_id, GetStyle(), m_ptOrg, m_container)));
  }

  virtual void FillLogEvent(TEventContainer & details) const override
  {
    UserMark::FillLogEvent(details);
    details["markType"] = "API";
    details["name"] = GetName();
  }

private:
  string m_name;
  string m_id;
};

