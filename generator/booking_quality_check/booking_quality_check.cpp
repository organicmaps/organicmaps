#include "generator/booking_dataset.hpp"
#include "generator/osm_source.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "std/fstream.hpp"
#include "std/iostream.hpp"
#include "std/numeric.hpp"
#include "std/random.hpp"

#include "3party/gflags/src/gflags/gflags.h"

DEFINE_bool(generate_classif, false, "Generate classificator.");

DEFINE_string(osm_file_name, "", "Input .o5m file");
DEFINE_string(booking_data, "", "Path to booking data in .tsv format");
DEFINE_string(sample_data, "", "Sample output path");
DEFINE_uint64(selection_size, 1000, "Selection size");
DEFINE_uint64(random_seed, minstd_rand::default_seed, "Seed for random shuffle");

using namespace generator;

ostream & operator<<(ostream & s, OsmElement const & e)
{
  for (auto const & tag : e.Tags())
  {
    auto t = tag;
    replace(t.key.begin(), t.key.end(), '\n', ' ');
    replace(t.value.begin(), t.value.end(), '\n', ' ');
    s << t.key << "=" << t.value << "\t";
  }
  return s;
}

int main(int argc, char * argv[])
{
  google::SetUsageMessage(
      "Takes OSM XML data from stdin and creates"
      " data and index files in several passes.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  LOG_SHORT(LINFO, ("Booking data:", FLAGS_booking_data));

  BookingDataset bookingDataset(FLAGS_booking_data);

  vector<OsmElement> elements;
  LOG_SHORT(LINFO, ("OSM data:", FLAGS_osm_file_name));
  {
    SourceReader reader =
        FLAGS_osm_file_name.empty() ? SourceReader() : SourceReader(FLAGS_osm_file_name);
    ProcessOsmElementsFromO5M(reader, [&](OsmElement * e)
                              {
                                if (bookingDataset.TourismFilter(*e))
                                  elements.emplace_back(*e);
                              });
  }
  LOG_SHORT(LINFO, ("Num of tourism elements:", elements.size()));

  vector<size_t> elementIndexes(elements.size());
  iota(elementIndexes.begin(), elementIndexes.end(), 0);

  shuffle(elementIndexes.begin(), elementIndexes.end(), minstd_rand(FLAGS_random_seed));
  if (FLAGS_selection_size < elementIndexes.size())
    elementIndexes.resize(FLAGS_selection_size);

  stringstream outStream;

  for (size_t i : elementIndexes)
  {
    OsmElement const & e = elements[i];
    auto const bookingIndexes = bookingDataset.GetNearestHotels(
        e.lat, e.lon, BookingDataset::kMaxSelectedElements, BookingDataset::kDistanceLimitInMeters);
    for (size_t const j : bookingIndexes)
    {
      auto const & hotel = bookingDataset.GetHotel(j);
      double const distanceMeters = ms::DistanceOnEarth(e.lat, e.lon, hotel.lat, hotel.lon);
      double score = BookingDataset::ScoreByLinearNormDistance(distanceMeters);

      bool matched = score > BookingDataset::kOptimalThreshold;

      outStream << "# ------------------------------------------" << fixed << setprecision(6)
                << endl;
      outStream << (matched ? 'y' : 'n') << " \t" << i << "\t " << j
                << " distance: " << distanceMeters << " score: " << score << endl;
      outStream << "# " << e << endl;
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
      file << outStream.str();
    else
      LOG(LERROR, ("Can't output into", FLAGS_sample_data));
  }

  return 0;
}
