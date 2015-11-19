#pragma once

#include "map/user_mark_container.hpp"

class ApiUserMarkContainer : public UserMarkContainer
{
public:
  ApiUserMarkContainer(double layerDepth, Framework & framework);

protected:
  UserMark * AllocateUserMark(m2::PointD const & ptOrg) override;
};
