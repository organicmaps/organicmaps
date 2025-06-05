#include "indexer/isolines_info.hpp"

#include "indexer/data_source.hpp"

#include "defines.hpp"

namespace isolines
{
bool LoadIsolinesInfo(DataSource const & dataSource, MwmSet::MwmId const & mwmId, IsolinesInfo & info)
{
  auto const handle = dataSource.GetMwmHandleById(mwmId);

  if (!handle.IsAlive())
    return false;

  auto const & value = *handle.GetValue();

  if (!value.m_cont.IsExist(ISOLINES_INFO_FILE_TAG))
    return false;

  auto readerPtr = value.m_cont.GetReader(ISOLINES_INFO_FILE_TAG);
  Deserializer deserializer(info);
  return deserializer.Deserialize(*readerPtr.GetPtr());
}
}  // namespace isolines
