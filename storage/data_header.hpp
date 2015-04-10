#pragma once

#include "base/std_serialization.hpp"

#include "storage/defines.hpp"

#include "coding/streams_sink.hpp"

#include "geometry/rect2d.hpp"

#include "std/string.hpp"
#include "std/tuple.hpp"

#include "base/start_mem_debug.hpp"

namespace feature
{   
  /// All file sizes are in bytes
  class DataHeader
  {
  private:
    typedef tuple<
              pair<int64_t, int64_t> // boundary;
            > params_t;
    params_t m_params;

    enum param_t { EBoundary };

    template <int N>
    typename tuple_element<N, params_t>::type const & Get() const { return m_params.get<N>(); }
    template <int N, class T>
    void Set(T const & t) { m_params.get<N>() = t; }

  public:
    DataHeader();

    /// Zeroes all fields
    void Reset();

    m2::RectD const Bounds() const;
    void SetBounds(m2::RectD const & r);

    /// @name Serialization
    //@{
    template <class TWriter> void Save(TWriter & writer) const
    {
      stream::SinkWriterStream<TWriter> w(writer);
      w << MAPS_MAJOR_VERSION_BINARY_FORMAT;
      serial::save_tuple(w, m_params);
    }
    /// @return false if header can't be read (invalid or newer version format)
    template <class TReader> bool Load(TReader & reader)
    {
      stream::SinkReaderStream<TReader> r(reader);

      uint32_t ver;
      r >> ver;
      if (ver > MAPS_MAJOR_VERSION_BINARY_FORMAT)
        return false;
      Reset();
      serial::load_tuple(r, m_params);
      return true;
    }
    //@}
  };

}

#include "base/stop_mem_debug.hpp"
