#pragma once
#include "indexer/coding_params.hpp"
#include "indexer/data_header.hpp"

#include "coding/file_container.hpp"

#include "std/noncopyable.hpp"


class FeatureType;
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

    inline version::Format GetMWMFormat() const { return m_header.GetFormat(); }

    inline serial::CodingParams const & GetDefCodingParams() const
    {
      return m_header.GetDefCodingParams();
    }
    inline serial::CodingParams GetCodingParams(int scaleIndex) const
    {
      return m_header.GetCodingParams(scaleIndex);
    }

    inline int GetScalesCount() const { return static_cast<int>(m_header.GetScalesCount()); }
    inline int GetScale(int i) const { return m_header.GetScale(i); }
    inline int GetLastScale() const { return m_header.GetLastScale(); }
  };

  class LoaderBase
  {
  public:
    LoaderBase(SharedLoadInfo const & info);
    virtual ~LoaderBase() {}

    // It seems like no need to store a copy of buffer (see FeaturesVector).
    using TBuffer = char const * ;

    /// @name Initialize functions.
    //@{
    void Init(TBuffer data);
    inline void InitFeature(FeatureType * p) { m_pF = p; }

    void ResetGeometry();
    //@}

    virtual uint8_t GetHeader() = 0;

    virtual void ParseTypes() = 0;
    virtual void ParseCommon() = 0;
    virtual void ParseHeader2() = 0;
    virtual uint32_t ParseGeometry(int scale) = 0;
    virtual uint32_t ParseTriangles(int scale) = 0;
    virtual void ParseMetadata() = 0;

    inline uint32_t GetTypesSize() const { return m_CommonOffset - m_TypesOffset; }

  protected:
    inline char const * DataPtr() const { return m_Data; }

    uint32_t CalcOffset(ArrayByteSource const & source) const;

    inline serial::CodingParams const & GetDefCodingParams() const
    {
      return m_Info.GetDefCodingParams();
    }
    inline serial::CodingParams GetCodingParams(int scaleIndex) const
    {
      return m_Info.GetCodingParams(scaleIndex);
    }

    uint8_t Header() const { return static_cast<uint8_t>(*DataPtr()); }

  protected:
    SharedLoadInfo const & m_Info;
    FeatureType * m_pF;

    TBuffer m_Data;

    static uint32_t const m_TypesOffset = 1;
    uint32_t m_CommonOffset, m_Header2Offset;

    uint32_t m_ptsSimpMask;

    typedef buffer_vector<uint32_t, DataHeader::MAX_SCALES_COUNT> offsets_t;
    offsets_t m_ptsOffsets, m_trgOffsets;

    static uint32_t const s_InvalidOffset = uint32_t(-1);

    void ReadOffsets(ArrayByteSource & src, uint8_t mask, offsets_t & offsets) const;
  };
}
