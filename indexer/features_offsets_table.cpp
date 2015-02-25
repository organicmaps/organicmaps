#include "features_offsets_table.hpp"

#include "../indexer/data_header.hpp"
#include "../indexer/features_vector.hpp"
#include "../coding/file_writer.hpp"
#include "../base/assert.hpp"
#include "../base/scope_guard.hpp"
#include "../std/string.hpp"
#include "../defines.hpp"

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

  FeaturesOffsetsTable::FeaturesOffsetsTable(FilesMappingContainer::Handle && handle)
      : m_handle(move(handle))
  {
    succinct::mapper::map(m_table, m_handle.GetData<char>());
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
  unique_ptr<FeaturesOffsetsTable> FeaturesOffsetsTable::Load(
      FilesMappingContainer const & container)
  {
    FilesMappingContainer::Handle handle(container.Map(FEATURES_OFFSETS_TABLE_FILE_TAG));
    if (!handle.IsValid())
      return unique_ptr<FeaturesOffsetsTable>();
    return unique_ptr<FeaturesOffsetsTable>(new FeaturesOffsetsTable(std::move(handle)));
  }

  // static
  unique_ptr<FeaturesOffsetsTable> FeaturesOffsetsTable::CreateIfNotExistsAndLoad(
      FilesMappingContainer const & container)
  {
    if (container.IsExist(FEATURES_OFFSETS_TABLE_FILE_TAG))
      return Load(container);

    if (!container.IsExist(HEADER_FILE_TAG))
      return unique_ptr<FeaturesOffsetsTable>();

    FilesContainerR cont(container.GetName());
    DataHeader header;
    header.Load(cont.GetReader(HEADER_FILE_TAG));

    Builder builder;
    FeaturesVector(cont, header).ForEachOffset([&builder] (FeatureType const &, uint32_t offset)
    {
      builder.PushOffset(offset);
    });
    unique_ptr<FeaturesOffsetsTable> table(Build(builder));
    FilesContainerW writeCont(container.GetName(), FileWriter::OP_WRITE_EXISTING);
    table->Save(writeCont);
    return table;
  }

  void FeaturesOffsetsTable::Save(FilesContainerW & container)
  {
    string const fileName = container.GetFileName() + "." FEATURES_OFFSETS_TABLE_FILE_TAG;
    MY_SCOPE_GUARD(deleteFileGuard, bind(&FileWriter::DeleteFileX, cref(fileName)));
    succinct::mapper::freeze(m_table, fileName.c_str());
    container.Write(fileName, FEATURES_OFFSETS_TABLE_FILE_TAG);
  }

  uint64_t FeaturesOffsetsTable::GetFeatureOffset(size_t index) const
  {
    ASSERT_LESS(index, size(), ("Index out of bounds", index, size()));
    return m_table.select(index);
  }
}  // namespace feature
