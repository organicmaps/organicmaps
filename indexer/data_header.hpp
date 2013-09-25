#pragma once

#include "coding_params.hpp"

#include "../geometry/rect2d.hpp"

#include "../base/buffer_vector.hpp"


class ModelReaderPtr;
class FileWriter;

namespace feature
{
  class DataHeader
  {
  public:
    /// Max possible scales. @see arrays in feature_impl.hpp
    static const size_t MAX_SCALES_COUNT = 4;

  private:
    serial::CodingParams m_codingParams;

    pair<int64_t, int64_t> m_bounds;

    buffer_vector<uint8_t, MAX_SCALES_COUNT> m_scales;
    buffer_vector<uint8_t, 2> m_langs;

  public:

    inline void SetCodingParams(serial::CodingParams const & cp)
    {
      m_codingParams = cp;
    }
    inline serial::CodingParams const & GetDefCodingParams() const
    {
      return m_codingParams;
    }
    serial::CodingParams GetCodingParams(int scaleIndex) const;

    m2::RectD const GetBounds() const;
    void SetBounds(m2::RectD const & r);

    template <size_t N>
    void SetScales(int const (&arr)[N])
    {
      m_scales.assign(arr, arr + N);
    }

    inline void AddLanguage(int8_t i)
    {
      ASSERT ( i > 0, () );
      m_langs.push_back(static_cast<uint8_t>(i));
    }

    inline size_t GetScalesCount() const { return m_scales.size(); }
    inline int GetScale(int i) const { return static_cast<int>(m_scales[i]); }
    inline int GetLastScale() const { return m_scales.back(); }

    pair<int, int> GetScaleRange() const;

    enum Version
    {
      unknownVersion = -1,
      v1 = 0,     // April 2011
      v2,         // November 2011 (store type index, instead of raw type in mwm)
      v3,         // March 2013 (store type index, instead of raw type in search data)
      lastVersion = v3
    };

    inline Version GetVersion() const { return m_ver; }
    inline bool IsMWMSuitable() const { return (m_ver <= lastVersion); }

    /// @name Serialization
    //@{
    void Save(FileWriter & w) const;

    void Load(ModelReaderPtr const & r, Version ver = unknownVersion);
    void LoadVer1(ModelReaderPtr const & r);
    //@}

    enum MapType
    {
      world,
      worldcoasts,
      country
    };

    inline void SetType(MapType t) { m_type = t; }
    inline MapType GetType() const { return m_type; }

  private:
    Version m_ver;
    MapType m_type;
  };
}
