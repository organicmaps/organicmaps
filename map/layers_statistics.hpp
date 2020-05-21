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

  explicit LayersStatistics(std::string const & layerName);

  void LogActivate(Status status, std::set<int64_t> const & mwmVersions = {}) const;
  void LogItemSelected(LayerItemType itemType) const;

private:
  std::string m_layerName;
};
