#pragma once

#include "editor/xml_feature.hpp"

#include "geometry/mercator.hpp"

#include "std/set.hpp"

namespace osm
{
using editor::XMLFeature;

bool LatLonEqual(ms::LatLon const & a, ms::LatLon const & b);

double ScoreLatLon(XMLFeature const & xmlFt, ms::LatLon const & latLon);

double ScoreGeometry(pugi::xml_document const & osmResponse, pugi::xml_node const & way,
                     set<m2::PointD> geometry);

// double ScoreNames(XMLFeature const & xmlFt, StringUtf8Multilang const & names);

// vector<string> GetOsmOriginalTagsForType(feature::Metadata::EType const et);

// double ScoreMetadata(XMLFeature const & xmlFt, feature::Metadata const & metadata);

// bool TypesEqual(XMLFeature const & xmlFt, feature::TypesHolder const & types);

pugi::xml_node GetBestOsmNode(pugi::xml_document const & osmResponse, ms::LatLon const latLon);

pugi::xml_node GetBestOsmWay(pugi::xml_document const & osmResponse, set<m2::PointD> const & geometry);
}  // namespace osm
