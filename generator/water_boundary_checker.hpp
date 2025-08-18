#pragma once
#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"

#include "geometry/region2d.hpp"
#include "geometry/tree4d.hpp"

class WaterBoundaryChecker
{
  struct RegionTraits
  {
    m2::RectD const & LimitRect(m2::RegionD const & r) const { return r.GetRect(); }
  };
  m4::Tree<m2::RegionD, RegionTraits> m_tree;

  size_t m_totalFeatures = 0;
  size_t m_totalBorders = 0;
  size_t m_skippedBorders = 0;
  size_t m_selectedPolygons = 0;

public:
  ~WaterBoundaryChecker()
  {
    LOG(LINFO, ("Features checked:", m_totalFeatures, "borders checked:", m_totalBorders,
                "borders skipped:", m_skippedBorders, "selected polygons:", m_selectedPolygons));
  }

  void LoadWaterGeometry(std::string const & rawGeometryFileName)
  {
    LOG(LINFO, ("Loading water geometry:", rawGeometryFileName));
    FileReader reader(rawGeometryFileName);
    ReaderSource<FileReader> file(reader);

    size_t total = 0;
    while (true)
    {
      uint64_t numGeometries = 0;
      file.Read(&numGeometries, sizeof(numGeometries));

      if (numGeometries == 0)
        break;

      ++total;

      for (size_t i = 0; i < numGeometries; ++i)
      {
        uint64_t numPoints = 0;
        file.Read(&numPoints, sizeof(numPoints));

        std::vector<m2::PointD> points(numPoints);
        file.Read(points.data(), sizeof(m2::PointD) * numPoints);
        m_tree.Add(m2::RegionD(std::move(points)));
      }
    }
    LOG(LINFO, ("Load", total, "water geometries"));
  }

  bool IsBoundaries(feature::FeatureBuilder const & fb)
  {
    ++m_totalFeatures;

    static uint32_t const kBoundaryType = classif().GetTypeByPath({"boundary", "administrative"});
    if (!fb.IsLine() || !fb.HasType(kBoundaryType, 2))
      return false;

    ++m_totalBorders;
    return true;
  }

  enum class ProcessState
  {
    Initial,
    Water,
    Earth
  };

  // TODO(pastk): boundaries along the coast are being "torn" into small pieces instead of being discarded completely.
  // Likely it happens because an already simplified coastline is used, while boundary lines are not simplified yet.
  // It causes these lines to intersect each other often.
  // https://github.com/organicmaps/organicmaps/issues/6445
  void ProcessBoundary(feature::FeatureBuilder const & boundary, std::vector<feature::FeatureBuilder> & parts)
  {
    double constexpr kExtension = 0.01;
    ProcessState state = ProcessState::Initial;

    feature::FeatureBuilder::PointSeq points;

    for (auto const & p : boundary.GetOuterGeometry())
    {
      m2::RectD r(p.x - kExtension, p.y - kExtension, p.x + kExtension, p.y + kExtension);
      size_t hits = 0;
      m_tree.ForEachInRect(r, [&](m2::RegionD const & rgn)
      {
        ++m_selectedPolygons;
        hits += rgn.Contains(p) ? 1 : 0;
      });

      bool inWater = (hits & 0x01) == 1;

      switch (state)
      {
      case ProcessState::Initial:
      {
        if (inWater)
        {
          state = ProcessState::Water;
        }
        else
        {
          points.push_back(p);
          state = ProcessState::Earth;
        }
        break;
      }
      case ProcessState::Water:
      {
        if (inWater)
        {
          // do nothing
        }
        else
        {
          points.push_back(p);
          state = ProcessState::Earth;
        }
        break;
      }
      case ProcessState::Earth:
      {
        if (inWater)
        {
          if (points.size() > 1)
          {
            parts.push_back(boundary);
            parts.back().AssignPoints(std::move(points));
          }
          points.clear();
          state = ProcessState::Water;
        }
        else
        {
          points.push_back(p);
        }
        break;
      }
      }
    }

    if (points.size() > 1)
    {
      parts.push_back(boundary);
      parts.back().AssignPoints(std::move(points));
    }

    if (parts.empty())
      m_skippedBorders++;
  }
};
