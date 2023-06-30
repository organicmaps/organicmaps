//#include "generator/booking_dataset.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_maker.hpp"
//#include "generator/opentable_dataset.hpp"
#include "generator/kayak_dataset.hpp"
#include "generator/osm_source.hpp"
#include "generator/raw_generator.hpp"
#include "generator/sponsored_dataset_inl.hpp"
#include "generator/translator.hpp"

#include "indexer/classificator_loader.hpp"

#include "base/file_name_utils.hpp"
#include "base/exception.hpp"
#include "base/geo_object_id.hpp"
#include "base/string_utils.hpp"

#include <fstream>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>

#include <gflags/gflags.h>

#include "boost/range/adaptor/map.hpp"
#include "boost/range/algorithm/copy.hpp"

using namespace std;

DEFINE_string(osm, "", "Input .o5m file");
DEFINE_string(booking, "", "Path to booking data in .tsv format");
DEFINE_string(opentable, "", "Path to opentable data in .tsv format");
DEFINE_string(factors, "", "Factors output path");
DEFINE_string(sample, "", "Path so sample file");

DEFINE_uint64(seed, minstd_rand::default_seed, "Seed for random shuffle");
DEFINE_uint64(selection_size, 10000, "Selection size");
DEFINE_bool(generate, false, "Generate unmarked sample");

using namespace generator;
using namespace feature;

namespace
{
string PrintBuilder(FeatureBuilder const & fb)
{
  ostringstream s;

  s << "Id: " << DebugPrint(fb.GetMostGenericOsmId()) << '\t'
    << "Name: " << fb.GetName(StringUtf8Multilang::kDefaultCode) << '\t';

  s << "Params: " << DebugPrint(fb.GetParams()) << '\t';

  auto const center = mercator::ToLatLon(fb.GetKeyPoint());
  s << "lat: " << center.m_lat << " lon: " << center.m_lon << '\t';

  if (fb.GetGeomType() == GeomType::Point)
    s << "GeomType: Point";
  else if (fb.GetGeomType() == GeomType::Area)
    s << "GeomType: Area";
  else
    CHECK(false, ());

  return s.str();
}

DECLARE_EXCEPTION(ParseError, RootException);

base::GeoObjectId ReadDebuggedPrintedOsmId(string const & str)
{
  istringstream sstr(str);
  string type;
  uint64_t id;
  sstr >> type >> id;

  if (sstr.fail())
    MYTHROW(ParseError, ("Can't make osmId from string", str));

  if (type == "node")
    return base::MakeOsmNode(id);
  if (type == "way")
    return base::MakeOsmWay(id);
  if (type == "relation")
    return base::MakeOsmRelation(id);

  MYTHROW(ParseError, ("Can't make osmId from string", str));
}

GenerateInfo GetGenerateInfo()
{
  GenerateInfo info;
  info.m_bookingDataFilename = FLAGS_booking;
  //info.m_opentableDataFilename = FLAGS_opentable;
  info.m_osmFileName = FLAGS_osm;
  info.SetNodeStorageType("map");
  info.SetOsmFileType("o5m");

  info.m_cacheDir = info.m_intermediateDir = base::GetDirectory(FLAGS_osm);

  // Set other info params here.

  return info;
}

template <typename Object>
struct SampleItem
{
  enum MatchStatus {Uninitialized, Yes, No};
  using ObjectId = typename Object::ObjectId;

  SampleItem() = default;

  SampleItem(base::GeoObjectId const & osmId, ObjectId const sponsoredId,
             MatchStatus match = Uninitialized)
    : m_osmId(osmId), m_sponsoredId(sponsoredId), m_match(match)
  {
  }

  base::GeoObjectId m_osmId;
  ObjectId m_sponsoredId = Object::InvalidObjectId();

  MatchStatus m_match = Uninitialized;
};

template <typename Object>
typename SampleItem<Object>::MatchStatus ReadMatchStatus(string_view str)
{
  if (str == "Yes")
    return SampleItem<Object>::Yes;

  if (str == "No")
    return SampleItem<Object>::No;

  if (str == "Uninitialized")
    return SampleItem<Object>::Uninitialized;

  MYTHROW(ParseError, ("Can't make SampleItem::MatchStatus from string:", str));
}

template <typename Object>
SampleItem<Object> ReadSampleItem(string const & str)
{
  SampleItem<Object> item;

  auto const parts = strings::Tokenize(str, "\t");
  CHECK_EQUAL(parts.size(), 3, ("Cant't make SampleItem from string:", str,
                                "due to wrong number of fields."));

  item.m_osmId = ReadDebuggedPrintedOsmId(string(parts[0]));
  if (!strings::to_uint(parts[1], item.m_sponsoredId.Get()))
    MYTHROW(ParseError, ("Can't make uint32 from string:", parts[1]));
  item.m_match = ReadMatchStatus<Object>(parts[2]);

  return item;
}

template <typename Object>
vector<SampleItem<Object>> ReadSample(istream & ist)
{
  vector<SampleItem<Object>> result;

  size_t lineNumber = 1;
  try
  {
    for (string line; getline(ist, line); ++lineNumber)
    {
      result.emplace_back(ReadSampleItem<Object>(line));
    }
  }
  catch (ParseError const & e)
  {
    LOG_SHORT(LERROR, ("Wrong format: line", lineNumber, e.Msg()));
    exit(1);
  }

  return result;
}

template <typename Object>
vector<SampleItem<Object>> ReadSampleFromFile(string const & name)
{
  ifstream ist(name);
  CHECK(ist.is_open(), ("Can't open file:", name, strerror(errno)));
  return ReadSample<Object>(ist);
}

void PrintOsmUrl(std::ostream & os, ms::LatLon const & ll)
{
  os << "# URL: https://www.openstreetmap.org/?mlat=" << ll.m_lat << "&mlon=" << ll.m_lon
     << "#map=18/" << ll.m_lat << "/" << ll.m_lon << endl;
};

template <typename Dataset, typename Object = typename Dataset::Object>
void GenerateFactors(Dataset const & dataset,
                     map<base::GeoObjectId, FeatureBuilder> const & features,
                     vector<SampleItem<Object>> const & sampleItems, ostream & ost)
{
  ost << fixed << setprecision(6);

  for (auto const & item : sampleItems)
  {
    auto const & object = dataset.GetStorage().GetObjectById(item.m_sponsoredId);
    auto const & feature = features.at(item.m_osmId);

    auto const score = dataset.CalcScore(object, feature);

    ost << "# ------------------------------------------" << endl;
    ost << (score.IsMatched() ? "YES" : "NO") << "\t" << DebugPrint(feature.GetMostGenericOsmId())
        << "\t" << object.m_id
        << "\tdistance: " << score.m_distance
        << "\tdistance score: " << score.m_linearNormDistanceScore
        << "\tname score: " << score.m_nameSimilarityScore
        << "\tresult score: " << score.GetMatchingScore()
        << endl;
    ost << "# " << PrintBuilder(feature) << endl;
    ost << "# " << object << endl;
    PrintOsmUrl(ost, object.m_latLon);
  }
}

enum class DatasetType
{
  Booking,
  Opentable
};

template <typename Dataset, typename Object = typename Dataset::Object>
void GenerateSample(Dataset const & dataset,
                    map<base::GeoObjectId, FeatureBuilder> const & features, ostream & ost)
{
  LOG_SHORT(LINFO, ("Num of elements:", features.size()));
  vector<base::GeoObjectId> elementIndexes(features.size());
  boost::copy(features | boost::adaptors::map_keys, begin(elementIndexes));

  // TODO(mgsergio): Try RandomSample (from search:: at the moment of writing).
  shuffle(elementIndexes.begin(), elementIndexes.end(), minstd_rand(static_cast<uint32_t>(FLAGS_seed)));
  if (FLAGS_selection_size < elementIndexes.size())
    elementIndexes.resize(FLAGS_selection_size);

  ost << fixed << setprecision(6);
  for (auto osmId : elementIndexes)
  {
    auto const & fb = features.at(osmId);
    auto const ll = mercator::ToLatLon(fb.GetKeyPoint());
    auto const sponsoredIndexes = dataset.GetStorage().GetNearestObjects(ll);

    ost << "# ------------------------------------------" << endl
        << "# " << PrintBuilder(fb) << endl;
    PrintOsmUrl(ost, ll);

    for (auto const sponsoredId : sponsoredIndexes)
    {
      auto const & object = dataset.GetStorage().GetObjectById(sponsoredId);
      auto const score = dataset.CalcScore(object, fb);

      ost << (score.IsMatched() ? "YES" : "NO") << "\t" << sponsoredId
          << "\tdistance: " << score.m_distance
          << "\tdistance score: " << score.m_linearNormDistanceScore
          << "\tname score: " << score.m_nameSimilarityScore
          << "\tresult score: " << score.GetMatchingScore()
          << endl
          << "# " << object << endl;
      PrintOsmUrl(ost, object.m_latLon);
    }

    ost << endl;
  }
}

template <typename Dataset>
string GetDatasetFilePath(GenerateInfo const & info);

template <>
string GetDatasetFilePath<KayakDataset>(GenerateInfo const & info)
{
  return info.m_bookingDataFilename;
}

//template <>
//string GetDatasetFilePath<OpentableDataset>(GenerateInfo const & info)
//{
//  return info.m_opentableDataFilename;
//}

class TranslatorMock : public Translator
{
public:
  TranslatorMock(std::shared_ptr<FeatureProcessorInterface> const & processor,
                 std::shared_ptr<generator::cache::IntermediateData> const & cache)
    : Translator(processor, cache, std::make_shared<FeatureMakerSimple>(cache->GetCache()))
  {
  }

  /// @name TranslatorInterface overrides.
  /// @{
  std::shared_ptr<TranslatorInterface> Clone() const override
  {
    UNREACHABLE();
    return nullptr;
  }
  void Merge(TranslatorInterface const &) override
  {
    UNREACHABLE();
  }
  /// @}
};

class AggregateProcessor : public FeatureProcessorInterface
{
public:
  /// @name FeatureProcessorInterface overrides.
  /// @{
  std::shared_ptr<FeatureProcessorInterface> Clone() const override
  {
    UNREACHABLE();
    return nullptr;
  }
  void Process(feature::FeatureBuilder & fb) override
  {
    auto const id = fb.GetMostGenericOsmId();
    m_features.emplace(id, std::move(fb));
  }
  void Finish() override {}
  /// @}

  std::map<base::GeoObjectId, feature::FeatureBuilder> m_features;
};

template <class Dataset> class DatasetFilter : public FilterInterface
{
  Dataset const & m_dataset;
public:
  DatasetFilter(Dataset const & dataset) : m_dataset(dataset) {}

  /// @name FilterInterface overrides.
  /// @{
  std::shared_ptr<FilterInterface> Clone() const override
  {
    UNREACHABLE();
    return nullptr;
  }
  bool IsAccepted(OsmElement const & e) const override
  {
    // All hotels under tourism tag.
    return !e.GetTag("tourism").empty();
  }
  bool IsAccepted(feature::FeatureBuilder const & fb) const override
  {
    return m_dataset.IsSponsoredCandidate(fb);
  }
  /// @}
};

template <typename Dataset, typename Object = typename Dataset::Object>
void RunImpl(GenerateInfo & info)
{
  auto const & dataSetFilePath = GetDatasetFilePath<Dataset>(info);
  Dataset dataset(dataSetFilePath);
  LOG_SHORT(LINFO, (dataset.GetStorage().Size(), "objects are loaded from a file:", dataSetFilePath));

  LOG_SHORT(LINFO, ("OSM data:", FLAGS_osm));

  generator::cache::IntermediateDataObjectsCache objectsCache;
  auto cache = std::make_shared<generator::cache::IntermediateData>(objectsCache, info);
  auto processor = make_shared<AggregateProcessor>();
  auto translator = std::make_shared<TranslatorMock>(processor, cache);
  translator->SetFilter(std::make_shared<DatasetFilter<Dataset>>(dataset));

  RawGenerator generator(info);
  generator.GenerateCustom(translator);
  CHECK(generator.Execute(), ());

  if (FLAGS_generate)
  {
    ostream * ost = &cout;
    unique_ptr<ofstream> ofst;
    if (!FLAGS_sample.empty())
    {
      ofst = std::make_unique<ofstream>(FLAGS_sample);
      CHECK(ofst->is_open(), ("Can't open file", FLAGS_sample, strerror(errno)));
      ost = ofst.get();
    }
    GenerateSample(dataset, processor->m_features, *ost);
  }
  else
  {
    auto const sample = ReadSampleFromFile<Object>(FLAGS_sample);
    LOG_SHORT(LINFO, ("Sample size is", sample.size()));
    ofstream ost(FLAGS_factors);
    CHECK(ost.is_open(), ("Can't open file", FLAGS_factors, strerror(errno)));
    GenerateFactors<Dataset>(dataset, processor->m_features, sample, ost);
  }
}

void Run(DatasetType const datasetType, GenerateInfo & info)
{
  switch (datasetType)
  {
  case DatasetType::Booking: RunImpl<KayakDataset>(info); break;
  //case DatasetType::Opentable: RunImpl<OpentableDataset>(info); break;
  }
}
}  // namespace

int main(int argc, char * argv[])
{
  gflags::SetUsageMessage("Calculates factors for given samples.");

  if (argc == 1)
  {
    gflags::ShowUsageWithFlags(argv[0]);
    exit(0);
  }

  gflags::ParseCommandLineFlags(&argc, &argv, true);

  CHECK(!FLAGS_sample.empty(), ("Please specify sample path."));
  CHECK(!FLAGS_osm.empty(), ("Please specify osm path."));
  CHECK(!FLAGS_booking.empty() || !FLAGS_opentable.empty(),
        ("Please specify either booking or opentable path."));
  CHECK(!FLAGS_factors.empty() || FLAGS_generate, ("Please either specify factors path"
                                                  "or use -generate."));

  auto const datasetType = FLAGS_booking.empty() ? DatasetType::Opentable : DatasetType::Booking;

  classificator::Load();

  auto info = GetGenerateInfo();
  GenerateIntermediateData(info);

  Run(datasetType, info);

  return 0;
}
