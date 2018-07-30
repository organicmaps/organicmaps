#pragma once

#include "platform/mwm_version.hpp"

#include "coding/geometry_coding.hpp"

#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"

#include <utility>

class FilesContainerR;
class FileWriter;
class ModelReaderPtr;

namespace feature
{
  class DataHeader
  {
  public:
    /// Max possible scales. @see arrays in feature_impl.hpp
    static const size_t MAX_SCALES_COUNT = 4;

  private:
    serial::GeometryCodingParams m_codingParams;

    // Rect around region border. Features which cross region border may cross this rect.
    std::pair<int64_t, int64_t> m_bounds;

    buffer_vector<uint8_t, MAX_SCALES_COUNT> m_scales;
    buffer_vector<uint8_t, 2> m_langs;

  public:
    DataHeader() = default;
    explicit DataHeader(string const & fileName);
    explicit DataHeader(FilesContainerR const & cont);

    inline void SetGeometryCodingParams(serial::GeometryCodingParams const & cp)
    {
      m_codingParams = cp;
    }
    inline serial::GeometryCodingParams const & GetDefGeometryCodingParams() const
    {
      return m_codingParams;
    }
    serial::GeometryCodingParams GetGeometryCodingParams(int scaleIndex) const;

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
    inline int GetScale(size_t i) const { return static_cast<int>(m_scales[i]); }
    inline int GetLastScale() const { return m_scales.back(); }

    std::pair<int, int> GetScaleRange() const;

    inline version::Format GetFormat() const { return m_format; }
    inline bool IsMWMSuitable() const { return m_format <= version::Format::lastFormat; }

    /// @name Serialization
    //@{
    void Save(FileWriter & w) const;
    void Load(FilesContainerR const & cont);

    enum MapType
    {
      world,
      worldcoasts,
      country
    };

    inline void SetType(MapType t) { m_type = t; }
    inline MapType GetType() const { return m_type; }

  private:
    version::Format m_format;
    MapType m_type;

    /// Use lastFormat as a default value for indexes building.
    /// Pass the valid format from wmw in all other cases.
    void Load(ModelReaderPtr const & r, version::Format format);
    void LoadV1(ModelReaderPtr const & r);
    //@}
  };
}
