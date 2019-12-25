#pragma once

#include "indexer/data_header.hpp"

#include "coding/files_container.hpp"
#include "coding/geometry_coding.hpp"

#include "base/macros.hpp"

#include <optional>

namespace feature
{
// This info is created once per FeaturesVector.
class SharedLoadInfo
{
public:
  using Reader = FilesContainerR::TReader;

  SharedLoadInfo(FilesContainerR const & cont, DataHeader const & header);
  ~SharedLoadInfo() = default;

  Reader GetDataReader() const;
  Reader GetMetadataReader() const;
  Reader GetMetadataIndexReader() const;
  Reader GetAltitudeReader() const;
  Reader GetGeometryReader(int ind) const;
  Reader GetTrianglesReader(int ind) const;
  std::optional<Reader> GetPostcodesReader() const;

  version::Format GetMWMFormat() const { return m_header.GetFormat(); }

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

  DISALLOW_COPY_AND_MOVE(SharedLoadInfo);
};
}  // namespace feature
