#pragma once

#include "ugc/types.hpp"

#include "geometry/point2d.hpp"

#include "base/thread_checker.hpp"
#include "base/visitor.hpp"

#include <memory>
#include <string>
#include <vector>

class Index;
class FeatureType;
struct FeatureID;

namespace ugc
{
class Storage
{
public:
  struct UGCIndex
  {
    DECLARE_VISITOR(visitor.VisitPoint(m_mercator, "x", "y"), visitor(m_type, "type"),
                    visitor(m_matchingType, "matching_type"),
                    visitor(m_offset, "offset"), visitor(m_deleted, "deleted"),
                    visitor(m_synchronized, "synchronized"), visitor(m_mwmName, "mwm_name"),
                    visitor(m_dataVersion, "data_version"), visitor(m_featureId, "feature_id"))

    m2::PointD m_mercator{};
    uint32_t m_type = 0;
    uint32_t m_matchingType = 0;
    uint64_t m_offset = 0;
    bool m_deleted = false;
    bool m_synchronized = false;
    std::string m_mwmName;
    int64_t m_dataVersion;
    uint32_t m_featureId = 0;
  };

  explicit Storage(Index const & index) : m_index(index) {}

  UGCUpdate GetUGCUpdate(FeatureID const & id) const;

  enum class SettingResult
  {
    Success,
    InvalidUGC,
    WritingError
  };

  SettingResult SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc);
  void SaveIndex() const;
  std::string GetUGCToSend() const;
  void MarkAllAsSynchronized();
  void Defragmentation();
  void Load();

  /// Testing
  std::vector<UGCIndex> const & GetIndexesForTesting() const { return m_UGCIndexes; }
  size_t GetNumberOfDeletedForTesting() const { return m_numberOfDeleted; }

private:
  uint64_t UGCSizeAtIndex(size_t const indexPosition) const;
  std::unique_ptr<FeatureType> GetFeature(FeatureID const & id) const;

  Index const & m_index;
  std::vector<UGCIndex> m_UGCIndexes;
  size_t m_numberOfDeleted = 0;
};

inline std::string DebugPrint(Storage::SettingResult const & result)
{
  switch (result)
  {
  case Storage::SettingResult::Success: return "Success";
  case Storage::SettingResult::InvalidUGC: return "Invalid UGC";
  case Storage::SettingResult::WritingError: return "Writing Error";
  }
}
}  // namespace ugc
