#pragma once

#include "storage/index.hpp"
#include "platform/country_defines.hpp"

namespace storage
{
/// Country queued for downloading.
/// @TODO(bykoianko) This class assumes that a map may consist of one or two mwm files.
/// But single mwm files are used now. So this class should be redisigned to support
/// only single mwm case.
class QueuedCountry
{
public:
  QueuedCountry(TCountryId const & m_countryId, MapOptions opt);

  void AddOptions(MapOptions opt);
  void RemoveOptions(MapOptions opt);
  /// In case we can't update file using diff scheme.
  void ResetToDefaultOptions();
  bool SwitchToNextFile();

  inline TCountryId const & GetCountryId() const { return m_countryId; }
  inline MapOptions GetInitOptions() const { return m_init; }
  inline MapOptions GetCurrentFileOptions() const { return m_current; }
  inline MapOptions GetDownloadedFilesOptions() const { return UnsetOptions(m_init, m_left); }

  inline bool operator==(TCountryId const & countryId) const { return m_countryId == countryId; }

private:
  TCountryId m_countryId;
  MapOptions m_init;
  MapOptions m_left;
  MapOptions m_current;
};
}  // namespace storage
