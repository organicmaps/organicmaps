#pragma once

#include "../3party/succinct/elias_fano.hpp"
#include "../3party/succinct/mapper.hpp"
#include "../coding/file_container.hpp"
#include "../std/stdint.hpp"
#include "../std/unique_ptr.hpp"
#include "../std/vector.hpp"

namespace feature
{
  class FeaturesOffsetsTable
  {
  public:
    class Builder
    {
    public:
      Builder();

      ~Builder();

      void PushOffset(uint64_t offset);

      inline uint64_t size() const
      {
        return static_cast<uint64_t>(m_offsets.size());
      }

    private:
      friend class FeaturesOffsetsTable;

      vector<uint64_t> m_offsets;
    };

    static unique_ptr<FeaturesOffsetsTable> Build(Builder & builder);

    static unique_ptr<FeaturesOffsetsTable> Load(
        FilesMappingContainer const & container);

    ~FeaturesOffsetsTable();

    FeaturesOffsetsTable(FeaturesOffsetsTable const &) = delete;
    FeaturesOffsetsTable const & operator=(FeaturesOffsetsTable const &) =
        delete;

    void Save(FilesContainerW & container);

    uint64_t GetFeatureOffset(size_t index) const;

    inline uint64_t size() const
    {
      return m_table.num_ones();
    }

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
