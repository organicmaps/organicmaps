#include "routing/routes_builder/data_source_storage.hpp"

#include "base/assert.hpp"

#include <utility>

namespace routing
{
namespace routes_builder
{
void DataSourceStorage::PushDataSource(std::unique_ptr<FrozenDataSource> && ptr)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_freeDataSources.emplace_back(std::move(ptr));
}

std::unique_ptr<FrozenDataSource> DataSourceStorage::GetDataSource()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  CHECK(!m_freeDataSources.empty(), ());
  std::unique_ptr<FrozenDataSource> front = std::move(m_freeDataSources.front());
  m_freeDataSources.pop_front();
  return front;
}
}  // namespace routes_builder
}  // namespace routing
