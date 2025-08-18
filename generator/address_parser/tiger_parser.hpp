#pragma once

#include "generator/address_enricher.hpp"

#include "geometry/latlon.hpp"

#include <string>
#include <vector>

namespace tiger
{

struct AddressEntry : public generator::AddressEnricher::RawEntryBase
{
  std::vector<ms::LatLon> m_geom;
};

void ParseGeometry(std::string_view s, std::vector<ms::LatLon> & geom);
feature::InterpolType ParseInterpolation(std::string_view s);
bool ParseLine(std::string_view line, AddressEntry & e);
}  // namespace tiger

namespace feature
{
std::string DebugPrint(InterpolType type);
}  // namespace feature
