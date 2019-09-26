#pragma once

#include "ugc/types.hpp"

#include "geometry/point2d.hpp"

#include "base/thread_checker.hpp"

#include <memory>
#include <string>

class DataSource;
class FeatureType;
struct FeatureID;

namespace feature
{
class TypesHolder;
}

namespace ugc
{
class Storage
{
public:
  explicit Storage(DataSource const & dataSource) : m_dataSource(dataSource) {}

  static std::string GetUGCFilePath();
  static std::string GetIndexFilePath();

  UGCUpdate GetUGCUpdate(FeatureID const & id) const;

  enum class SettingResult
  {
    Success,
    InvalidUGC,
    WritingError
  };

  SettingResult SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc);
  std::string GetUGCToSend() const;
  void MarkAllAsSynchronized();
  void Defragmentation();
  void Load();
  size_t GetNumberOfUnsynchronized() const;
  bool HasUGCForPlace(uint32_t bestType, m2::PointD const & point) const;
  void Validate() const;

  /// Testing
  UpdateIndexes & GetIndexesForTesting() { return m_indexes; }
  size_t GetNumberOfDeletedForTesting() const { return m_numberOfDeleted; }
  SettingResult SetUGCUpdateForTesting(FeatureID const & id, v0::UGCUpdate const & ugc);
  void LoadForTesting(std::string const & testIndexFilePath);
  bool SaveIndexForTesting(std::string const & testIndexFilePath = "") const;

private:
  void DefragmentationImpl(bool force);
  uint64_t UGCSizeAtPos(size_t const pos) const;
  std::unique_ptr<FeatureType> GetFeature(FeatureID const & id) const;
  void Migrate(std::string const & indexFilePath);
  UpdateIndexes::const_iterator FindIndex(FeatureID const & id) const;
  UpdateIndexes::const_iterator FindIndex(uint32_t bestType, m2::PointD const & point) const;

  DataSource const & m_dataSource;
  UpdateIndexes m_indexes;
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
  UNREACHABLE();
}
}  // namespace ugc

namespace lightweight
{
namespace impl
{
size_t GetNumberOfUnsentUGC();
}  // namespace impl
}  //namespace lightweight
