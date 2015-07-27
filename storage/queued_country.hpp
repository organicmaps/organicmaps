#pragma once

#include "storage/index.hpp"
#include "platform/country_defines.hpp"

namespace storage
{
/// Country queued for downloading.
class QueuedCountry
{
public:
  QueuedCountry(TIndex const & index, MapOptions opt);

  void AddOptions(MapOptions opt);
  void RemoveOptions(MapOptions opt);
  bool SwitchToNextFile();

  inline TIndex const & GetIndex() const { return m_index; }
  inline MapOptions GetInitOptions() const { return m_init; }
  inline MapOptions GetCurrentFile() const { return m_current; }
  inline MapOptions GetDownloadedFiles() const { return UnsetOptions(m_init, m_left); }

  inline bool operator==(TIndex const & index) const { return m_index == index; }

private:
  TIndex m_index;
  MapOptions m_init;
  MapOptions m_left;
  MapOptions m_current;
};
}  // namespace storage
