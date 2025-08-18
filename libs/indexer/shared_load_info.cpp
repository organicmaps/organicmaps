#include "indexer/shared_load_info.hpp"

#include "indexer/feature_impl.hpp"
#include "indexer/features_offsets_table.hpp"

#include "defines.hpp"

namespace feature
{
SharedLoadInfo::SharedLoadInfo(FilesContainerR const & cont, DataHeader const & header,
                               feature::FeaturesOffsetsTable const * relTable,
                               indexer::MetadataDeserializer * metaDeserializer)
  : m_cont(cont)
  , m_header(header)
  , m_relTable(relTable)
  , m_metaDeserializer(metaDeserializer)
{}

SharedLoadInfo::Reader SharedLoadInfo::GetDataReader() const
{
  return m_cont.GetReader(FEATURES_FILE_TAG);
}

SharedLoadInfo::Reader SharedLoadInfo::GetGeometryReader(size_t ind) const
{
  return m_cont.GetReader(GetTagForIndex(GEOMETRY_FILE_TAG, ind));
}

SharedLoadInfo::Reader SharedLoadInfo::GetTrianglesReader(size_t ind) const
{
  return m_cont.GetReader(GetTagForIndex(TRIANGLE_FILE_TAG, ind));
}

RouteRelationBase SharedLoadInfo::ReadRelation(uint32_t id) const
{
  auto reader = m_cont.GetReader(RELATIONS_FILE_TAG);
  ReaderSource src(reader);
  src.Skip(m_relTable->GetFeatureOffset(id));

  RouteRelationBase res;
  res.Read(src);
  return res;
}

}  // namespace feature
