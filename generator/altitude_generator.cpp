#include "generator/altitude_generator.hpp"
#include "generator/routing_generator.hpp"
#include "generator/srtm_parser.hpp"

#include "routing/routing_helpers.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "geometry/latlon.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include "std/algorithm.hpp"
#include "std/type_traits.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

#include "3party/succinct/elias_fano.hpp"
#include "3party/succinct/mapper.hpp"
#include "3party/succinct/rs_bit_vector.hpp"

using namespace feature;

namespace
{
using namespace routing;

AltitudeHeader::TAltitudeSectionVersion constexpr kAltitudeSectionVersion = 1;

class SrtmGetter : public IAltitudeGetter
{
public:
  SrtmGetter(string const & srtmPath) : m_srtmManager(srtmPath) {}

  // IAltitudeGetter overrides:
  feature::TAltitude GetAltitude(m2::PointD const & p) override
  {
    return m_srtmManager.GetHeight(MercatorBounds::ToLatLon(p));
  }

private:
  generator::SrtmTileManager m_srtmManager;
};

class Processor
{
public:
  using TFeatureAltitude = pair<uint32_t, Altitude>;
  using TFeatureAltitudes = vector<TFeatureAltitude>;

  Processor(IAltitudeGetter & altitudeGetter)
    : m_altitudeGetter(altitudeGetter), m_minAltitude(kInvalidAltitude)
  {
  }

  TFeatureAltitudes const & GetFeatureAltitudes() const { return m_featureAltitudes; }

  vector<bool> const & GetAltitudeAvailability() const { return m_altitudeAvailability; }

  TAltitude GetMinAltitude() const { return m_minAltitude; }

  void operator()(FeatureType const & f, uint32_t const & id)
  {
    if (id != m_altitudeAvailability.size())
    {
      LOG(LERROR, ("There's a gap in feature id order."));
      return;
    }

    bool hasAltitude = false;
    MY_SCOPE_GUARD(removeTmpDir, [&] ()
    {
      m_altitudeAvailability.push_back(hasAltitude);
    });

    if (!routing::IsRoad(feature::TypesHolder(f)))
      return;

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = f.GetPointsCount();
    if (pointsCount == 0)
      return;

    TAltitudes altitudes;
    bool valid = true;
    TAltitude minFeatureAltitude = kInvalidAltitude;
    for (size_t i = 0; i < pointsCount; ++i)
    {
      TAltitude const a = m_altitudeGetter.GetAltitude(f.GetPoint(i));
      if (a == kInvalidAltitude)
      {
        valid = false;
        return;
      }

      if (minFeatureAltitude == kInvalidAltitude)
        minFeatureAltitude = a;
      else
        minFeatureAltitude = min(minFeatureAltitude, a);

      altitudes.push_back(a);
    }
    if (!valid)
      return;

    hasAltitude = true;
    m_featureAltitudes.push_back(make_pair(id, Altitude(move(altitudes))));

    if (m_minAltitude == kInvalidAltitude)
      m_minAltitude = minFeatureAltitude;
    else
      m_minAltitude = min(minFeatureAltitude, m_minAltitude);
  }

  bool HasAltitudeInfo() const
  {
    return !m_featureAltitudes.empty();
  }

  void SortFeatureAltitudes()
  {
    sort(m_featureAltitudes.begin(), m_featureAltitudes.end(), my::LessBy(&Processor::TFeatureAltitude::first));
  }

private:
  IAltitudeGetter & m_altitudeGetter;
  TFeatureAltitudes m_featureAltitudes;
  vector<bool> m_altitudeAvailability;
  TAltitude m_minAltitude;
};

uint32_t GetFileSize(string const & filePath)
{
  uint64_t size;
  if (!my::GetFileSize(filePath, size))
  {
    LOG(LERROR, (filePath, "Unable to get file size"));
    return 0;
  }

  LOG(LINFO, (filePath, "size =", size, "bytes"));
  return size;
}

void MoveFileToAltitudeSection(string const & filePath, uint32_t fileSize, FileWriter & w)
{
  {
    ReaderSource<FileReader> r = FileReader(filePath);
    w.Write(&fileSize, sizeof(fileSize));
    rw::ReadAndWrite(r, w);
    LOG(LINFO, (filePath, "size is", fileSize));
  }
  FileWriter::DeleteFileX(filePath);
}
}  // namespace

namespace routing
{
void BuildRoadAltitudes(string const & baseDir, string const & countryName, IAltitudeGetter & altitudeGetter)
{
  try
  {
    // Preparing altitude information.
    string const mwmPath = my::JoinFoldersToPath(baseDir, countryName + DATA_FILE_EXTENSION);

    Processor processor(altitudeGetter);
    feature::ForEachFromDat(mwmPath, processor);

    if (!processor.HasAltitudeInfo())
    {
      LOG(LINFO, ("No altitude information for road features of mwm", countryName));
      return;
    }

    processor.SortFeatureAltitudes();
    Processor::TFeatureAltitudes const & featureAltitudes = processor.GetFeatureAltitudes();

    // Writing compressed bit vector with features which have altitude information.
    succinct::rs_bit_vector altitudeAvailability(processor.GetAltitudeAvailability());
    string const altitudeAvailabilityPath = my::JoinFoldersToPath(baseDir, "altitude_availability.bitvector");
    LOG(LINFO, ("altitudeAvailability succinct::mapper::size_of(altitudeAvailability) =", succinct::mapper::size_of(altitudeAvailability)));
    succinct::mapper::freeze(altitudeAvailability, altitudeAvailabilityPath.c_str());

    // Writing feature altitude information to a file and memorizing the offsets.
    string const altitudeInfoPath = my::JoinFoldersToPath(baseDir, "altitude_info");
    vector<uint32_t> offsets;
    TAltitude const minAltitude = processor.GetMinAltitude();
    offsets.reserve(featureAltitudes.size());
    {
      FileWriter altitudeInfoW(altitudeInfoPath);
      for (auto const & a : featureAltitudes)
      {
        offsets.push_back(altitudeInfoW.Pos());
        // Feature altitude serializing.
        a.second.Serialize(minAltitude, altitudeInfoW);
      }
    }
    LOG(LINFO, ("Altitude was written for", featureAltitudes.size(), "features."));

    // Writing feature altitude offsets.
    CHECK(is_sorted(offsets.begin(), offsets.end()), ());
    CHECK(adjacent_find(offsets.begin(), offsets.end()) == offsets.end(), ());
    LOG(LINFO, ("Max altitude info offset =", offsets.back(), "number of offsets = =", offsets.size()));
    succinct::elias_fano::elias_fano_builder builder(offsets.back(), offsets.size());
    for (uint32_t offset : offsets)
      builder.push_back(offset);

    succinct::elias_fano featureTable(&builder);
    string const featuresTablePath = my::JoinFoldersToPath(baseDir, "altitude_offsets.elias_fano");
    succinct::mapper::freeze(featureTable, featuresTablePath.c_str());

    uint32_t const altitudeAvailabilitySize = GetFileSize(altitudeAvailabilityPath);
    uint32_t const altitudeInfoSize = GetFileSize(altitudeInfoPath);
    uint32_t const featuresTableSize = GetFileSize(featuresTablePath);

    FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
    FileWriter w = cont.GetWriter(ALTITUDES_FILE_TAG);

    // Writing section with altitude information.
    // Writing altitude section header.
    TAltitudeSectionOffset const headerSize = AltitudeHeader::GetHeaderSize();
    TAltitudeSectionOffset const featuresTableOffset = headerSize +
        sizeof(altitudeAvailabilitySize) + altitudeAvailabilitySize;
    TAltitudeSectionOffset const altitudeInfoOffset = featuresTableOffset +
        sizeof(featuresTableSize) + featuresTableSize;
    AltitudeHeader header(kAltitudeSectionVersion, processor.GetMinAltitude(),
                          altitudeInfoOffset + sizeof(TAltitudeSectionOffset) /* for altitude info size */);
    header.Serialize(w);

    // Copying parts of altitude sections to mwm.
    MoveFileToAltitudeSection(altitudeAvailabilityPath, altitudeAvailabilitySize, w);
    MoveFileToAltitudeSection(featuresTablePath, featuresTableSize, w);
    MoveFileToAltitudeSection(altitudeInfoPath, altitudeInfoSize, w);
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("An exception happened while creating", ALTITUDES_FILE_TAG, "section. ", e.what()));
  }
}

void BuildRoadAltitudes(string const & srtmPath, string const & baseDir, string const & countryName)
{
  LOG(LINFO, ("srtmPath =", srtmPath, "baseDir =", baseDir, "countryName =", countryName));
  SrtmGetter srtmGetter(srtmPath);
  BuildRoadAltitudes(baseDir, countryName, srtmGetter);
}
}  // namespace routing
