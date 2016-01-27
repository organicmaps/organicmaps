#include "base/logging.hpp"

#include "editor/osm_feature_matcher.hpp"

namespace osm
{
bool LatLonEqual(ms::LatLon const & a, ms::LatLon const & b)
{
  double constexpr eps = MercatorBounds::GetCellID2PointAbsEpsilon();
  return a.EqualDxDy(b, eps);
}

double ScoreLatLon(XMLFeature const & xmlFt, ms::LatLon const & latLon)
{
  // TODO: Find proper score values;
  return LatLonEqual(latLon, xmlFt.GetCenter()) ? 10 : -10;
}

double ScoreGeometry(pugi::xml_document const & osmResponse, pugi::xml_node const & way,
                     set<m2::PointD> geometry)
{
  int matched = 0;
  int total = 0;
  // TODO(mgsergio): optimize using sorting and scaning eps-squares.
  // The idea is to build squares with side = eps on points from the first set.
  // Sort them by y and x in y groups.
  // Then pass points from the second set through constructed struct and register events
  // like square started, square ended, point encounted.

  for (auto const xNodeRef : way.select_nodes("nd/@ref"))
  {
    ++total;
    string const nodeRef = xNodeRef.attribute().value();
    auto const node = osmResponse.select_node(("node[@ref='" + nodeRef + "']").data()).node();
    if (!node)
    {
      LOG(LDEBUG, ("OSM response have ref", nodeRef,
                   "but have no node with such id.", osmResponse));
      continue;  // TODO: or break and return -1000?
    }

    XMLFeature xmlFt(node);
    for (auto pointIt = begin(geometry); pointIt != end(geometry); ++pointIt)
    {
      if (LatLonEqual(xmlFt.GetCenter(), MercatorBounds::ToLatLon(*pointIt)))
      {
        ++matched;
        geometry.erase(pointIt);
      }
    }
  }

  return static_cast<double>(matched) / total * 100;
}

// double ScoreNames(XMLFeature const & xmlFt, StringUtf8Multilang const & names)
// {
//   double score = 0;
//   names.ForEach([&score, &xmlFt](uint8_t const langCode, string const & name)
//                 {
//                   if (xmlFt.GetName(langCode) == name)
//                     score += 1;
//                   return true;
//                 });

//   // TODO(mgsergio): Deafult name match should have greater wieght. Should it?

//   return score;
// }

// vector<string> GetOsmOriginalTagsForType(feature::Metadata::EType const et)
// {
//   static multimap<feature::Metadata::EType, string> const kFmd2Osm = {
//     {feature::Metadata::EType::FMD_CUISINE, "cuisine"},
//     {feature::Metadata::EType::FMD_OPEN_HOURS, "opening_hours"},
//     {feature::Metadata::EType::FMD_PHONE_NUMBER, "phone"},
//     {feature::Metadata::EType::FMD_PHONE_NUMBER, "contact:phone"},
//     {feature::Metadata::EType::FMD_FAX_NUMBER, "fax"},
//     {feature::Metadata::EType::FMD_FAX_NUMBER, "contact:fax"},
//     {feature::Metadata::EType::FMD_STARS, "stars"},
//     {feature::Metadata::EType::FMD_OPERATOR, "operator"},
//     {feature::Metadata::EType::FMD_URL, "url"},
//     {feature::Metadata::EType::FMD_WEBSITE, "website"},
//     {feature::Metadata::EType::FMD_WEBSITE, "contact:website"},
//     // TODO: {feature::Metadata::EType::FMD_INTERNET, ""},
//     {feature::Metadata::EType::FMD_ELE, "ele"},
//     {feature::Metadata::EType::FMD_TURN_LANES, "turn:lanes"},
//     {feature::Metadata::EType::FMD_TURN_LANES_FORWARD, "turn:lanes:forward"},
//     {feature::Metadata::EType::FMD_TURN_LANES_BACKWARD, "turn:lanes:backward"},
//     {feature::Metadata::EType::FMD_EMAIL, "email"},
//     {feature::Metadata::EType::FMD_EMAIL, "contact:email"},
//     {feature::Metadata::EType::FMD_POSTCODE, "addr:postcode"},
//     {feature::Metadata::EType::FMD_WIKIPEDIA, "wikipedia"},
//     {feature::Metadata::EType::FMD_MAXSPEED, "maxspeed"},
//     {feature::Metadata::EType::FMD_FLATS, "addr:flats"},
//     {feature::Metadata::EType::FMD_HEIGHT, "height"},
//     {feature::Metadata::EType::FMD_MIN_HEIGHT, "min_height"},
//     {feature::Metadata::EType::FMD_MIN_HEIGHT, "building:min_level"},
//     {feature::Metadata::EType::FMD_DENOMINATION, "denomination"},
//     {feature::Metadata::EType::FMD_BUILDING_LEVELS, "building:levels"},
//   };

//   vector<string> result;
//   auto const range = kFmd2Osm.equal_range(et);
//   for (auto it = range.first; it != range.second; ++it)
//     result.emplace_back(it->second);

//   return result;
// }

// double ScoreMetadata(XMLFeature const & xmlFt, feature::Metadata const & metadata)
// {
//   double score = 0;

//   for (auto const type : metadata.GetPresentTypes())
//   {
//     for (auto const osm_tag : GetOsmOriginalTagsForType(static_cast<feature::Metadata::EType>(type)))
//     {
//       if (xmlFt.GetTagValue(osm_tag) == metadata.Get(type))
//       {
//         score += 1;
//         break;
//       }
//     }
//   }

//   return score;
// }

// bool TypesEqual(XMLFeature const & xmlFt, feature::TypesHolder const & types)
// {
//   // TODO: Use mapcss-mapping.csv to correctly match types.
//   return true;
// }

pugi::xml_node GetBestOsmNode(pugi::xml_document const & osmResponse, ms::LatLon const latLon)
{
  double bestScore = -1;
  pugi::xml_node bestMatchNode;

  for (auto const & xNode : osmResponse.select_nodes("osm/node"))
  {
    try
    {
      XMLFeature xmlFt(xNode.node());

      // if (!TypesEqual(xmlFt, feature::TypesHolder(ft)))
      //   continue;

      double nodeScore = ScoreLatLon(xmlFt, latLon);
      if (nodeScore < 0)
        continue;

      // nodeScore += ScoreNames(xmlFt, ft.GetNames());
      // nodeScore += ScoreMetadata(xmlFt, ft.GetMetadata());

      if (bestScore < nodeScore)
      {
        bestScore = nodeScore;
        bestMatchNode = xNode.node();
      }
    }
    catch (editor::XMLFeatureNoLatLonError const & ex)
    {
      LOG(LWARNING, ("No lat/lon attribute in osm response node.", ex.Msg()));
      continue;
    }
  }

  // TODO(mgsergio): Add a properly defined threshold.
  // if (bestScore < minimumScoreThreshold)
  //   return pugi::xml_node;

  return bestMatchNode;
}

pugi::xml_node GetBestOsmWay(pugi::xml_document const & osmResponse, set<m2::PointD> const & geometry)
{
  double bestScore = -1;
  pugi::xml_node bestMatchWay;

  for (auto const & xWay : osmResponse.select_nodes("osm/way"))
  {
    try
    {
      XMLFeature xmlFt(xWay.node());

      // if (!TypesEqual(xmlFt, feature::TypesHolder(ft)))
      //   continue;

      double nodeScore = ScoreGeometry(osmResponse, xWay.node(), geometry);
      if (nodeScore < 0)
        continue;

      // nodeScore += ScoreNames(xmlFt, ft.GetNames());
      // nodeScore += ScoreMetadata(xmlFt, ft.GetMetadata());

      if (bestScore < nodeScore)
      {
        bestScore = nodeScore;
        bestMatchWay = xWay.node();
      }
    }
    catch (editor::XMLFeatureNoLatLonError const & ex)
    {
      LOG(LWARNING, ("No lat/lon attribute in osm response node.", ex.Msg()));
      continue;
    }
  }

  // TODO(mgsergio): Add a properly defined threshold.
  // if (bestScore < minimumScoreThreshold)
  //   return pugi::xml_node;

  return bestMatchWay;
}

}  // namespace osm
