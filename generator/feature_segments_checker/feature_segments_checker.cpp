#include "generator/srtm_parser.hpp"

#include "routing/bicycle_model.hpp"

#include "coding/file_name_utils.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/map_style_reader.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "std/cmath.hpp"
#include "std/iostream.hpp"
#include "std/fstream.hpp"
#include "std/map.hpp"
#include "std/set.hpp"
#include "std/string.hpp"

#include "3party/gflags/src/gflags/gflags.h"

DEFINE_string(srtm_dir_path, "", "Path to directory with SRTM files");
DEFINE_string(mwm_file_path, "", "Path to an mwm file.");

namespace
{
routing::BicycleModel const & GetBicycleModel()
{
  static routing::BicycleModel const instance;
  return instance;
}

int32_t Coord2RoughCoord(double d)
{
  int32_t constexpr kFactor = 100000;
  return static_cast<int32_t>(d * kFactor);
}

struct RoughPoint
{
  RoughPoint(m2::PointD const & point)
    : x(Coord2RoughCoord(point.x)), y(Coord2RoughCoord(point.y)) {}

  int32_t x;
  int32_t y;
};

bool operator< (RoughPoint const & l, RoughPoint const & r)
{
  if (l.x != r.x)
    return l.x < r.x;
  return l.y < r.y;
}

template <typename Cont>
void PrintCont(Cont const & cont, string const & title, string const & msgText1,
               string const & msgText2)
{
  cout << endl << title << endl;
  for (auto const & a : cont)
    cout << a.second << msgText1 << a.first << msgText2 << endl;
}

template <typename Cont>
void WriteCSV(Cont const & cont, string const & fileName)
{
  ofstream fout(fileName);
  for (auto const & a : cont)
    fout << a.first << "," << a.second << endl;
}

/// \returns expected point altitude in meters according to linear model.
double ExpectedPointAltitude(int16_t startAltitudeMeters, int16_t endAltitudeMeters,
                             double distFromStartMeters, double featureLengthMeters)
{
  if (featureLengthMeters == 0.0)
    return 0;
  double const k = (endAltitudeMeters - startAltitudeMeters) / featureLengthMeters;
  return startAltitudeMeters + k * distFromStartMeters;
}

class Processor
{
public:
  generator::SrtmTileManager & m_srtmManager;
  set<RoughPoint> m_uniqueRoadPoints;
  /// Key is an altitude difference for a feature in meters. If a feature goes up the key is greater then 0.
  /// Value is a number of features.
  map<generator::SrtmTile::THeight, uint32_t> m_altitudeDiffs;
  /// Key is a length of a feature in meters. Value is a number of features.
  map<uint32_t, uint32_t> m_featureLength;
  /// Key is a length of a feature segment in meters. Value is a segment counter.
  map<uint32_t, uint32_t> m_segLength;
  /// Key is difference between two values:
  /// 1. how many meters it's necessary to go up following the feature. If feature wavy
  ///   calculates only raise meters.
  /// 2. the difference in altitude between end and start of features.
  /// Value is a segment counter.
  map<int32_t, uint32_t> m_featureWave;
  /// Key is number of meters which is necessary to go up following the feature.
  /// Value is a number of features.
  map<int32_t, uint32_t> m_featureUp;
  /// Key is number of meters which is necessary to go down following the feature.
  /// Value is a number of features.
  map<int32_t, uint32_t> m_featureDown;
  /// Key is number of meters. It shows altitude deviation of intermediate feature points
  /// from linear model.
  /// Value is a number of features.
  map<int32_t, uint32_t> m_diffFromLinear;
  /// Number of features for GetBicycleModel().IsRoad(feature) == true.
  uint32_t m_roadCount;
  /// Number of features for empty features with GetBicycleModel().IsRoad(feature).
  uint32_t m_emptyRoadCount;
  /// Point counter for feature where GetBicycleModel().IsRoad(feature) == true.
  uint32_t m_roadPointCount;
  /// Number of features for GetBicycleModel().IsRoad(feature) != true.
  uint32_t m_notRoadCount;

  Processor(generator::SrtmTileManager & manager)
    : m_srtmManager(manager), m_roadCount(0), m_emptyRoadCount(0), m_roadPointCount(0), m_notRoadCount(0)
  {
  }

  void operator()(FeatureType const & f, uint32_t const & id)
  {
    f.ParseBeforeStatistic();
    if (!GetBicycleModel().IsRoad(f))
    {
      ++m_notRoadCount;
      return;
    }

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    uint32_t const numPoints = f.GetPointsCount();
    if (numPoints == 0)
    {
      ++m_emptyRoadCount;
      return;
    }

    ++m_roadCount;

    m_roadPointCount += numPoints;

    for (uint32_t i = 0; i < numPoints; ++i)
      m_uniqueRoadPoints.insert(RoughPoint(f.GetPoint(i)));

    // Feature length and feature segment length.
    double realFeatureLengthMeters = 0.0;
    for (uint32_t i = 0; i + 1 < numPoints; ++i)
    {
      // Feature segment length.
      double const realSegmentLengthMeters = MercatorBounds::DistanceOnEarth(f.GetPoint(i), f.GetPoint(i + 1));
      m_segLength[static_cast<uint32_t>(floor(realSegmentLengthMeters))]++;

      // Feature length.
      realFeatureLengthMeters += realSegmentLengthMeters;
    }
    m_featureLength[static_cast<uint32_t>(realFeatureLengthMeters)]++;

    // Feature altitude difference.
    generator::SrtmTile::THeight const startAltitude =
        m_srtmManager.GetHeight(MercatorBounds::ToLatLon(f.GetPoint(0)));
    generator::SrtmTile::THeight const endAltitude =
        m_srtmManager.GetHeight(MercatorBounds::ToLatLon(f.GetPoint(numPoints - 1)));
    int16_t const altitudeDiff = endAltitude - startAltitude;
    m_altitudeDiffs[altitudeDiff]++;

    // Wave feature factor. Climb minus altitude difference.
    int32_t climb = 0;
    int32_t up = 0;
    int32_t down = 0;
    for (uint32_t i = 0; i + 1 < numPoints; ++i)
    {
      auto const segAltDiff =
          (m_srtmManager.GetHeight(MercatorBounds::ToLatLon(f.GetPoint(i + 1))) -
           m_srtmManager.GetHeight(MercatorBounds::ToLatLon(f.GetPoint(i))));
      if (altitudeDiff >= 0)
      {
        // If the feature goes up generaly calculates how many meters it's necessary
        // to go up following the feature.
        if (segAltDiff > 0)
          climb += segAltDiff;
      }
      else
      {
        // If the feature goes down generaly calculates how many meters it's necessary
        // to go down following the feature.
        if (segAltDiff < 0)
          climb += segAltDiff;
      }

      if (segAltDiff >= 0)
        up += segAltDiff;
      else
        down += segAltDiff;
    }
    // Estimates monotony of the feature. If climb == altitudeDiff it's monotonous.
    // If not it's wavy. The more abs(climb - altitudeDiff) the more wavy the feature.
    int32_t const waveFactor = climb - altitudeDiff;
    m_featureWave[waveFactor]++;
    m_featureUp[up]++;
    m_featureDown[down]++;

    // Altitude deviation of internal feature points from linear model.
    if (realFeatureLengthMeters == 0.0)
      return;

    double distFromStartMeters = 0;
    for (uint32_t i = 1; i + 1 < numPoints; ++i)
    {
      // Feature segment length.
      double const segmentLengthMeters =
          MercatorBounds::DistanceOnEarth(f.GetPoint(i - 1), f.GetPoint(i));
      distFromStartMeters += segmentLengthMeters;

      generator::SrtmTile::THeight const pointAltitude =
          m_srtmManager.GetHeight(MercatorBounds::ToLatLon(f.GetPoint(i)));
      int32_t const deviation = static_cast<int32_t>(ExpectedPointAltitude(startAltitude, endAltitude, distFromStartMeters,
                                                                           realFeatureLengthMeters)) - pointAltitude;
      m_diffFromLinear[deviation]++;
    }
  }
};

double CalculateBinaryEntropy(map<int32_t, uint32_t> const & diffFromLinear)
{
  uint32_t innerPointCount = 0;
  for (auto const & f : diffFromLinear)
    innerPointCount += f.second;

  if (innerPointCount == 0)
    return 0.0;

  double entropy = 0;
  for (auto const & f : diffFromLinear)
  {
    double const p = static_cast<double>(f.second) / innerPointCount;
    entropy += -p * log2(p);
  }
  return entropy;
}
}  // namespace

int main(int argc, char ** argv)
{
  google::SetUsageMessage("This tool extracts some staticstics about features and its altitudes.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  LOG(LINFO, ("srtm_dir_path =", FLAGS_srtm_dir_path));
  LOG(LINFO, ("mwm_file_path =", FLAGS_mwm_file_path));

  classificator::Load();
  generator::SrtmTileManager manager(FLAGS_srtm_dir_path);

  Processor processor(manager);
  feature::ForEachFromDat(FLAGS_mwm_file_path, processor);

  PrintCont(processor.m_altitudeDiffs, "Altitude difference between start and end of features.",
            " feature(s) with altitude difference ", " meter(s)");
  WriteCSV(processor.m_altitudeDiffs, "altitude_difference.csv");

  PrintCont(processor.m_featureLength, "Feature length.", " feature(s) with length ", " meter(s)");
  WriteCSV(processor.m_featureLength, "feature_length.csv");

  PrintCont(processor.m_segLength, "Feature segment length.", " segment(s) with length ",
            " meter(s)");

  PrintCont(processor.m_featureWave, "Wave factor", " feature(s) with wave factor ", "");
  WriteCSV(processor.m_featureWave, "feature_wave.csv");

  PrintCont(processor.m_featureUp, "Feature go up in meters", " feature(s) go up ", " meter(s)");
  WriteCSV(processor.m_featureUp, "feature_up.csv");

  PrintCont(processor.m_featureDown, "Feature go down in meters", " feature(s) go down ", " meter(s)");
  WriteCSV(processor.m_featureDown, "feature_down.csv");

  PrintCont(processor.m_diffFromLinear, "Altitude deviation of internal feature points from linear model.",
            " internal feature point(s) deviate from linear model with ", " meter(s)");

  cout << endl << "Road feature count = " << processor.m_roadCount << endl;
  cout << "Empty road feature count = " << processor.m_emptyRoadCount << endl;
  cout << "Unique road points count = " << processor.m_uniqueRoadPoints.size() << endl;
  cout << "All road point count = " << processor.m_roadPointCount << endl;
  cout << "Not road feature count = " << processor.m_notRoadCount << endl;
  cout << "Binary entropy for altitude deviation of internal feature points from linear model  = "
       << CalculateBinaryEntropy(processor.m_diffFromLinear) << endl;
  return 0;
}
