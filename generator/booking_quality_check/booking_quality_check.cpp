#include "std/iostream.hpp"

#include "generator/booking_dataset.hpp"
#include "generator/osm_source.hpp"

#include "3party/gflags/src/gflags/gflags.h"

DEFINE_bool(generate_classif, false, "Generate classificator.");

DEFINE_bool(preprocess, false, "1st pass - count features");
DEFINE_string(osm_file_name, "", "Input .o5m file");
DEFINE_string(booking_data, "", "Path to booking data in .tsv format");
DEFINE_uint64(selection_size, 1000, "Selection size");

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

  vector<OsmElement> selectedElements;
  for (size_t i : elementIndexes)
    selectedElements.emplace_back(elements[i]);



  return 0;
}