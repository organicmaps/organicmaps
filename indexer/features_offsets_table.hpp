#pragma once

#include "coding/mmap_reader.hpp"
#include "defines.hpp"
#include "std/cstdint.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"
#include "3party/succinct/elias_fano.hpp"
#include "3party/succinct/mapper.hpp"

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
      inline size_t size() const { return m_offsets.size(); }

    private:
      friend class FeaturesOffsetsTable;

      vector<uint32_t> m_offsets;
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
    /// \param fileName a full path of the file to load or store data
    /// \return a pointer to an instance of FeaturesOffsetsTable or nullptr
    ///         when it's not possible to load FeaturesOffsetsTable.
    static unique_ptr<FeaturesOffsetsTable> Load(string const & fileName);

    /// Loads FeaturesOffsetsTable from FilesMappingContainer. Note
    /// that some part of a file referenced by container will be
    /// mapped to the memory and used by internal structures of
    /// FeaturesOffsetsTable.
    /// If there is no FeaturesOffsetsTable section in the container,
    /// the function builds it from section devoted to features.
    ///
    /// \warning May take a lot of time if there is no precomputed section
    ///
    /// \param fileName a full path of the file to load or store data
    /// \param localFile Representation of the map files with features data ( uses only if we need to construct them)
    /// \return a pointer to an instance of FeaturesOffsetsTable or nullptr
    ///         when it's not possible to create FeaturesOffsetsTable.
    static unique_ptr<FeaturesOffsetsTable> CreateIfNotExistsAndLoad(string const & fileName, platform::LocalCountryFile const & localFile);

    FeaturesOffsetsTable(FeaturesOffsetsTable const &) = delete;
    FeaturesOffsetsTable const & operator=(FeaturesOffsetsTable const &) = delete;

    /// Serializes current instance to a section in container.
    ///
    /// \param fileName a full path of the file to store data
    void Save(string const & fileName);

    /// \param index index of a feature
    /// \return offset a feature
    uint32_t GetFeatureOffset(size_t index) const;

    /// \param offset offset of a feature
    /// \return index of a feature
    size_t GetFeatureIndexbyOffset(uint32_t offset) const;

    /// \return number of features offsets in a table.
    inline size_t size() const { return static_cast<size_t>(m_table.num_ones()); }

    /// \return byte size of a table, may be slightly different from a
    ///         real byte size in memory or on disk due to alignment, but
    ///         can be used in benchmarks, logging, etc.
    //inline size_t byte_size() { return static_cast<size_t>(succinct::mapper::size_of(m_table)); }

  private:
    FeaturesOffsetsTable(succinct::elias_fano::elias_fano_builder & builder);

    FeaturesOffsetsTable(string const &);

    succinct::elias_fano m_table;

    unique_ptr<MmapReader> m_pReader;
  };
}  // namespace feature
