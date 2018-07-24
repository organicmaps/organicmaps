#pragma once

#include "indexer/data_header.hpp"
#include "indexer/feature.hpp"

#include "coding/file_container.hpp"
#include "coding/geometry_coding.hpp"

#include "std/noncopyable.hpp"

class ArrayByteSource;

namespace feature
{
  class LoaderBase;

  /// This info is created once.
  class SharedLoadInfo : private noncopyable
  {
    FilesContainerR const & m_cont;
    DataHeader const & m_header;

    using TReader = FilesContainerR::TReader;

    LoaderBase * m_loader;
    void CreateLoader();

  public:
    SharedLoadInfo(FilesContainerR const & cont, DataHeader const & header);
    ~SharedLoadInfo();

    TReader GetDataReader() const;
    TReader GetMetadataReader() const;
    TReader GetMetadataIndexReader() const;
    TReader GetAltitudeReader() const;
    TReader GetGeometryReader(int ind) const;
    TReader GetTrianglesReader(int ind) const;

    LoaderBase * GetLoader() const { return m_loader; }

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
  };

  class LoaderBase
  {
  public:
    LoaderBase(SharedLoadInfo const & info);
    virtual ~LoaderBase() = default;

    // It seems like no need to store a copy of buffer (see FeaturesVector).
    using Buffer = char const *;

    virtual uint8_t GetHeader(FeatureType const & ft) const = 0;

    virtual void ParseTypes(FeatureType & ft) const = 0;
    virtual void ParseCommon(FeatureType & ft) const = 0;
    virtual void ParseHeader2(FeatureType & ft) const = 0;
    virtual uint32_t ParseGeometry(int scale, FeatureType & ft) const = 0;
    virtual uint32_t ParseTriangles(int scale, FeatureType & ft) const = 0;
    virtual void ParseMetadata(FeatureType & ft) const = 0;

    uint32_t GetTypesSize(FeatureType const & ft) const;

  protected:
    uint32_t CalcOffset(ArrayByteSource const & source, Buffer const data) const;

    serial::GeometryCodingParams const & GetDefGeometryCodingParams() const
    {
      return m_Info.GetDefGeometryCodingParams();
    }
    serial::GeometryCodingParams GetGeometryCodingParams(int scaleIndex) const
    {
      return m_Info.GetGeometryCodingParams(scaleIndex);
    }

    uint8_t Header(Buffer const data) const { return static_cast<uint8_t>(*data); }

    void ReadOffsets(ArrayByteSource & src, uint8_t mask,
                     FeatureType::GeometryOffsets & offsets) const;

    SharedLoadInfo const & m_Info;
    static uint32_t const s_InvalidOffset = uint32_t(-1);
  };
}
