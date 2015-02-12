#pragma once

#include "../coding/file_container.hpp"
#include "../std/stdint.hpp"
#include "../std/unique_ptr.hpp"
#include "../std/vector.hpp"
#include "../3party/succinct/elias_fano.hpp"
#include "../3party/succinct/mapper.hpp"

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
      Builder() = default;
      ~Builder() = default;

      /// Adds offset to the end of the sequence of already
      /// accumulated offsets. Note that offset must be strictly
      /// greater than all previously added offsets.
      ///
      /// \param offset a feature's offset in a MWM file
      void PushOffset(uint64_t offset);

      /// \return number of already accumulated offsets
      inline uint64_t size() const
      {
        return static_cast<uint64_t>(m_offsets.size());
      }

    private:
      friend class FeaturesOffsetsTable;

      vector<uint64_t> m_offsets;
    };

    /// Builds FeaturesOffsetsTable from the strictly increasing
    /// sequence of file offsets.
    ///
    /// \param builder Builder containing sequence of offsets.
    /// \return a pointer to an instance of FeaturesOffsetsTable
    static unique_ptr<FeaturesOffsetsTable> Build(Builder & builder);

    /// Loads FeaturesOffsetsTable from FilesMappingContainer. Note
    /// that some part of a file referenced by container will be
    /// mapped to the memory and used by internal structures of
    /// FeaturesOffsetsTable.
    ///
    /// \param container a container with a section devoted to
    ///                  FeaturesOffsetsTable
    /// \return a pointer to an instance of FeaturesOffsetsTable or nullptr
    ///         when it's not possible to load FeaturesOffsetsTable.
    static unique_ptr<FeaturesOffsetsTable> Load(FilesMappingContainer const & container);

    ~FeaturesOffsetsTable() = default;
    FeaturesOffsetsTable(FeaturesOffsetsTable const &) = delete;
    FeaturesOffsetsTable const & operator=(FeaturesOffsetsTable const &) = delete;

    /// Serializes current instance to a section in container.
    ///
    /// \param container a container current instance will be serialized to
    void Save(FilesContainerW & container);

    /// \param index index of a feature
    /// \return offset a feature
    uint64_t GetFeatureOffset(size_t index) const;

    /// \return number of features offsets in a table.
    inline uint64_t size() const
    {
      return m_table.num_ones();
    }

    /// \return byte size of a table, may be slightly different from a
    ///         real byte size in memory or on disk due to alignment, but
    ///         can be used in benchmarks, logging, etc.
    inline uint64_t byte_size()
    {
      return succinct::mapper::size_of(m_table);
    }

  private:
    FeaturesOffsetsTable(succinct::elias_fano::elias_fano_builder & builder);

    FeaturesOffsetsTable(FilesMappingContainer::Handle && handle);

    succinct::elias_fano m_table;

    FilesMappingContainer::Handle m_handle;
  };
}  // namespace feature
