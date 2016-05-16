#include "generator/srtm_parser.hpp"

#include "routing/bicycle_model.hpp"

#include "coding/file_name_utils.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/map_style_reader.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "3party/gflags/src/gflags/gflags.h"

#include "std/iostream.hpp"
#include "std/map.hpp"
#include "std/string.hpp"

DEFINE_string(srtm_path, "", "Path to directory with SRTM files");
DEFINE_string(mwm_path, "", "Path to an mwm file.");

namespace
{
routing::BicycleModel const & GetBicycleModel()
{
  static routing::BicycleModel const instance;
  return instance;
}

template <typename K, typename V>
void IncrementStats(K const & k, map<K, V> & m)
{
  if (m.find(k) == m.cend())
    m[k] = 0;
  m[k] += 1;
}

template <typename Cont>
void PrintCont(Cont const & cont, string const & title, string const & msgText1, string const & msgText2)
{
  cout << endl << title << endl;
  for (auto const a : cont)
    cout << a.second << msgText1 << a.first << msgText2 << endl;
}

/// \brief Adds |point| to |uniqueRoadPoints| if there's no a point in |uniqueRoadPoints|
/// in |kErrorMeters| environment of |point|.
/// \note This method does get an exact result. The problem is points are sorted in |uniqueRoadPoints|
/// according to operator<(m2::PointD const &, m2::PointD const &). It's not the same criteria with
/// MercatorBounds::DistanceOnEarth(lowerPnt, point). But according to tests the error is less then 1%,
/// and function implemented below works much faster.
bool IsNewFeatureEnd(m2::PointD const & point, set<m2::PointD> & uniqueRoadPoints)
{
  double constexpr kErrorMeters = 3.;
  auto const lower = uniqueRoadPoints.lower_bound(point);
  auto const upper = uniqueRoadPoints.upper_bound(point);

  if (lower == uniqueRoadPoints.cend() || upper == uniqueRoadPoints.cend())
    uniqueRoadPoints.insert(point);

  m2::PointD const lowerPnt = *lower;
  m2::PointD const upperPnt = *upper;

  if (MercatorBounds::DistanceOnEarth(lowerPnt, point) <= kErrorMeters
      || MercatorBounds::DistanceOnEarth(upperPnt, point) <= kErrorMeters)
  {
    return false;
  }

  uniqueRoadPoints.insert(point);
  return true;
}

class Processor
{
public:
  generator::SrtmTileManager & m_srtmManager;
  set<m2::PointD> m_uniqueRoadPoints;
  /// Key is altitude difference for feature in meters. If a feature goes up the key is more then 0.
  /// Value is a feature counter.
  map<generator::SrtmTile::THeight, size_t> m_altitudeDiffs;
  /// Key is length of feature in meters. Value is a feature counter.
  map<size_t, size_t> m_featureLength;
  /// Key is length of feature segment in meters. Value is a segment counter.
  map<size_t, size_t> m_segLength;
  /// Key is difference between two values:
  /// 1. how many meters it's necessary to go up following the feature. If feature wavy
  ///   calculates only raise meters.
  /// 2. the difference in altitude between end and start of features.
  /// Value is a segment counter.
  map<int32_t, size_t> m_featureWave;
  /// Feature counter for GetBicycleModel().IsRoad(feature) == true.
  uint32_t m_roadCount;
  /// Point counter for feature where GetBicycleModel().IsRoad(feature) == true.
  uint32_t m_roadPointCount;
  /// Feature counter for GetBicycleModel().IsRoad(feature) != true.
  uint32_t m_notRoadCount;

  Processor(generator::SrtmTileManager & manager)
    : m_srtmManager(manager), m_roadCount(0), m_roadPointCount(0), m_notRoadCount(0) {}

  void operator() (FeatureType const & f, uint32_t const & id)
  {
    f.ParseBeforeStatistic();
    if (!GetBicycleModel().IsRoad(f))
    {
      ++m_notRoadCount;
      return;
    }

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    ++m_roadCount;

    size_t const pointCount = f.GetPointsCount();
    m_roadPointCount += pointCount;

    for (int i = 0; i < pointCount; ++i)
    {
      IsNewFeatureEnd(f.GetPoint(i), m_uniqueRoadPoints);
    }

    // Feature length.
    size_t featureLengthMeters = 0;
    if (pointCount != 0)
      featureLengthMeters = MercatorBounds::DistanceOnEarth(f.GetPoint(0), f.GetPoint(pointCount - 1));
      IncrementStats(featureLengthMeters, m_featureLength);

    // Feature segment length.
    if (pointCount != 0)
    {
      for (int i = 0; i < pointCount - 1; ++i)
      {
        size_t const segmentLengthMeters = MercatorBounds::DistanceOnEarth(f.GetPoint(i), f.GetPoint(i + 1));
        IncrementStats(segmentLengthMeters, m_segLength);
      }
    }

    // Feature altitude difference.
    generator::SrtmTile::THeight startAltitude = 0, endAltitude = 0;
    if (pointCount != 0)
    {
      startAltitude = m_srtmManager.GetHeight(MercatorBounds::ToLatLon(f.GetPoint(0)));
      endAltitude = m_srtmManager.GetHeight(MercatorBounds::ToLatLon(f.GetPoint(pointCount - 1)));
    }

    int16_t const altitudeDiff = endAltitude - startAltitude;
    IncrementStats(altitudeDiff, m_altitudeDiffs);

    // Wave feature factor. Climb minus altitude difference.
    int32_t climb = 0;
    if (pointCount != 0)
    {
      for (int i = 0; i < pointCount - 1; ++i)
      {
        auto const segAltDiff = (m_srtmManager.GetHeight(MercatorBounds::ToLatLon(f.GetPoint(i + 1))) -
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
      }
      // Estimates monotony of the feature. If climb == altitudeDiff it's monotonous.
      // If not it's wavy. The more abs(climb - altitudeDiff) the more wavy the feature.
      int32_t const waveFactor = climb - altitudeDiff;
      IncrementStats(waveFactor, m_featureWave);
    }
  }
};
} // namespace

int main(int argc, char ** argv)
{
  google::SetUsageMessage("This tool extracts some staticstics about features and its altetude.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  LOG(LINFO, ("srtm_path =", FLAGS_srtm_path));
  LOG(LINFO, ("mwm_path =", FLAGS_mwm_path));

  classificator::Load();
  generator::SrtmTileManager manager(FLAGS_srtm_path);

  Processor doProcess(manager);
  feature::ForEachFromDat(FLAGS_mwm_path, doProcess);

  cout << endl << "doProcess.m_roadCount = " << doProcess.m_roadCount << endl;
  cout << "doProcess.m_uniqueRoadPoints.size = " << doProcess.m_uniqueRoadPoints.size() << endl;
  cout << "doProcess.m_roadPointCount = " << doProcess.m_roadPointCount << endl;
  cout << "doProcess.m_notRoadCount = " << doProcess.m_notRoadCount << endl;

  PrintCont(doProcess.m_altitudeDiffs, "Altitude difference between start and end of features.",
            " feature(s) with altitude difference ", " meter(s)");
  PrintCont(doProcess.m_featureLength, "Feature length.", " feature(s) with length ", " meter(s)");
  PrintCont(doProcess.m_segLength, "Feature segment length.", " segment(s) with length ", " meter(s)");
  PrintCont(doProcess.m_featureWave, "Wave factor", " features(s) with wave factor ", "");
  return 0;
}
