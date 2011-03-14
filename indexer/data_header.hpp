#pragma once

#include "../geometry/rect2d.hpp"

#include "../std/array.hpp"

#include "../base/start_mem_debug.hpp"


class FileReader;
class FileWriter;

namespace feature
{   
  /// All file sizes are in bytes
  class DataHeader
  {
    int64_t m_base;

    pair<int64_t, int64_t> m_bounds;

    array<uint8_t, 4> m_scales;

  public:
    DataHeader();

    /// Zero all fields
    void Reset();

    void SetBase(m2::PointD const & p);
    int64_t GetBase() const { return m_base; }

    m2::RectD const GetBounds() const;
    void SetBounds(m2::RectD const & r);

    void SetScales(int * arr);
    size_t GetScalesCount() const { return m_scales.size(); }
    int GetScale(int i) const { return m_scales[i]; }

    /// @name Serialization
    //@{
    void Save(FileWriter & w) const;
    void Load(FileReader const & r);
    //@}
  };
}

#include "../base/stop_mem_debug.hpp"
