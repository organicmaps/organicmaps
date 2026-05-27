#pragma once

#include "coding/files_container.hpp"

#include "3party/succinct/elias_fano.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace feature
{
/// This class is a wrapper around elias-fano encoder, which allows
/// to efficiently encode a sequence of strictly increasing features
/// offsets in a MWM file and access them by feature's index.
class FeaturesOffsetsTable
{
  DISALLOW_COPY_AND_MOVE(FeaturesOffsetsTable);

public:
  /// This class is used to accumulate strictly increasing features
  /// offsets and then build FeaturesOffsetsTable.
  class Builder
  {
  public:
    /// Adds offset to the end of the sequence of already
    /// accumulated offsets. Note that offset must be strictly
    /// greater than all previously added offsets.
    ///
    /// \param offset a feature's offset in a MWM file
    void PushOffset(uint32_t offset);

    /// \return number of already accumulated offsets
    size_t size() const { return m_offsets.size(); }

  private:
    friend class FeaturesOffsetsTable;

    std::vector<uint32_t> m_offsets;
  };

  /// Builds FeaturesOffsetsTable from the strictly increasing
  /// sequence of file offsets.
  ///
  /// \param builder Builder containing sequence of offsets.
  /// \return a pointer to an instance of FeaturesOffsetsTable
  static std::unique_ptr<FeaturesOffsetsTable> Build(Builder & builder);

  static std::unique_ptr<FeaturesOffsetsTable> Load(FilesContainerR const & cont, std::string const & tag);
  static void Build(FilesContainerR const & cont, std::string const & storePath);

  /// Serializes current instance to a section in container.
  ///
  /// \param filePath a full path of the file to store data
  void Save(std::string const & filePath);

  /// \param index index of a feature
  /// \return offset a feature
  uint32_t GetFeatureOffset(uint32_t index) const;

  /// \param offset offset of a feature
  /// \return index of a feature
  /// Calls BinarySearch and CHECK.
  uint32_t GetFeatureIndexbyOffset(uint32_t offset) const;

  /// Non-asserting binary search over the strictly increasing values.
  /// \param value the value (usually offset in file or some ID) to look up
  /// \return index where @p value is stored, or std::nullopt if not present.
  std::optional<uint32_t> BinarySearch(uint32_t value) const;

  /// \return number of features offsets in a table.
  size_t size() const { return static_cast<size_t>(m_table.num_ones()); }

private:
  FeaturesOffsetsTable(succinct::elias_fano::elias_fano_builder & builder);
  FeaturesOffsetsTable() = default;

  succinct::elias_fano m_table;
  std::unique_ptr<MemoryRegion> m_memRegion;
};

// Builds feature offsets table in an mwm or rebuilds an existing
// one.
bool BuildOffsetsTable(std::string const & filePath);
}  // namespace feature
