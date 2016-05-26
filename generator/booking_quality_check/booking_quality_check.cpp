#include "std/iostream.hpp"

#include "generator/booking_dataset.hpp"
#include "generator/osm_source.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "3party/gflags/src/gflags/gflags.h"

DEFINE_bool(generate_classif, false, "Generate classificator.");

DEFINE_bool(preprocess, false, "1st pass - count features");
DEFINE_string(osm_file_name, "", "Input .o5m file");
DEFINE_string(booking_data, "", "Path to booking data in .tsv format");
DEFINE_uint64(selection_size, 1000, "Selection size");

using namespace generator;


ostream & operator << (ostream & s, OsmElement const & e)
{
  for (auto const & tag : e.Tags())
  {
    s << tag.key << "=" << tag.value << "\t";
  }
  return s;
}

int main(int argc, char * argv[])
{
  google::SetUsageMessage("Takes OSM XML data from stdin and creates"
                          " data and index files in several passes.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  LOG_SHORT(LINFO, ("Booking data:",FLAGS_booking_data));

  BookingDataset bookingDataset(FLAGS_booking_data);

  // Here we can add new tags to element!!!
  auto const filterAction = [&](OsmElement * e)
  {
    if (bookingDataset.BookingFilter(*e))
      return;

  };

  vector<OsmElement> elements;
  auto const counterAction = [&](OsmElement * e)
  {
    if (bookingDataset.TourismFilter(*e))
      elements.emplace_back(*e);
  };

  LOG_SHORT(LINFO, ("OSM data:", FLAGS_osm_file_name));
  {
    SourceReader reader = FLAGS_osm_file_name.empty() ? SourceReader() : SourceReader(FLAGS_osm_file_name);
    ProcessOsmElementsFromO5M(reader, counterAction);
  }
  LOG_SHORT(LINFO, ("Tourism elements:", elements.size()));

  vector<size_t> elementIndexes(elements.size());
  size_t counter = 0;
  for (auto & e : elementIndexes)
    e = counter++;

  random_shuffle(elementIndexes.begin(), elementIndexes.end());
  elementIndexes.resize(FLAGS_selection_size);

  for (size_t i : elementIndexes)
  {
    OsmElement const & e = elements[i];
    auto const bookingIndexes = bookingDataset.GetNearestHotels(e.lat, e.lon, 3, 150);
    for (size_t const j : bookingIndexes)
    {
      auto const & hotel = bookingDataset.GetHotel(j);
      double const dist = ms::DistanceOnEarth(e.lat, e.lon, hotel.lat, hotel.lon);
      cout << "# ------------------------------------------" << fixed << setprecision(6) << endl;
      cout << "y \t" << i << "\t " << j << " dist: " << dist << endl;
      cout << "# " << e << endl;
      cout << "# " << hotel << endl;
      cout << "# URL: https://www.openstreetmap.org/?mlat=" << hotel.lat << "&mlon=" << hotel.lon << "#map=18/"<< hotel.lat << "/" << hotel.lon << endl;
    }
    cout << endl << endl;
  }


  return 0;
}