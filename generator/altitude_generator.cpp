#include "generator/altitude_generator.hpp"
#include "generator/srtm_parser.hpp"

#include "routing/routing_helpers.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"

#include "coding/files_container.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/varint.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <algorithm>
#include <type_traits>
#include <utility>
#include <vector>

#include "3party/succinct/elias_fano.hpp"
#include "3party/succinct/mapper.hpp"
#include "3party/succinct/rs_bit_vector.hpp"

using namespace feature;

namespace
{
using namespace routing;

class SrtmGetter : public AltitudeGetter
{
public:
  explicit SrtmGetter(std::string const & srtmDir) : m_srtmManager(srtmDir) {}

  // AltitudeGetter overrides:
  geometry::Altitude GetAltitude(m2::PointD const & p) override
  {
    return m_srtmManager.GetHeight(mercator::ToLatLon(p));
  }

private:
  generator::SrtmTileManager m_srtmManager;
};

class Processor
{
public:
  struct FeatureAltitude
  {
    FeatureAltitude() : m_featureId(0) {}
    FeatureAltitude(uint32_t featureId, Altitudes const & altitudes)
      : m_featureId(featureId), m_altitudes(altitudes)
    {
    }

    uint32_t m_featureId;
    Altitudes m_altitudes;
  };

  using TFeatureAltitudes = std::vector<FeatureAltitude>;

  explicit Processor(AltitudeGetter & altitudeGetter)
    : m_altitudeGetter(altitudeGetter), m_minAltitude(geometry::kInvalidAltitude)
  {
  }

  TFeatureAltitudes const & GetFeatureAltitudes() const { return m_featureAltitudes; }

  succinct::bit_vector_builder & GetAltitudeAvailabilityBuilder()
  {
    return m_altitudeAvailabilityBuilder;
  }

  geometry::Altitude GetMinAltitude() const { return m_minAltitude; }

  void operator()(FeatureType & f, uint32_t const & id)
  {
    if (id != m_altitudeAvailabilityBuilder.size())
    {
      LOG(LERROR, ("There's a gap in feature id order."));
      return;
    }

    bool hasAltitude = false;
    SCOPE_GUARD(altitudeAvailabilityBuilding,
                [&]() { m_altitudeAvailabilityBuilder.push_back(hasAltitude); });

    if (!routing::IsRoad(feature::TypesHolder(f)))
      return;

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = f.GetPointsCount();
    if (pointsCount == 0)
      return;

    geometry::Altitudes altitudes;
    geometry::Altitude minFeatureAltitude = geometry::kInvalidAltitude;
    for (size_t i = 0; i < pointsCount; ++i)
    {
      geometry::Altitude const a = m_altitudeGetter.GetAltitude(f.GetPoint(i));
      if (a == geometry::kInvalidAltitude)
      {
        // One invalid point invalidates the whole feature.
        return;
      }

      if (minFeatureAltitude == geometry::kInvalidAltitude)
        minFeatureAltitude = a;
      else
        minFeatureAltitude = std::min(minFeatureAltitude, a);

      altitudes.push_back(a);
    }

    hasAltitude = true;
    m_featureAltitudes.emplace_back(id, Altitudes(std::move(altitudes)));

    if (m_minAltitude == geometry::kInvalidAltitude)
      m_minAltitude = minFeatureAltitude;
    else
      m_minAltitude = std::min(minFeatureAltitude, m_minAltitude);
  }

  bool HasAltitudeInfo() const { return !m_featureAltitudes.empty(); }

  bool IsFeatureAltitudesSorted()
  {
    return std::is_sorted(m_featureAltitudes.begin(), m_featureAltitudes.end(),
                          base::LessBy(&Processor::FeatureAltitude::m_featureId));
  }

private:
  AltitudeGetter & m_altitudeGetter;
  TFeatureAltitudes m_featureAltitudes;
  succinct::bit_vector_builder m_altitudeAvailabilityBuilder;
  geometry::Altitude m_minAltitude;
};
}  // namespace

namespace routing
{
void BuildRoadAltitudes(std::string const & mwmPath, AltitudeGetter & altitudeGetter)
{
  try
  {
    // Preparing altitude information.
    Processor processor(altitudeGetter);
    feature::ForEachFeature(mwmPath, processor);

    if (!processor.HasAltitudeInfo())
    {
      LOG(LINFO, ("No altitude information for road features of mwm:", mwmPath));
      return;
    }

    CHECK(processor.IsFeatureAltitudesSorted(), ());

    FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
    auto w = cont.GetWriter(ALTITUDES_FILE_TAG);

    AltitudeHeader header;
    header.m_minAltitude = processor.GetMinAltitude();

    auto const startOffset = w->Pos();
    header.Serialize(*w);
    {
      // Altitude availability serialization.
      coding::FreezeVisitor<Writer> visitor(*w);
      succinct::bit_vector_builder & builder = processor.GetAltitudeAvailabilityBuilder();
      succinct::rs_bit_vector(&builder).map(visitor);
    }
    header.m_featureTableOffset = base::checked_cast<uint32_t>(w->Pos() - startOffset);

    std::vector<uint32_t> offsets;
    std::vector<uint8_t> deltas;
    {
      // Altitude info serialization to memory.
      MemWriter<std::vector<uint8_t>> writer(deltas);
      Processor::TFeatureAltitudes const & featureAltitudes = processor.GetFeatureAltitudes();
      for (auto const & a : featureAltitudes)
      {
        offsets.push_back(base::checked_cast<uint32_t>(writer.Pos()));
        a.m_altitudes.Serialize(header.m_minAltitude, writer);
      }
    }
    {
      // Altitude offsets serialization.
      CHECK(std::is_sorted(offsets.begin(), offsets.end()), ());
      CHECK(adjacent_find(offsets.begin(), offsets.end()) == offsets.end(), ());

      succinct::elias_fano::elias_fano_builder builder(offsets.back(), offsets.size());
      for (uint32_t offset : offsets)
        builder.push_back(offset);

      coding::FreezeVisitor<Writer> visitor(*w);
      succinct::elias_fano(&builder).map(visitor);
    }
    // Writing altitude info.
    header.m_altitudesOffset = base::checked_cast<uint32_t>(w->Pos() - startOffset);
    w->Write(deltas.data(), deltas.size());
    w->WritePaddingByEnd(8);
    header.m_endOffset = base::checked_cast<uint32_t>(w->Pos() - startOffset);

    // Rewriting header info.
    auto const endOffset = w->Pos();
    w->Seek(startOffset);
    header.Serialize(w);
    w->Seek(endOffset);
    LOG(LINFO, (ALTITUDES_FILE_TAG, "section is ready. The size is", header.m_endOffset));
    if (processor.HasAltitudeInfo())
      LOG(LINFO, ("Min altitude is", processor.GetMinAltitude()));
    else
      LOG(LINFO, ("Min altitude isn't defined."));
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("An exception happened while creating", ALTITUDES_FILE_TAG, "section:", e.what()));
  }
}

void BuildRoadAltitudes(std::string const & mwmPath, std::string const & srtmDir)
{
  LOG(LINFO, ("mwmPath =", mwmPath, "srtmDir =", srtmDir));
  SrtmGetter srtmGetter(srtmDir);
  BuildRoadAltitudes(mwmPath, srtmGetter);
}
}  // namespace routing
