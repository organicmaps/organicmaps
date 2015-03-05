#include "features_offsets_table.hpp"

#include "../indexer/data_header.hpp"
#include "../indexer/features_vector.hpp"
#include "../coding/file_writer.hpp"
#include "../coding/internal/file_data.hpp"
#include "../platform/platform.hpp"
#include "../base/assert.hpp"
#include "../base/scope_guard.hpp"
#include "../std/string.hpp"


namespace feature
{
  void FeaturesOffsetsTable::Builder::PushOffset(uint64_t const offset)
  {
    ASSERT(m_offsets.empty() || m_offsets.back() < offset, ());
    m_offsets.push_back(offset);
  }

  FeaturesOffsetsTable::FeaturesOffsetsTable(succinct::elias_fano::elias_fano_builder & builder)
      : m_table(&builder)
  {
  }

  FeaturesOffsetsTable::FeaturesOffsetsTable(string const & fileName)
  {
    m_pSrc = unique_ptr<MmapReader>(new MmapReader(fileName));
    succinct::mapper::map(m_table, reinterpret_cast<char const *>(m_pSrc->Data()));
  }

  // static
  unique_ptr<FeaturesOffsetsTable> FeaturesOffsetsTable::Build(Builder & builder)
  {
    vector<uint64_t> const & offsets = builder.m_offsets;

    uint64_t const numOffsets = offsets.size();
    uint64_t const maxOffset = offsets.empty() ? 0 : offsets.back();

    succinct::elias_fano::elias_fano_builder elias_fano_builder(maxOffset, numOffsets);
    for (uint64_t offset : offsets)
      elias_fano_builder.push_back(offset);

    return unique_ptr<FeaturesOffsetsTable>(new FeaturesOffsetsTable(elias_fano_builder));
  }

  // static
  unique_ptr<FeaturesOffsetsTable> FeaturesOffsetsTable::Load(string const & countryName)
  {
    string const fileName  = GetIndexFileName(countryName);
    uint64_t size;
    if (!GetPlatform().GetFileSizeByFullPath(fileName, size))
      return unique_ptr<FeaturesOffsetsTable>();
    return unique_ptr<FeaturesOffsetsTable>(new FeaturesOffsetsTable(fileName));
  }

  // static
  unique_ptr<FeaturesOffsetsTable> FeaturesOffsetsTable::CreateIfNotExistsAndLoad(
      string const & countryName)
  {
    string const fileName = GetIndexFileName(countryName);
    uint64_t size;
    if (GetPlatform().GetFileSizeByFullPath(fileName,size))
      return Load(countryName);

    string const mwmName = GetPlatform().WritablePathForFile(countryName + DATA_FILE_EXTENSION);
    FilesContainerR cont(mwmName);
    if (!cont.IsExist(HEADER_FILE_TAG))
      return unique_ptr<FeaturesOffsetsTable>();

    DataHeader header;
    header.Load(cont.GetReader(HEADER_FILE_TAG));

    Builder builder;
    FeaturesVector(cont, header).ForEachOffset([&builder] (FeatureType const &, uint32_t offset)
    {
      builder.PushOffset(offset);
    });
    unique_ptr<FeaturesOffsetsTable> table(Build(builder));
    table->Save(countryName);
    return table;
  }

  void FeaturesOffsetsTable::Save(string const & countryName)
  {
    string const fileName = GetIndexFileName(countryName);
    string const fileNameTmp = fileName + EXTENSION_TMP;
    succinct::mapper::freeze(m_table, fileNameTmp.c_str());
    my::RenameFileX(fileNameTmp, fileName);
  }

  uint64_t FeaturesOffsetsTable::GetFeatureOffset(size_t index) const
  {
    ASSERT_LESS(index, size(), ("Index out of bounds", index, size()));
    return m_table.select(index);
  }

  size_t FeaturesOffsetsTable::GetFeatureIndexbyOffset(uint64_t offset) const
  {
    ASSERT_LESS_OR_EQUAL(offset, m_table.select(size() - 1), ("Offset out of bounds", offset,
                                                     m_table.select(size() - 1)));

    //Binary search in elias_fano list
    size_t first = 0, last = size();
    size_t count = last - first, step, current;
    while (count > 0)
    {
      step = count / 2;
      current = first + step;
      if (m_table.select(current) < offset)
      {
        first = ++current;
        count -= step + 1;
      }
      else
        count = step;
    }
    return current;

  }

  string FeaturesOffsetsTable::GetIndexFileName(string const & countryName)
  {
    return GetPlatform().WritablePathForFileIndexes(countryName) + countryName + FEATURES_OFFSETS_TABLE_FILE_EXT;
  }

}  // namespace feature
