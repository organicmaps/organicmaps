#pragma once

#include "storage/index.hpp"
#include "platform/country_defines.hpp"

namespace storage
{
/// Country queued for downloading.
class QueuedCountry
{
public:
  QueuedCountry(TIndex const & index, TMapOptions opt);

  void AddOptions(TMapOptions opt);
  void RemoveOptions(TMapOptions opt);
  bool SwitchToNextFile();

  inline TIndex const & GetIndex() const { return m_index; }
  inline TMapOptions GetInitOptions() const { return m_init; }
  inline TMapOptions GetCurrentFile() const { return m_current; }
  inline TMapOptions GetDownloadedFiles() const { return UnsetOptions(m_init, m_left); }

  inline bool operator==(TIndex const & index) const { return m_index == index; }

private:
  TIndex m_index;
  TMapOptions m_init;
  TMapOptions m_left;
  TMapOptions m_current;
};
}  // namespace storage
