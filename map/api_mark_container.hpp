#pragma once

#include "user_mark_container.hpp"
#include "api_mark_point.hpp"

class ApiUserMarkContainer : public UserMarkContainer
{
public:
  ApiUserMarkContainer(double layerDepth, Framework & framework);

  virtual Type GetType() const { return API_MARK; }

  virtual string GetActiveTypeName() const;
protected:
  virtual string GetTypeName() const;
  virtual UserMark * AllocateUserMark(m2::PointD const & ptOrg);
};

