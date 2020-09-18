#pragma once

#include <cstdint>
#include <set>
#include <string>

class LayersStatistics
{
public:
  enum class Status
  {
    Success,
    Error,
    Unavailable,
  };

  enum class LayerItemType
  {
    Point,
    Cluster,
  };

  enum class InitType
  {
    User,
    Auto,
  };

  explicit LayersStatistics(std::string const & layerName);

  void LogActivate(Status status, std::set<int64_t> const & mwmVersions = {},
                   InitType initType = InitType::User) const;
  void LogItemSelected(LayerItemType itemType) const;

private:
  std::string m_layerName;
};
