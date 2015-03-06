#pragma once

#include "../coding/file_container.hpp"
#include "../coding/mmap_reader.hpp"
#include "../defines.hpp"
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
    /// \param countryName a countryName to save index file to
    /// \return a pointer to an instance of FeaturesOffsetsTable or nullptr
    ///         when it's not possible to load FeaturesOffsetsTable.
    static unique_ptr<FeaturesOffsetsTable> Load(string const & countryName);

    /// Loads FeaturesOffsetsTable from FilesMappingContainer. Note
    /// that some part of a file referenced by container will be
    /// mapped to the memory and used by internal structures of
    /// FeaturesOffsetsTable.
    /// If there is no FeaturesOffsetsTable section in the container,
    /// the function builds it from section devoted to features.
    ///
    /// \warning May take a lot of time if there is no precomputed section
    ///
    /// \param countryName a country to create index to
    /// \return a pointer to an instance of FeaturesOffsetsTable or nullptr
    ///         when it's not possible to create FeaturesOffsetsTable.
    static unique_ptr<FeaturesOffsetsTable> CreateIfNotExistsAndLoad(string const & countryName);

    FeaturesOffsetsTable(FeaturesOffsetsTable const &) = delete;
    FeaturesOffsetsTable const & operator=(FeaturesOffsetsTable const &) = delete;

    /// Serializes current instance to a section in container.
    ///
    /// \param countryName a name of the country to create data
    void Save(string const & countryName);

    /// \param index index of a feature
    /// \return offset a feature
    uint64_t GetFeatureOffset(size_t index) const;

    /// \param offset offset of a feature
    /// \return index of a feature
    size_t GetFeatureIndexbyOffset(uint64_t offset) const;

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

    /// Delete temporary index file (only for features offsets table)
    static void CleanIndexFile(string const & countryName)
    {
      FileWriter::DeleteFileX(GetIndexFileName(countryName));
    }

  private:
    static string GetIndexFileName(string const & countryName);

    FeaturesOffsetsTable(succinct::elias_fano::elias_fano_builder & builder);

    FeaturesOffsetsTable(string const &);

    succinct::elias_fano m_table;

    unique_ptr<MmapReader> m_pSrc;
  };
}  // namespace feature
