#pragma once

#include "coding_params.hpp"

#include "../geometry/rect2d.hpp"

#include "../std/array.hpp"


class ModelReaderPtr;
class FileWriter;

namespace feature
{
  class DataHeader
  {
    serial::CodingParams m_codingParams;

    pair<int64_t, int64_t> m_bounds;

    array<uint8_t, 4> m_scales;

  public:

    void SetCodingParams(serial::CodingParams const & params) { m_codingParams = params; }
    serial::CodingParams const & GetCodingParams() const { return m_codingParams; }

    m2::RectD const GetBounds() const;
    void SetBounds(m2::RectD const & r);

    void SetScales(int * arr);

    inline size_t GetScalesCount() const { return m_scales.size(); }
    inline int GetScale(int i) const { return m_scales[i]; }
    inline int GetLastScale() const { return GetScale(GetScalesCount() - 1); }

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

  private:
    Version m_ver;
  };
}
