#include "routing/car_router.hpp"

#include "routing/loaded_path_segment.hpp"
#include "routing/road_graph_router.hpp"
#include "routing/turns_generator.hpp"

#include "platform/mwm_traits.hpp"

namespace routing
{
namespace
{
template <typename ToDo>
bool ForEachCountryInfo(Index & index, ToDo && toDo)
{
  vector<shared_ptr<MwmInfo>> infos;
  index.GetMwmsInfo(infos);

  for (auto const & info : infos)
  {
    if (info->GetType() == MwmInfo::COUNTRY)
    {
      if (!toDo(*info))
        return false;
    }
  }

  return true;
}
}  //  namespace

CarRouter::CarRouter(Index & index, TCountryFileFn const & countryFileFn,
                     unique_ptr<IndexRouter> localRouter)
  : m_index(index), m_router(move(localRouter))
{
}

CarRouter::ResultCode CarRouter::CalculateRoute(m2::PointD const & startPoint,
                                                m2::PointD const & startDirection,
                                                m2::PointD const & finalPoint,
                                                RouterDelegate const & delegate, Route & route)
{
  if (AllMwmsHaveRoutingIndex())
    return m_router->CalculateRoute(startPoint, startDirection, finalPoint, delegate, route);

  return CarRouter::ResultCode::FileTooOld;
}

bool CarRouter::AllMwmsHaveRoutingIndex() const
{
  return ForEachCountryInfo(m_index, [&](MwmInfo const & info) {
    return version::MwmTraits(info.m_version).HasRoutingIndex();
  });
}
}  // namespace routing
