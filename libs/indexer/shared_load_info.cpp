#include "indexer/shared_load_info.hpp"

#include "indexer/feature_impl.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/route_relation.hpp"

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
{
  if (m_header.GetType() == DataHeader::MapType::Country)
    m_relsReader = m_cont.GetReaderSafe(RELATIONS_FILE_TAG);
}

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

RelationReader SharedLoadInfo::ReadRelation(uint32_t id) const
{
  ASSERT(m_relsReader.GetPtr(), ());
  return feature::RelationReader(m_relsReader, m_relTable->GetFeatureOffset(id));
}

RouteRelation SharedLoadInfo::GetRelation(uint32_t id) const
{
  ASSERT(m_relsReader.GetPtr(), ());
  ReaderSource src(m_relsReader);
  src.Skip(m_relTable->GetFeatureOffset(id));

  RouteRelation res;
  if (m_version < DatSectionHeader::Version::V2)
    res.RouteRelationBase::Read(src);
  else
    res.Read(src);
  return res;
}

}  // namespace feature
