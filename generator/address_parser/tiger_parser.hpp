#pragma once

#include "indexer/feature_utils.hpp"

#include "geometry/latlon.hpp"

#include "coding/read_write_utils.hpp"

#include <string>
#include <vector>

namespace tiger
{

struct AddressEntry
{
  std::string m_from, m_to, m_street, m_postcode;
  feature::InterpolType m_interpol = feature::InterpolType::None;
  std::vector<ms::LatLon> m_geom;

  template <class TSink> void Save(TSink & sink) const
  {
    rw::Write(sink, m_from);
    rw::Write(sink, m_to);
    rw::Write(sink, m_street);
    rw::Write(sink, m_postcode);

    WriteToSink(sink, static_cast<uint8_t>(m_interpol));
  }

  template <class TSource> void Load(TSource & src)
  {
    rw::Read(src, m_from);
    rw::Read(src, m_to);
    rw::Read(src, m_street);
    rw::Read(src, m_postcode);

    m_interpol = static_cast<feature::InterpolType>(ReadPrimitiveFromSource<uint8_t>(src));
  }
};

void ParseGeometry(std::string_view s, std::vector<ms::LatLon> & geom);
feature::InterpolType ParseInterpolation(std::string_view s);
bool ParseLine(std::string_view line, AddressEntry & e);
} // namespace tiger

namespace feature
{
std::string DebugPrint(InterpolType type);
} // namespace feature
