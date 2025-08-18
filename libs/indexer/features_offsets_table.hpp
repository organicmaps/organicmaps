#pragma once

#include "coding/files_container.hpp"
#include "coding/mmap_reader.hpp"

#include "defines.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "3party/succinct/elias_fano.hpp"
#include "3party/succinct/mapper.hpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace platform
{
class LocalCountryFile;
}

namespace feature
{
/// This class is a wrapper around elias-fano encoder, which allows
/// to efficiently encode a sequence of strictly increasing features
/// offsets in a MWM file and access them by feature's index.
class FeaturesOffsetsTable
{
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

  /// Load table by full path to the table file.
  static std::unique_ptr<FeaturesOffsetsTable> Load(std::string const & filePath);

  static std::unique_ptr<FeaturesOffsetsTable> Load(FilesContainerR const & cont, std::string const & tag);
  static void Build(FilesContainerR const & cont, std::string const & storePath);

  FeaturesOffsetsTable(FeaturesOffsetsTable const &) = delete;
  FeaturesOffsetsTable const & operator=(FeaturesOffsetsTable const &) = delete;

  /// Serializes current instance to a section in container.
  ///
  /// \param filePath a full path of the file to store data
  void Save(std::string const & filePath);

  /// \param index index of a feature
  /// \return offset a feature
  uint32_t GetFeatureOffset(size_t index) const;

  /// \param offset offset of a feature
  /// \return index of a feature
  size_t GetFeatureIndexbyOffset(uint32_t offset) const;

  /// \return number of features offsets in a table.
  size_t size() const { return static_cast<size_t>(m_table.num_ones()); }

  /// \return byte size of a table, may be slightly different from a
  ///         real byte size in memory or on disk due to alignment, but
  ///         can be used in benchmarks, logging, etc.
  // size_t byte_size() { return static_cast<size_t>(succinct::mapper::size_of(m_table)); }

private:
  FeaturesOffsetsTable(succinct::elias_fano::elias_fano_builder & builder);
  FeaturesOffsetsTable(std::string const & filePath);
  FeaturesOffsetsTable() = default;

  static std::unique_ptr<FeaturesOffsetsTable> LoadImpl(std::string const & filePath);

  succinct::elias_fano m_table;
  std::unique_ptr<MmapReader> m_pReader;

  ::detail::MappedFile m_file;
  ::detail::MappedFile::Handle m_handle;
};

// Builds feature offsets table in an mwm or rebuilds an existing
// one.
bool BuildOffsetsTable(std::string const & filePath);
}  // namespace feature
