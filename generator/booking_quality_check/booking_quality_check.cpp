#include "generator/booking_dataset.hpp"
#include "generator/booking_scoring.hpp"
#include "generator/feature_builder.hpp"
#include "generator/osm_source.hpp"

#include "indexer/classificator_loader.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "coding/file_name_utils.hpp"

#include "base/string_utils.hpp"

#include "std/cstdlib.hpp"
#include "std/cstring.hpp"
#include "std/fstream.hpp"
#include "std/iostream.hpp"
#include "std/numeric.hpp"
#include "std/random.hpp"
#include "std/unique_ptr.hpp"

#include "3party/gflags/src/gflags/gflags.h"

DEFINE_string(osm, "", "Input .o5m file");
DEFINE_string(booking, "", "Path to booking data in .tsv format");
DEFINE_string(factors, "", "Factors output path");
DEFINE_string(sample, "", "Path so sample file");
DEFINE_uint64(selection_size, 1000, "Selection size");
DEFINE_uint64(seed, minstd_rand::default_seed, "Seed for random shuffle");

using namespace generator;

namespace
{
string PrintBuilder(FeatureBuilder1 const & fb)
{
  ostringstream s;

  s << "Id: " << DebugPrint(fb.GetMostGenericOsmId()) << '\t'
    << "Name: " << fb.GetName(StringUtf8Multilang::kDefaultCode) << '\t';

  auto const params = fb.GetParams();
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

osm::Id ReadDebuggedPrintedOsmId(string const & str)
{
  istringstream sstr(str);
  string type;
  uint64_t id;
  sstr >> type >> id;

  if (sstr.fail())
    MYTHROW(ParseError, ("Can't make osmId from string", str));

  if (type == "relation")
    return osm::Id::Relation(id);
  if (type == "way")
    return osm::Id::Way(id);
  if (type == "node")
    return osm::Id::Node(id);

  MYTHROW(ParseError, ("Can't make osmId from string", str));
}

class Emitter : public EmitterBase
{
public:
  Emitter(BookingDataset const & booking, map<osm::Id, FeatureBuilder1> & features)
    : m_bookingDataset(booking)
    , m_features(features)
  {
    LOG_SHORT(LINFO, ("OSM data:", FLAGS_osm));
  }

  void operator()(FeatureBuilder1 & fb) override
  {
    if (m_bookingDataset.CanBeBooking(fb))
      m_features.emplace(fb.GetMostGenericOsmId(), fb);
  }

  void GetNames(vector<string> & names) const override
  {
    names.clear();
  }

  bool Finish() override
  {
    LOG_SHORT(LINFO, ("Num of tourism elements:", m_features.size()));
    return true;
  }

private:
  BookingDataset const & m_bookingDataset;
  map<osm::Id, FeatureBuilder1> & m_features;
};

feature::GenerateInfo GetGenerateInfo()
{
  feature::GenerateInfo info;
  info.m_bookingDatafileName = FLAGS_booking;
  info.m_osmFileName = FLAGS_osm;
  info.SetNodeStorageType("map");
  info.SetOsmFileType("o5m");

  info.m_intermediateDir = my::GetDirectory(FLAGS_factors);

  // Set other info params here.

  return info;
}

struct SampleItem
{
  enum MatchStatus {Uninitialized, Yes, No};

  SampleItem() = default;

  SampleItem(osm::Id const & osmId, uint32_t const bookingId, MatchStatus const match = Uninitialized)
   : m_osmId(osmId)
   , m_bookingId(bookingId)
   , m_match(match)
  {
  }

  osm::Id m_osmId;
  uint32_t m_bookingId = BookingDataset::kInvalidHotelIndex;

  MatchStatus m_match = Uninitialized;
};

SampleItem::MatchStatus ReadMatchStatus(string const & str)
{
  if (str == "Yes")
    return SampleItem::Yes;

  if (str == "No")
    return SampleItem::No;

  if (str == "Uninitialized")
    return SampleItem::Uninitialized;

  MYTHROW(ParseError, ("Can't make SampleItem::MatchStatus from string:", str));
}

SampleItem ReadSampleItem(string const & str)
{
  SampleItem item;

  auto const parts = strings::Tokenize(str, "\t");
  CHECK_EQUAL(parts.size(), 3, ("Cant't make SampleItem from string:", str,
                                "due to wrong number of fields."));

  item.m_osmId = ReadDebuggedPrintedOsmId(parts[0]);
  if (!strings::to_uint(parts[1], item.m_bookingId))
    MYTHROW(ParseError, ("Can't make uint32 from string:", parts[1]));
  item.m_match = ReadMatchStatus(parts[2]);

  return item;
}

vector<SampleItem> ReadSample(istream & ist)
{
  vector<SampleItem> result;

  size_t lineNumber = 1;
  try
  {
    for (string line; getline(ist, line); ++lineNumber)
    {
      result.emplace_back(ReadSampleItem(line));
    }
  }
  catch (ParseError const & e)
  {
    LOG(LERROR, ("Wrong format: line", lineNumber, e.Msg()));
    exit(1);
  }

  return result;
}

vector<SampleItem> ReadSampleFromFile(string const & name)
{
  ifstream ist(name);
  CHECK(ist.is_open(), ("Can't open file:", name, strerror(errno)));
  return ReadSample(ist);
}

void GenerateFactors(BookingDataset const & booking, map<osm::Id, FeatureBuilder1> const & features,
                     vector<SampleItem> const & sampleItems, ostream & ost)
{
  for (auto const & item : sampleItems)
  {
    auto const & hotel = booking.GetHotelById(item.m_bookingId);
    auto const & feature = features.at(item.m_osmId);

    auto const score = booking_scoring::Match(hotel, feature);

    auto const center = MercatorBounds::ToLatLon(feature.GetKeyPoint());
    double const distanceMeters = ms::DistanceOnEarth(center.lat, center.lon,
                                                      hotel.lat, hotel.lon);
    auto const matched = score.IsMatched();

    ost << "# ------------------------------------------" << fixed << setprecision(6)
        << endl;
    ost << (matched ? 'y' : 'n') << " \t" << DebugPrint(feature.GetMostGenericOsmId())
        << "\t " << hotel.id
        << "\tdistance: " << distanceMeters
        << "\tdistance score: " << score.m_linearNormDistanceScore
        << "\tname score: " << score.m_nameSimilarityScore
        << "\tresult score: " << score.GetMatchingScore()
        << endl;
    ost << "# " << PrintBuilder(feature) << endl;
    ost << "# " << hotel << endl;
    ost << "# URL: https://www.openstreetmap.org/?mlat=" << hotel.lat
        << "&mlon=" << hotel.lon << "#map=18/" << hotel.lat << "/" << hotel.lon << endl;
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
  CHECK(!FLAGS_booking.empty(), ("Please specify booking path."));
  CHECK(!FLAGS_factors.empty(), ("Please specify factors path."));

  classificator::Load();

  auto info = GetGenerateInfo();
  GenerateIntermediateData(info);

  LOG_SHORT(LINFO, ("Booking data:", FLAGS_booking));
  BookingDataset booking(info.m_bookingDatafileName);
  LOG_SHORT(LINFO, (booking.Size(), "hotels are loaded from Booking."));

  map<osm::Id, FeatureBuilder1> features;
  GenerateFeatures(info, [&booking, &features](feature::GenerateInfo const & /* info */)
  {
    return make_unique<Emitter>(booking, features);
  });

  auto const sample = ReadSampleFromFile(FLAGS_sample);
  LOG(LINFO, ("Sample size is", sample.size()));
  {
    ofstream ost(FLAGS_factors);
    CHECK(ost.is_open(), ("Can't open file", FLAGS_factors, strerror(errno)));
    GenerateFactors(booking, features, sample, ost);
  }

  return 0;
}
