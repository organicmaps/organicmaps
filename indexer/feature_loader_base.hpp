#pragma once
#include "coding_params.hpp"
#include "data_header.hpp"

#include "../coding/file_container.hpp"


class FeatureType;
class ArrayByteSource;

namespace feature
{
  class LoaderBase;

  /// This info is created once.
  class SharedLoadInfo
  {
    FilesContainerR const & m_cont;
    DataHeader const & m_header;

    typedef FilesContainerR::ReaderT ReaderT;

  public:
    SharedLoadInfo(FilesContainerR const & cont, DataHeader const & header);

    ReaderT GetDataReader() const;
    ReaderT GetGeometryReader(int ind) const;
    ReaderT GetTrianglesReader(int ind) const;

    LoaderBase * CreateLoader() const;

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

    inline void AssignFeature(FeatureType * p) { m_pF = p; }

    // It seems like no need to store a copy of buffer (see FeaturesVector).
    typedef char const * BufferT;

    virtual uint8_t GetHeader() = 0;

    virtual void ParseTypes() = 0;
    virtual void ParseCommon() = 0;
    virtual void ParseHeader2() = 0;
    virtual uint32_t ParseGeometry(int scale) = 0;
    virtual uint32_t ParseTriangles(int scale) = 0;

    void Deserialize(BufferT data);

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

    BufferT m_Data;

    static uint32_t const m_TypesOffset = 1;
    uint32_t m_CommonOffset, m_Header2Offset;

    uint32_t m_ptsSimpMask;

    typedef array<uint32_t, 4> offsets_t; // should be synchronized with ARRAY_SIZE(g_arrScales)

    offsets_t m_ptsOffsets, m_trgOffsets;
  };
}
