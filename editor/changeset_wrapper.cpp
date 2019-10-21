#include "editor/changeset_wrapper.hpp"
#include "editor/feature_matcher.hpp"

#include "indexer/feature.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <random>
#include <sstream>
#include <utility>

#include "private.h"

using namespace std;

using editor::XMLFeature;

namespace
{
m2::RectD GetBoundingRect(vector<m2::PointD> const & geometry)
{
  m2::RectD rect;
  for (auto const & p : geometry)
  {
    auto const latLon = mercator::ToLatLon(p);
    rect.Add({latLon.m_lon, latLon.m_lat});
  }
  return rect;
}

bool OsmFeatureHasTags(pugi::xml_node const & osmFt)
{
  return osmFt.child("tag");
}

string const static kVowels = "aeiouy";

vector<string> const static kMainTags = {"amenity",   "shop",    "tourism", "historic", "craft",
                                         "emergency", "barrier", "highway", "office",   "leisure",
                                         "waterway",  "natural", "place",   "entrance", "building"};

string GetTypeForFeature(XMLFeature const & node)
{
  for (string const & key : kMainTags)
  {
    if (node.HasTag(key))
    {
      string const value = node.GetTagValue(key);
      if (value == "yes")
        return key;
      else if (key == "shop" || key == "office" || key == "building" || key == "entrance")
        return value + " " + key;  // "convenience shop"
      else if (!value.empty() && value.back() == 's')
        // Remove 's' from the tail: "toilets" -> "toilet".
        return value.substr(0, value.size() - 1);
      else
        return value;
    }
  }

  // Did not find any known tags.
  return node.HasAnyTags() ? "unknown object" : "empty object";
}

vector<m2::PointD> NaiveSample(vector<m2::PointD> const & source, size_t count)
{
  count = min(count, source.size());
  vector<m2::PointD> result;
  result.reserve(count);
  vector<size_t> indexes;
  indexes.reserve(count);

  minstd_rand engine;
  uniform_int_distribution<size_t> distrib(0, source.size());

  while (count--)
  {
    size_t index;
    do
    {
      index = distrib(engine);
    } while (find(begin(indexes), end(indexes), index) != end(indexes));
    result.push_back(source[index]);
    indexes.push_back(index);
  }

  return result;
}
}  // namespace

namespace pugi
{
string DebugPrint(xml_document const & doc)
{
  ostringstream stream;
  doc.print(stream, "  ");
  return stream.str();
}
}  // namespace pugi

namespace osm
{
ChangesetWrapper::ChangesetWrapper(KeySecret const & keySecret,
                                   ServerApi06::KeyValueTags const & comments) noexcept
  : m_changesetComments(comments), m_api(OsmOAuth::ServerAuth(keySecret))
{
}

ChangesetWrapper::~ChangesetWrapper()
{
  if (m_changesetId)
  {
    try
    {
      m_changesetComments["comment"] = GetDescription();
      m_api.UpdateChangeSet(m_changesetId, m_changesetComments);
      m_api.CloseChangeSet(m_changesetId);
    }
    catch (exception const & ex)
    {
      LOG(LWARNING, (ex.what()));
    }
  }
}

void ChangesetWrapper::LoadXmlFromOSM(ms::LatLon const & ll, pugi::xml_document & doc,
                                      double radiusInMeters)
{
  auto const response = m_api.GetXmlFeaturesAtLatLon(ll.m_lat, ll.m_lon, radiusInMeters);
  if (response.first != OsmOAuth::HTTP::OK)
    MYTHROW(HttpErrorException, ("HTTP error", response, "with GetXmlFeaturesAtLatLon", ll));

  if (pugi::status_ok != doc.load(response.second.c_str()).status)
    MYTHROW(
        OsmXmlParseException,
        ("Can't parse OSM server response for GetXmlFeaturesAtLatLon request", response.second));
}

void ChangesetWrapper::LoadXmlFromOSM(ms::LatLon const & min, ms::LatLon const & max,
                                      pugi::xml_document & doc)
{
  auto const response = m_api.GetXmlFeaturesInRect(min.m_lat, min.m_lon, max.m_lat, max.m_lon);
  if (response.first != OsmOAuth::HTTP::OK)
    MYTHROW(HttpErrorException, ("HTTP error", response, "with GetXmlFeaturesInRect", min, max));

  if (pugi::status_ok != doc.load(response.second.c_str()).status)
    MYTHROW(OsmXmlParseException,
            ("Can't parse OSM server response for GetXmlFeaturesInRect request", response.second));
}

XMLFeature ChangesetWrapper::GetMatchingNodeFeatureFromOSM(m2::PointD const & center)
{
  // Match with OSM node.
  ms::LatLon const ll = mercator::ToLatLon(center);
  pugi::xml_document doc;
  // Throws!
  LoadXmlFromOSM(ll, doc);

  pugi::xml_node const bestNode = matcher::GetBestOsmNode(doc, ll);
  if (bestNode.empty())
  {
    MYTHROW(OsmObjectWasDeletedException,
            ("OSM does not have any nodes at the coordinates", ll, ", server has returned:", doc));
  }

  if (!OsmFeatureHasTags(bestNode))
  {
    stringstream sstr;
    bestNode.print(sstr);
    LOG(LDEBUG, ("Node has no tags", sstr.str()));
    MYTHROW(EmptyFeatureException, ("Node has no tags"));
  }

  return XMLFeature(bestNode);
}

XMLFeature ChangesetWrapper::GetMatchingAreaFeatureFromOSM(vector<m2::PointD> const & geometry)
{
  auto const kSamplePointsCount = 3;
  bool hasRelation = false;
  // Try several points in case of poor osm response.
  for (auto const & pt : NaiveSample(geometry, kSamplePointsCount))
  {
    ms::LatLon const ll = mercator::ToLatLon(pt);
    pugi::xml_document doc;
    // Throws!
    LoadXmlFromOSM(ll, doc);

    if (doc.select_node("osm/relation"))
    {
      auto const rect = GetBoundingRect(geometry);
      LoadXmlFromOSM(ms::LatLon(rect.minY(), rect.minX()), ms::LatLon(rect.maxY(), rect.maxX()), doc);
      hasRelation = true;
    }

    pugi::xml_node const bestWayOrRelation = matcher::GetBestOsmWayOrRelation(doc, geometry);
    if (!bestWayOrRelation)
    {
      if (hasRelation)
        break;
      continue;
    }

    if (!OsmFeatureHasTags(bestWayOrRelation))
    {
      stringstream sstr;
      bestWayOrRelation.print(sstr);
      LOG(LDEBUG, ("The matched object has no tags", sstr.str()));
      MYTHROW(EmptyFeatureException, ("The matched object has no tags"));
    }

    return XMLFeature(bestWayOrRelation);
  }
  MYTHROW(OsmObjectWasDeletedException, ("OSM does not have any matching way for feature"));
}

void ChangesetWrapper::Create(XMLFeature node)
{
  if (m_changesetId == kInvalidChangesetId)
    m_changesetId = m_api.CreateChangeSet(m_changesetComments);

  // Changeset id should be updated for every OSM server commit.
  node.SetAttribute("changeset", strings::to_string(m_changesetId));
  // TODO(AlexZ): Think about storing/logging returned OSM ids.
  UNUSED_VALUE(m_api.CreateElement(node));
  m_created_types[GetTypeForFeature(node)]++;
}

void ChangesetWrapper::Modify(XMLFeature node)
{
  if (m_changesetId == kInvalidChangesetId)
    m_changesetId = m_api.CreateChangeSet(m_changesetComments);

  // Changeset id should be updated for every OSM server commit.
  node.SetAttribute("changeset", strings::to_string(m_changesetId));
  m_api.ModifyElement(node);
  m_modified_types[GetTypeForFeature(node)]++;
}

void ChangesetWrapper::Delete(XMLFeature node)
{
  if (m_changesetId == kInvalidChangesetId)
    m_changesetId = m_api.CreateChangeSet(m_changesetComments);

  // Changeset id should be updated for every OSM server commit.
  node.SetAttribute("changeset", strings::to_string(m_changesetId));
  m_api.DeleteElement(node);
  m_deleted_types[GetTypeForFeature(node)]++;
}

string ChangesetWrapper::TypeCountToString(TypeCount const & typeCount)
{
  if (typeCount.empty())
    return string();

  // Convert map to vector and sort pairs by count, descending.
  vector<pair<string, size_t>> items;
  for (auto const & tc : typeCount)
    items.push_back(tc);

  sort(items.begin(), items.end(),
       [](pair<string, size_t> const & a, pair<string, size_t> const & b)
       {
         return a.second > b.second;
       });

  ostringstream ss;
  size_t const limit = min(size_t(3), items.size());
  for (size_t i = 0; i < limit; ++i)
  {
    if (i > 0)
    {
      // Separator: "A and B" for two, "A, B, and C" for three or more.
      if (limit > 2)
        ss << ", ";
      else
        ss << " ";
      if (i == limit - 1)
        ss << "and ";
    }

    auto & currentPair = items[i];
    // If we have more objects left, make the last one a list of these.
    if (i == limit - 1 && limit < items.size())
    {
      int count = 0;
      for (auto j = i; j < items.size(); ++j)
        count += items[j].second;
      currentPair = {"other object", count};
    }

    // Format a count: "a shop" for single shop, "4 shops" for multiple.
    if (currentPair.second == 1)
    {
      if (kVowels.find(currentPair.first.front()) != string::npos)
        ss << "an";
      else
        ss << "a";
    }
    else
    {
      ss << currentPair.second;
    }
    ss << ' ' << currentPair.first;
    if (currentPair.second > 1)
    {
      if (currentPair.first.size() >= 2)
      {
        string const lastTwo = currentPair.first.substr(currentPair.first.size() - 2);
        // "bench" -> "benches", "marsh" -> "marshes", etc.
        if (lastTwo.back() == 'x' || lastTwo == "sh" || lastTwo == "ch" || lastTwo == "ss")
        {
          ss << 'e';
        }
        // "library" -> "libraries"
        else if (lastTwo.back() == 'y' && kVowels.find(lastTwo.front()) == string::npos)
        {
          auto const pos = static_cast<size_t>(ss.tellp());
          ss.seekp(static_cast<typename ostringstream::pos_type>(pos - 1));
          ss << "ie";
        }
      }
      ss << 's';
    }
  }
  return ss.str();
}

string ChangesetWrapper::GetDescription() const
{
  string result;
  if (!m_created_types.empty())
    result = "Created " + TypeCountToString(m_created_types);
  if (!m_modified_types.empty())
  {
    if (!result.empty())
      result += "; ";
    result += "Updated " + TypeCountToString(m_modified_types);
  }
  if (!m_deleted_types.empty())
  {
    if (!result.empty())
      result += "; ";
    result += "Deleted " + TypeCountToString(m_deleted_types);
  }
  return result;
}
}  // namespace osm
