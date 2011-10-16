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

    inline size_t GetScalesCount() const { return m_scales.size(); }
    inline int GetScale(int i) const { return static_cast<int>(m_scales[i]); }
    inline int GetLastScale() const { return m_scales.back(); }

    pair<int, int> GetScaleRange() const;

    /// @name Serialization
    //@{
    void Save(FileWriter & w) const;
    void Load(ModelReaderPtr const & r);

    void LoadVer1(ModelReaderPtr const & r);
    //@}

    enum Version {
      v1,     // April 2011
      v2      // September 2011
    };
    inline Version GetVersion() const { return m_ver; }

    enum MapType {
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
