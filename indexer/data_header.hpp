#pragma once

#include "coding/geometry_coding.hpp"

#include "geometry/rect2d.hpp"

#include "base/assert.hpp"
#include "base/buffer_vector.hpp"

#include <string>
#include <utility>

class FilesContainerR;
class FileWriter;
class ModelReaderPtr;

namespace feature
{
class DataHeader
{
public:
  /// @note An order is important here, @see Load function.
  enum class MapType : uint8_t
  {
    World,
    WorldCoasts,
    Country
  };

  /// Max possible geometry scales. @see arrays in feature_impl.hpp
  static constexpr size_t kMaxScalesCount = 4;

  DataHeader() = default;
  explicit DataHeader(std::string const & fileName);
  explicit DataHeader(FilesContainerR const & cont);

  inline void SetGeometryCodingParams(serial::GeometryCodingParams const & cp) { m_codingParams = cp; }

  inline serial::GeometryCodingParams const & GetDefGeometryCodingParams() const { return m_codingParams; }

  serial::GeometryCodingParams GetGeometryCodingParams(int scaleIndex) const;

  m2::RectD GetBounds() const;
  void SetBounds(m2::RectD const & r);

  template <size_t N>
  void SetScales(int const (&arr)[N])
  {
    m_scales.assign(arr, arr + N);
  }

  size_t GetScalesCount() const { return m_scales.size(); }
  int GetScale(size_t i) const { return static_cast<int>(m_scales[i]); }
  int GetLastScale() const { return m_scales.back(); }

  std::pair<int, int> GetScaleRange() const;

  void Save(FileWriter & w) const;
  void Load(FilesContainerR const & cont);

  void SetType(MapType t) { m_type = t; }
  MapType GetType() const { return m_type; }

private:
  void Load(ModelReaderPtr const & r);

  MapType m_type = MapType::World;

  serial::GeometryCodingParams m_codingParams;

  // Rect around region border. Features which cross region border may cross this rect.
  std::pair<int64_t, int64_t> m_bounds;

  buffer_vector<uint8_t, kMaxScalesCount> m_scales;
  // Is not used now (see data/countries_meta.txt). Can be reused for something else.
  buffer_vector<uint8_t, 2> m_langs;
};

std::string DebugPrint(DataHeader::MapType type);
}  // namespace feature
