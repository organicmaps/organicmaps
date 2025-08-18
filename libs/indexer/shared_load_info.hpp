#pragma once

#include "indexer/dat_section_header.hpp"
#include "indexer/data_header.hpp"
#include "indexer/route_relation.hpp"

#include "coding/files_container.hpp"
#include "coding/geometry_coding.hpp"

#include "base/macros.hpp"

namespace indexer
{
class MetadataDeserializer;
}

namespace feature
{
class FeaturesOffsetsTable;

// This info is created once per FeaturesVector.
class SharedLoadInfo
{
public:
  using Reader = FilesContainerR::TReader;

  SharedLoadInfo(FilesContainerR const & cont, DataHeader const & header,
                 feature::FeaturesOffsetsTable const * relTable, indexer::MetadataDeserializer * metaDeserializer);

  Reader GetDataReader() const;
  Reader GetGeometryReader(size_t ind) const;
  Reader GetTrianglesReader(size_t ind) const;

  RouteRelationBase ReadRelation(uint32_t id) const;

  serial::GeometryCodingParams const & GetDefGeometryCodingParams() const
  {
    return m_header.GetDefGeometryCodingParams();
  }

  serial::GeometryCodingParams GetGeometryCodingParams(int scaleIndex) const
  {
    return m_header.GetGeometryCodingParams(scaleIndex);
  }

  int GetScalesCount() const { return static_cast<int>(m_header.GetScalesCount()); }
  int GetScale(int i) const { return m_header.GetScale(i); }
  int GetLastScale() const { return m_header.GetLastScale(); }

private:
  FilesContainerR const & m_cont;
  DataHeader const & m_header;

public:
  feature::FeaturesOffsetsTable const * m_relTable;
  indexer::MetadataDeserializer * m_metaDeserializer;
  feature::DatSectionHeader::Version m_version;

  DISALLOW_COPY_AND_MOVE(SharedLoadInfo);
};
}  // namespace feature
