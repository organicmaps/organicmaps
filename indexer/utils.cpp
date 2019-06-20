#include "indexer/utils.hpp"

using namespace std;

namespace indexer
{
MwmSet::MwmHandle FindWorld(DataSource const & dataSource,
                            vector<shared_ptr<MwmInfo>> const & infos)
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
  vector<shared_ptr<MwmInfo>> infos;
  dataSource.GetMwmsInfo(infos);
  return FindWorld(dataSource, infos);
}
}  // namespace indexer
