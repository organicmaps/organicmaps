#include "map/api_mark_container.hpp"

#include "map/api_mark_point.hpp"
#include "map/framework.hpp"

ApiUserMarkContainer::ApiUserMarkContainer(double layerDepth, Framework & framework)
  : UserMarkContainer(layerDepth, UserMarkType::API_MARK, framework)
{
}

UserMark * ApiUserMarkContainer::AllocateUserMark(const m2::PointD & ptOrg)
{
  return new ApiMarkPoint(ptOrg, this);
}

