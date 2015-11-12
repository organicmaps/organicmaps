#pragma once

#include "map/api_mark_point.hpp"
#include "map/user_mark_container.hpp"

class ApiUserMarkContainer : public UserMarkContainer
{
public:
  ApiUserMarkContainer(double layerDepth, Framework & framework);

  // UserMarkContainer overrides:
  Type GetType() const override { return API_MARK; }
  string GetActiveTypeName() const override;

protected:
  // UserMarkContainer overrides:
  string GetTypeName() const override;
  UserMark * AllocateUserMark(m2::PointD const & ptOrg) override;
};
