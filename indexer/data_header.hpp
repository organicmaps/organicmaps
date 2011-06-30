#pragma once

#include "geometry_serialization.hpp"

#include "../geometry/rect2d.hpp"

#include "../std/array.hpp"

#include "../base/start_mem_debug.hpp"


class ModelReaderPtr;
class FileWriter;

namespace feature
{
  /// All file sizes are in bytes
  class DataHeader
  {
    serial::CodingParams m_codingParams;

    pair<int64_t, int64_t> m_bounds;

    array<uint8_t, 4> m_scales;

  public:
    DataHeader();

    /// Zero all fields
    void Reset();

    void SetCodingParams(serial::CodingParams const & params) { m_codingParams = params; }
    serial::CodingParams const & GetCodingParams() const { return m_codingParams; }

    m2::RectD const GetBounds() const;
    void SetBounds(m2::RectD const & r);

    void SetScales(int * arr);
    size_t GetScalesCount() const { return m_scales.size(); }
    int GetScale(int i) const { return m_scales[i]; }
    pair<int, int> GetScaleRange() const;

    /// @name Serialization
    //@{
    void Save(FileWriter & w) const;
    void Load(ModelReaderPtr const & r);
    //@}
  };
}

#include "../base/stop_mem_debug.hpp"
