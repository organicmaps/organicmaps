#include "indexer/shared_load_info.hpp"

#include "indexer/feature_impl.hpp"

#include "defines.hpp"

namespace feature
{
SharedLoadInfo::SharedLoadInfo(FilesContainerR const & cont, DataHeader const & header)
  : m_cont(cont), m_header(header)
{
  CHECK_NOT_EQUAL(m_header.GetFormat(), version::Format::v1, ("Old maps format is not supported"));
}

SharedLoadInfo::Reader SharedLoadInfo::GetDataReader() const
{
  if (GetMWMFormat() < version::Format::v10)
    return m_cont.GetReader(FEATURES_FILE_TAG_V1_V9);
  return m_cont.GetReader(FEATURES_FILE_TAG);
}

SharedLoadInfo::Reader SharedLoadInfo::GetMetadataReader() const
{
  return m_cont.GetReader(METADATA_FILE_TAG);
}

SharedLoadInfo::Reader SharedLoadInfo::GetMetadataIndexReader() const
{
  return m_cont.GetReader(METADATA_INDEX_FILE_TAG);
}

SharedLoadInfo::Reader SharedLoadInfo::GetAltitudeReader() const
{
  return m_cont.GetReader(ALTITUDES_FILE_TAG);
}

SharedLoadInfo::Reader SharedLoadInfo::GetGeometryReader(int ind) const
{
  return m_cont.GetReader(GetTagForIndex(GEOMETRY_FILE_TAG, ind));
}

SharedLoadInfo::Reader SharedLoadInfo::GetTrianglesReader(int ind) const
{
  return m_cont.GetReader(GetTagForIndex(TRIANGLE_FILE_TAG, ind));
}

std::optional<SharedLoadInfo::Reader> SharedLoadInfo::GetPostcodesReader() const
{
  if (!m_cont.IsExist(POSTCODES_FILE_TAG))
    return {};

  return m_cont.GetReader(POSTCODES_FILE_TAG);
}
}  // namespace feature
