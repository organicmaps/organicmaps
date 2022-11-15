#pragma once

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
  WaterBoundaryChecker(std::string const & rawGeometryFileName)
  {
    LoadWaterGeometry(rawGeometryFileName);
  }

  ~WaterBoundaryChecker()
  {
    LOG_SHORT(LINFO, ("Features checked:", m_totalFeatures, "borders checked:", m_totalBorders,
                "borders skipped:", m_skippedBorders, "selected polygons:", m_selectedPolygons));
  }

  void LoadWaterGeometry(std::string const & rawGeometryFileName)
  {
    LOG_SHORT(LINFO, ("Loading water geometry:", rawGeometryFileName));
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
        m_tree.Add(m2::RegionD(move(points)));
      }
    }
    LOG_SHORT(LINFO, ("Load", total, "water geometries"));
  }

  bool IsBoundaries(feature::FeatureBuilder const & fb)
  {
    ++m_totalFeatures;

    auto static const kBoundaryType = classif().GetTypeByPath({"boundary", "administrative"});
    if (fb.FindType(kBoundaryType, 2) == ftype::GetEmptyValue())
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

  void ProcessBoundary(feature::FeatureBuilder const & boundary, std::vector<feature::FeatureBuilder> & parts)
  {
    auto const & line = boundary.GetGeometry().front();

    double constexpr kExtension = 0.01;
    ProcessState state = ProcessState::Initial;

    feature::FeatureBuilder::PointSeq points;

    for (size_t i = 0; i < line.size(); ++i)
    {
      m2::PointD const & p = line[i];
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
            parts.back().ResetGeometry();
            for (auto const & pt : points)
              parts.back().AddPoint(pt);
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
      parts.back().ResetGeometry();
      for (auto const & pt : points)
        parts.back().AddPoint(pt);
    }

    if (parts.empty())
      m_skippedBorders++;
  }
};
