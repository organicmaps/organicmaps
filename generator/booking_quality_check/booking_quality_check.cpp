#include "generator/booking_dataset.hpp"
#include "generator/emitter_booking.hpp"
#include "generator/feature_builder.hpp"
#include "generator/opentable_dataset.hpp"
#include "generator/osm_source.hpp"
#include "generator/sponsored_scoring.hpp"

#include "indexer/classificator_loader.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "coding/file_name_utils.hpp"

#include "base/exception.hpp"
#include "base/geo_object_id.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>

#include "3party/gflags/src/gflags/gflags.h"

#include "boost/range/adaptor/map.hpp"
#include "boost/range/algorithm/copy.hpp"

using namespace std;

DEFINE_string(osm, "", "Input .o5m file");
DEFINE_string(booking, "", "Path to booking data in .tsv format");
DEFINE_string(opentable, "", "Path to opentable data in .tsv format");
DEFINE_string(factors, "", "Factors output path");
DEFINE_string(sample, "", "Path so sample file");

DEFINE_uint64(seed, minstd_rand::default_seed, "Seed for random shuffle");
DEFINE_uint64(selection_size, 1000, "Selection size");
DEFINE_bool(generate, false, "Generate unmarked sample");

using namespace generator;

namespace
{
string PrintBuilder(FeatureBuilder1 const & fb)
{
  ostringstream s;

  s << "Id: " << DebugPrint(fb.GetMostGenericOsmId()) << '\t'
    << "Name: " << fb.GetName(StringUtf8Multilang::kDefaultCode) << '\t';

  auto const & params = fb.GetParams();
  auto const street = params.GetStreet();
  auto const house = params.house.Get();

  string address = street;
  if (!house.empty())
  {
    if (!street.empty())
      address += ", ";
    address += house;
  }

  if (!address.empty())
    s << "Address: " << address << '\t';

  auto const center = MercatorBounds::ToLatLon(fb.GetKeyPoint());
  s << "lat: " << center.lat << " lon: " << center.lon << '\t';

  if (fb.GetGeomType() == feature::GEOM_POINT)
    s << "GeomType: GEOM_POINT";
  else if (fb.GetGeomType() == feature::GEOM_AREA)
    s << "GeomType: GEOM_AREA";
  else
    CHECK(false, ());

  return s.str();
}

DECLARE_EXCEPTION(ParseError, RootException);

base::GeoObjectId ReadDebuggedPrintedOsmId(string const & str)
{
  istringstream sstr(str);
  string type;
  uint64_t id;
  sstr >> type >> id;

  if (sstr.fail())
    MYTHROW(ParseError, ("Can't make osmId from string", str));

  if (type == "node")
    return base::MakeOsmNode(id);
  if (type == "way")
    return base::MakeOsmWay(id);
  if (type == "relation")
    return base::MakeOsmRelation(id);

  MYTHROW(ParseError, ("Can't make osmId from string", str));
}

feature::GenerateInfo GetGenerateInfo()
{
  feature::GenerateInfo info;
  info.m_bookingDatafileName = FLAGS_booking;
  info.m_opentableDatafileName = FLAGS_opentable;
  info.m_osmFileName = FLAGS_osm;
  info.SetNodeStorageType("map");
  info.SetOsmFileType("o5m");

  info.m_intermediateDir = base::GetDirectory(FLAGS_factors);

  // Set other info params here.

  return info;
}

template <typename Object>
struct SampleItem
{
  enum MatchStatus {Uninitialized, Yes, No};
  using ObjectId = typename Object::ObjectId;

  SampleItem() = default;

  SampleItem(base::GeoObjectId const & osmId, ObjectId const sponsoredId,
             MatchStatus match = Uninitialized)
    : m_osmId(osmId), m_sponsoredId(sponsoredId), m_match(match)
  {
  }

  base::GeoObjectId m_osmId;
  ObjectId m_sponsoredId = Object::InvalidObjectId();

  MatchStatus m_match = Uninitialized;
};

template <typename Object>
typename SampleItem<Object>::MatchStatus ReadMatchStatus(string const & str)
{
  if (str == "Yes")
    return SampleItem<Object>::Yes;

  if (str == "No")
    return SampleItem<Object>::No;

  if (str == "Uninitialized")
    return SampleItem<Object>::Uninitialized;

  MYTHROW(ParseError, ("Can't make SampleItem::MatchStatus from string:", str));
}

template <typename Object>
SampleItem<Object> ReadSampleItem(string const & str)
{
  SampleItem<Object> item;

  auto const parts = strings::Tokenize(str, "\t");
  CHECK_EQUAL(parts.size(), 3, ("Cant't make SampleItem from string:", str,
                                "due to wrong number of fields."));

  item.m_osmId = ReadDebuggedPrintedOsmId(parts[0]);
  if (!strings::to_uint(parts[1], item.m_sponsoredId.Get()))
    MYTHROW(ParseError, ("Can't make uint32 from string:", parts[1]));
  item.m_match = ReadMatchStatus<Object>(parts[2]);

  return item;
}

template <typename Object>
vector<SampleItem<Object>> ReadSample(istream & ist)
{
  vector<SampleItem<Object>> result;

  size_t lineNumber = 1;
  try
  {
    for (string line; getline(ist, line); ++lineNumber)
    {
      result.emplace_back(ReadSampleItem<Object>(line));
    }
  }
  catch (ParseError const & e)
  {
    LOG_SHORT(LERROR, ("Wrong format: line", lineNumber, e.Msg()));
    exit(1);
  }

  return result;
}

template <typename Object>
vector<SampleItem<Object>> ReadSampleFromFile(string const & name)
{
  ifstream ist(name);
  CHECK(ist.is_open(), ("Can't open file:", name, strerror(errno)));
  return ReadSample<Object>(ist);
}

template <typename Dataset, typename Object = typename Dataset::Object>
void GenerateFactors(Dataset const & dataset,
                     map<base::GeoObjectId, FeatureBuilder1> const & features,
                     vector<SampleItem<Object>> const & sampleItems, ostream & ost)
{
  for (auto const & item : sampleItems)
  {
    auto const & object = dataset.GetStorage().GetObjectById(item.m_sponsoredId);
    auto const & feature = features.at(item.m_osmId);

    auto const score = generator::sponsored_scoring::Match(object, feature);

    auto const center = MercatorBounds::ToLatLon(feature.GetKeyPoint());
    double const distanceMeters = ms::DistanceOnEarth(center, object.m_latLon);
    auto const matched = score.IsMatched();

    ost << "# ------------------------------------------" << fixed << setprecision(6)
        << endl;
    ost << (matched ? 'y' : 'n') << " \t" << DebugPrint(feature.GetMostGenericOsmId())
        << "\t " << object.m_id
        << "\tdistance: " << distanceMeters
        << "\tdistance score: " << score.m_linearNormDistanceScore
        << "\tname score: " << score.m_nameSimilarityScore
        << "\tresult score: " << score.GetMatchingScore()
        << endl;
    ost << "# " << PrintBuilder(feature) << endl;
    ost << "# " << object << endl;
    ost << "# URL: https://www.openstreetmap.org/?mlat="
        << object.m_latLon.lat << "&mlon=" << object.m_latLon.lon << "#map=18/"
        << object.m_latLon.lat << "/" << object.m_latLon.lon << endl;
  }
}

enum class DatasetType
{
  Booking,
  Opentable
};

template <typename Dataset, typename Object = typename Dataset::Object>
void GenerateSample(Dataset const & dataset,
                    map<base::GeoObjectId, FeatureBuilder1> const & features, ostream & ost)
{
  LOG_SHORT(LINFO, ("Num of elements:", features.size()));
  vector<base::GeoObjectId> elementIndexes(features.size());
  boost::copy(features | boost::adaptors::map_keys, begin(elementIndexes));

  // TODO(mgsergio): Try RandomSample (from search:: at the moment of writing).
  shuffle(elementIndexes.begin(), elementIndexes.end(), minstd_rand(static_cast<uint32_t>(FLAGS_seed)));
  if (FLAGS_selection_size < elementIndexes.size())
    elementIndexes.resize(FLAGS_selection_size);

  stringstream outStream;

  for (auto osmId : elementIndexes)
  {
    auto const & fb = features.at(osmId);
    auto const sponsoredIndexes = dataset.GetStorage().GetNearestObjects(
        MercatorBounds::ToLatLon(fb.GetKeyPoint()));

    for (auto const sponsoredId : sponsoredIndexes)
    {
      auto const & object = dataset.GetStorage().GetObjectById(sponsoredId);
      auto const score = sponsored_scoring::Match(object, fb);

      auto const center = MercatorBounds::ToLatLon(fb.GetKeyPoint());
      double const distanceMeters = ms::DistanceOnEarth(center, object.m_latLon);
      auto const matched = score.IsMatched();

      outStream << "# ------------------------------------------" << fixed << setprecision(6)
                << endl;
      outStream << (matched ? 'y' : 'n') << " \t" << DebugPrint(osmId) << "\t " << sponsoredId
                << "\tdistance: " << distanceMeters
                << "\tdistance score: " << score.m_linearNormDistanceScore
                << "\tname score: " << score.m_nameSimilarityScore
                << "\tresult score: " << score.GetMatchingScore()
                << endl;
      outStream << "# " << PrintBuilder(fb) << endl;
      outStream << "# " << object << endl;
      outStream << "# URL: https://www.openstreetmap.org/?mlat="
                << object.m_latLon.lat << "&mlon=" << object.m_latLon.lon
                << "#map=18/" << object.m_latLon.lat << "/" << object.m_latLon.lon << endl;
    }
    if (!sponsoredIndexes.empty())
      outStream << endl << endl;
  }

  if (FLAGS_sample.empty())
  {
    cout << outStream.str();
  }
  else
  {
    ofstream file(FLAGS_sample);
    if (file.is_open())
      file << outStream.str();
    else
      LOG_SHORT(LERROR, ("Can't output into", FLAGS_sample, strerror(errno)));
  }
}

template <typename Dataset>
string GetDatasetFilePath(feature::GenerateInfo const & info);

template <>
string GetDatasetFilePath<BookingDataset>(feature::GenerateInfo const & info)
{
  return info.m_bookingDatafileName;
}

template <>
string GetDatasetFilePath<OpentableDataset>(feature::GenerateInfo const & info)
{
  return info.m_opentableDatafileName;
}

template <typename Dataset, typename Object = typename Dataset::Object>
void RunImpl(feature::GenerateInfo & info)
{
  auto const & dataSetFilePath = GetDatasetFilePath<Dataset>(info);
  Dataset dataset(dataSetFilePath);
  LOG_SHORT(LINFO, (dataset.GetStorage().Size(), "objects are loaded from a file:", dataSetFilePath));

  map<base::GeoObjectId, FeatureBuilder1> features;
  LOG_SHORT(LINFO, ("OSM data:", FLAGS_osm));
  auto emitter = make_shared<EmitterBooking<Dataset>>(dataset, features);
  GenerateFeatures(info, emitter);

  if (FLAGS_generate)
  {
    ofstream ost(FLAGS_sample);
    GenerateSample(dataset, features, ost);
  }
  else
  {
    auto const sample = ReadSampleFromFile<Object>(FLAGS_sample);
    LOG_SHORT(LINFO, ("Sample size is", sample.size()));
    ofstream ost(FLAGS_factors);
    CHECK(ost.is_open(), ("Can't open file", FLAGS_factors, strerror(errno)));
    GenerateFactors<Dataset>(dataset, features, sample, ost);
  }
}

void Run(DatasetType const datasetType, feature::GenerateInfo & info)
{
  switch (datasetType)
  {
  case DatasetType::Booking: RunImpl<BookingDataset>(info); break;
  case DatasetType::Opentable: RunImpl<OpentableDataset>(info); break;
  }
}
}  // namespace

int main(int argc, char * argv[])
{
  google::SetUsageMessage("Calculates factors for given samples.");

  if (argc == 1)
  {
    google::ShowUsageWithFlags(argv[0]);
    exit(0);
  }

  google::ParseCommandLineFlags(&argc, &argv, true);

  CHECK(!FLAGS_sample.empty(), ("Please specify sample path."));
  CHECK(!FLAGS_osm.empty(), ("Please specify osm path."));
  CHECK(!FLAGS_booking.empty() ^ !FLAGS_opentable.empty(),
        ("Please specify either booking or opentable path."));
  CHECK(!FLAGS_factors.empty() ^ FLAGS_generate, ("Please either specify factors path"
                                                  "or use -generate."));

  auto const datasetType = FLAGS_booking.empty() ? DatasetType::Opentable : DatasetType::Booking;

  classificator::Load();

  auto info = GetGenerateInfo();
  GenerateIntermediateData(info);

  Run(datasetType, info);

  return 0;
}
