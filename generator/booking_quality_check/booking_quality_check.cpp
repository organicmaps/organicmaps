#include "generator/booking_dataset.hpp"
#include "generator/booking_scoring.hpp"
#include "generator/feature_builder.hpp"
#include "generator/osm_source.hpp"

#include "indexer/classificator_loader.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "std/fstream.hpp"
#include "std/iostream.hpp"
#include "std/numeric.hpp"
#include "std/random.hpp"
#include "std/unique_ptr.hpp"

#include "3party/gflags/src/gflags/gflags.h"

// TODO(mgsergio):Unused: DEFINE_bool(generate_classif, false, "Generate classificator.");

DEFINE_string(osm_file_name, "", "Input .o5m file");
DEFINE_string(booking_data, "", "Path to booking data in .tsv format");
DEFINE_string(sample_data, "", "Sample output path");
DEFINE_uint64(selection_size, 1000, "Selection size");
DEFINE_uint64(seed, minstd_rand::default_seed, "Seed for random shuffle");

using namespace generator;

namespace
{
string PrintBuilder(FeatureBuilder1 const & fb)
{
  ostringstream s;

  s << "Name: " << fb.GetName(StringUtf8Multilang::kDefaultCode) << '\t';

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

struct Emitter : public EmitterBase
{
  Emitter(feature::GenerateInfo const & info)
    : m_bookingDataset(info.m_bookingDatafileName)
  {
    LOG_SHORT(LINFO, (m_bookingDataset.Size(), "hotels are loaded from Booking."));
    LOG_SHORT(LINFO, ("OSM data:", FLAGS_osm_file_name));
  }

  void operator()(FeatureBuilder1 & fb) override
  {
    if (m_bookingDataset.TourismFilter(fb))
      m_features.emplace_back(fb);
  }

  void GetNames(vector<string> & names) const override
  {
    names.clear();
  }

  bool Finish() override
  {
    LOG_SHORT(LINFO, ("Num of tourism elements:", m_features.size()));
    vector<size_t> elementIndexes(m_features.size());
    iota(elementIndexes.begin(), elementIndexes.end(), 0);

    shuffle(elementIndexes.begin(), elementIndexes.end(), minstd_rand(FLAGS_seed));
    if (FLAGS_selection_size < elementIndexes.size())
      elementIndexes.resize(FLAGS_selection_size);

    stringstream outStream;

    for (size_t i : elementIndexes)
    {
      auto const & fb = m_features[i];
      auto const center = MercatorBounds::ToLatLon(fb.GetKeyPoint());
      auto const bookingIndexes = m_bookingDataset.GetNearestHotels(
          center.lat, center.lon,
          BookingDataset::kMaxSelectedElements,
          BookingDataset::kDistanceLimitInMeters);

      for (size_t const j : bookingIndexes)
      {
        auto const & hotel = m_bookingDataset.GetHotel(j);
        auto const score = booking_scoring::Match(hotel, fb);

        double const distanceMeters = ms::DistanceOnEarth(center.lat, center.lon,
                                                          hotel.lat, hotel.lon);
        auto const matched = score.IsMatched();

        outStream << "# ------------------------------------------" << fixed << setprecision(6)
                  << endl;
        outStream << (matched ? 'y' : 'n') << " \t" << i << "\t " << j
                  << "\tdistance: " << distanceMeters
                  << "\tdistance score: " << score.m_linearNormDistanceScore
                  << "\tname score: " << score.m_nameSimilarityScore
                  << "\tresult score: " << score.GetMatchingScore()
                  << endl;
        outStream << "# " << PrintBuilder(fb) << endl;
        outStream << "# " << hotel << endl;
        outStream << "# URL: https://www.openstreetmap.org/?mlat=" << hotel.lat
                  << "&mlon=" << hotel.lon << "#map=18/" << hotel.lat << "/" << hotel.lon << endl;
      }
      if (!bookingIndexes.empty())
        outStream << endl << endl;
    }

    if (FLAGS_sample_data.empty())
    {
      cout << outStream.str();
    }
    else
    {
      ofstream file(FLAGS_sample_data);
      if (file.is_open())
      {
        file << outStream.str();
      }
      else
      {
        LOG(LERROR, ("Can't output into", FLAGS_sample_data));
        return false;
      }
    }

    return true;
  }

  BookingDataset m_bookingDataset;
  vector<FeatureBuilder1> m_features;
};

unique_ptr<Emitter> GetEmitter(feature::GenerateInfo const & info)
{
  LOG_SHORT(LINFO, ("Booking data:", FLAGS_booking_data));
  return make_unique<Emitter>(info);
}

feature::GenerateInfo GetGenerateInfo()
{
  feature::GenerateInfo info;
  info.m_bookingDatafileName = FLAGS_booking_data;
  info.m_osmFileName = FLAGS_osm_file_name;
  info.SetNodeStorageType("map");
  info.SetOsmFileType("o5m");

  auto const lastSlash = FLAGS_sample_data.rfind("/");
  if (lastSlash == string::npos)
    info.m_intermediateDir = ".";
  else
    info.m_intermediateDir = FLAGS_sample_data.substr(0, lastSlash);
  // ...
  return info;
}
}  // namespace

int main(int argc, char * argv[])
{
  google::SetUsageMessage(
      "Takes OSM XML data from stdin and creates"
      " data and index files in several passes.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  classificator::Load();

  auto info = GetGenerateInfo();
  GenerateIntermediateData(info);
  GenerateFeatures(info, GetEmitter);

  return 0;
}
