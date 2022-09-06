#pragma once

#include "indexer/data_header.hpp"

#include "coding/files_container.hpp"
#include "coding/geometry_coding.hpp"

#include "base/macros.hpp"

namespace feature
{
// This info is created once per FeaturesVector.
class SharedLoadInfo
{
public:
  using Reader = FilesContainerR::TReader;

  SharedLoadInfo(FilesContainerR const & cont, DataHeader const & header);

  Reader GetDataReader() const;
  Reader GetGeometryReader(int ind) const;
  Reader GetTrianglesReader(int ind) const;

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
