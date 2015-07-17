#include "indexer/features_offsets_table.hpp"

#include "indexer/data_header.hpp"
#include "indexer/features_vector.hpp"
#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "std/string.hpp"


namespace feature
{
  void FeaturesOffsetsTable::Builder::PushOffset(uint32_t const offset)
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
    m_pReader.reset(new MmapReader(fileName));
    succinct::mapper::map(m_table, reinterpret_cast<char const *>(m_pReader->Data()));
  }

  // static
  unique_ptr<FeaturesOffsetsTable> FeaturesOffsetsTable::Build(Builder & builder)
  {
    vector<uint32_t> const & offsets = builder.m_offsets;

    size_t const numOffsets = offsets.size();
    uint32_t const maxOffset = offsets.empty() ? 0 : offsets.back();

    succinct::elias_fano::elias_fano_builder elias_fano_builder(maxOffset, numOffsets);
    for (uint32_t offset : offsets)
      elias_fano_builder.push_back(offset);

    return unique_ptr<FeaturesOffsetsTable>(new FeaturesOffsetsTable(elias_fano_builder));
  }

  // static
  unique_ptr<FeaturesOffsetsTable> FeaturesOffsetsTable::Load(string const & fileName)
  {
    uint64_t size;
    if (!GetPlatform().GetFileSizeByFullPath(fileName, size))
      return unique_ptr<FeaturesOffsetsTable>();
    return unique_ptr<FeaturesOffsetsTable>(new FeaturesOffsetsTable(fileName));
  }

  // static
  unique_ptr<FeaturesOffsetsTable> FeaturesOffsetsTable::CreateIfNotExistsAndLoad(
      string const & fileName, platform::LocalCountryFile const & localFile)
  {
    uint64_t size;
    if (GetPlatform().GetFileSizeByFullPath(fileName,size))
      return Load(fileName);

    FilesContainerR mwmFileContainer(localFile.GetPath(TMapOptions::EMap));

    LOG(LINFO, ("Features offsets table is absent! Creating a new one."));

    if (!mwmFileContainer.IsExist(HEADER_FILE_TAG))
      return unique_ptr<FeaturesOffsetsTable>();

    DataHeader header;
    header.Load(mwmFileContainer.GetReader(HEADER_FILE_TAG));

    Builder builder;
    FeaturesVector(mwmFileContainer, header).ForEachOffset([&builder] (FeatureType const &, uint32_t offset)
    {
      builder.PushOffset(offset);
    });
    unique_ptr<FeaturesOffsetsTable> table(Build(builder));
    table->Save(fileName);
    return table;
  }

  void FeaturesOffsetsTable::Save(string const & fileName)
  {
    LOG(LINFO, ("Saving features offsets table to ", fileName));
    string const fileNameTmp = fileName + EXTENSION_TMP;
    succinct::mapper::freeze(m_table, fileNameTmp.c_str());
    my::RenameFileX(fileNameTmp, fileName);
  }

  uint32_t FeaturesOffsetsTable::GetFeatureOffset(size_t index) const
  {
    ASSERT_LESS(index, size(), ("Index out of bounds", index, size()));
    return m_table.select(index);
  }

  size_t FeaturesOffsetsTable::GetFeatureIndexbyOffset(uint32_t offset) const
  {
    ASSERT_GREATER(size(), 0, ("We must not ask empty table"));
    ASSERT_LESS_OR_EQUAL(offset, m_table.select(size() - 1), ("Offset out of bounds", offset,
                                                     m_table.select(size() - 1)));
    ASSERT_GREATER_OR_EQUAL(offset, m_table.select(0), ("Offset out of bounds", offset,
                                                        m_table.select(size() - 1)));
    //Binary search in elias_fano list
    size_t leftBound = 0, rightBound = size();
    while (leftBound + 1 < rightBound) {
      size_t middle = leftBound + (rightBound - leftBound) / 2;
      if (m_table.select(middle) <= offset)
        leftBound = middle;
      else
        rightBound = middle;
    }
    ASSERT_EQUAL(offset, m_table.select(leftBound), ("Can't find offset", offset, "in the table"));
    return leftBound;
  }
}  // namespace feature
