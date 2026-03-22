#include "indexer/utils.hpp"

namespace indexer
{
MwmSet::MwmHandle FindWorld(DataSource const & dataSource, std::vector<std::shared_ptr<MwmInfo>> const & infos)
{
  MwmSet::MwmHandle handle;
  for (auto const & info : infos)
  {
    if (info->GetType() == MwmInfo::WORLD)
    {
      handle = dataSource.GetMwmHandleById(MwmSet::MwmId(info));
      break;
    }
  }
  return handle;
}

MwmSet::MwmHandle FindWorld(DataSource const & dataSource)
{
  std::vector<std::shared_ptr<MwmInfo>> infos;
  dataSource.GetMwmsInfo(infos);
  return FindWorld(dataSource, infos);
}
}  // namespace indexer
