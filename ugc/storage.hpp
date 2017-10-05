#pragma once

#include "ugc/types.hpp"

#include "geometry/point2d.hpp"

#include "base/thread_checker.hpp"

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
    m2::PointD m_mercator{};
    uint32_t m_type{};
    uint64_t m_offset{};
    bool m_isDeleted = false;
    bool m_isSynchronized = false;
    std::string m_mwmName;
    std::string m_dataVersion;
    uint32_t m_featureId{};
  };

  explicit Storage(Index const & index);

  UGCUpdate GetUGCUpdate(FeatureID const & id) const;
  void SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc);

  void SaveIndex() const;
  void Load();
  void Defragmentation();
  std::string GetUGCToSend() const;
  void MarkAllAsSynchronized();

  std::unique_ptr<FeatureType> GetOriginalFeature(FeatureID const & id) const;

private:
  Index const & m_index;
  std::vector<UGCIndex> m_UGCIndexes;
  size_t m_numberOfDeleted = 0;
  ThreadChecker m_threadChecker;
};
}  // namespace ugc
