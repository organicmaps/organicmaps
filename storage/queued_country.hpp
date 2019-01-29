#pragma once

#include "storage/storage_defines.hpp"

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
  QueuedCountry(CountryId const & m_countryId, MapOptions opt);

  void AddOptions(MapOptions opt);
  void RemoveOptions(MapOptions opt);
  /// In case we can't update file using diff scheme.
  void ResetToDefaultOptions();
  bool SwitchToNextFile();

  void SetFrozen(bool isFrozen) { m_isFrozen = isFrozen; }
  bool IsFrozen() const { return m_isFrozen; }

  inline CountryId const & GetCountryId() const { return m_countryId; }
  inline MapOptions GetInitOptions() const { return m_init; }
  inline MapOptions GetCurrentFileOptions() const { return m_current; }
  inline MapOptions GetDownloadedFilesOptions() const { return UnsetOptions(m_init, m_left); }

  inline bool operator==(CountryId const & countryId) const { return m_countryId == countryId; }

private:
  CountryId m_countryId;
  MapOptions m_init;
  MapOptions m_left;
  MapOptions m_current;
  bool m_isFrozen = false;
};
}  // namespace storage
