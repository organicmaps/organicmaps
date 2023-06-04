#pragma once

#include "indexer/data_source.hpp"

#include <list>
#include <memory>
#include <mutex>

namespace routing
{
namespace routes_builder
{
class DataSourceStorage
{
public:
  void PushDataSource(std::unique_ptr<FrozenDataSource> && ptr);
  std::unique_ptr<FrozenDataSource> GetDataSource();

private:
  std::mutex m_mutex;
  std::list<std::unique_ptr<FrozenDataSource>> m_freeDataSources;
};
}  // namespace routes_builder
}  // namespace routing
