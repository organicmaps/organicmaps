#include "generator/raw_generator.hpp"

//#include "generator/complex_loader.hpp"
#include "generator/features_processing_helpers.hpp"
#include "generator/final_processor_cities.hpp"
#include "generator/final_processor_coastline.hpp"
#include "generator/final_processor_country.hpp"
#include "generator/final_processor_world.hpp"
#include "generator/osm_source.hpp"
#include "generator/processor_factory.hpp"
#include "generator/raw_generator_writer.hpp"
#include "generator/translator_factory.hpp"
#include "generator/translators_pool.hpp"

#include "base/timer.hpp"

#include "defines.hpp"

namespace generator
{
namespace
{
class Stats
{
public:
  Stats(size_t logCallCountThreshold)
    : m_timer(true /* start */), m_logCallCountThreshold(logCallCountThreshold)
  {
  }

  void Log(std::vector<OsmElement> const & elements, uint64_t pos, bool forcePrint = false)
  {
    for (auto const & e : elements)
    {
      if (e.IsNode())
        ++m_nodeCounter;
      else if (e.IsWay())
        ++m_wayCounter;
      else if (e.IsRelation())
        ++m_relationCounter;
    }

    m_element_counter += elements.size();
    if (!forcePrint && m_callCount != m_logCallCountThreshold)
    {
      ++m_callCount;
      return;
    }

    auto static constexpr kBytesInMiB = 1024.0 * 1024.0;
    auto const posMiB = pos / kBytesInMiB;
    auto const elapsedSeconds = m_timer.ElapsedSeconds();
    auto const avgSpeedMiBPerSec = posMiB / elapsedSeconds;
    auto const speedMiBPerSec =
        (pos - m_prevFilePos) / (elapsedSeconds - m_prevElapsedSeconds) / kBytesInMiB;

    LOG(LINFO, ("Read", m_element_counter, "elements [pos:", posMiB,
                "MiB, avg read speed:", avgSpeedMiBPerSec, " MiB/s, read speed:", speedMiBPerSec,
                "MiB/s [n:", m_nodeCounter, ", w:", m_wayCounter, ", r:", m_relationCounter, "]]"));

    m_prevFilePos = pos;
    m_prevElapsedSeconds = elapsedSeconds;
    m_nodeCounter = 0;
    m_wayCounter = 0;
    m_relationCounter = 0;
    m_callCount = 0;
  }

private:
  base::Timer m_timer;
  size_t const m_logCallCountThreshold = 0;
  size_t m_callCount = 0;
  uint64_t m_prevFilePos = 0;
  double m_prevElapsedSeconds = 0.0;
  size_t m_element_counter = 0;
  size_t m_nodeCounter = 0;
  size_t m_wayCounter = 0;
  size_t m_relationCounter = 0;
};
}  // namespace

RawGenerator::RawGenerator(feature::GenerateInfo & genInfo, size_t threadsCount, size_t chunkSize)
  : m_genInfo(genInfo)
  , m_threadsCount(threadsCount)
  , m_chunkSize(chunkSize)
  , m_cache(std::make_shared<generator::cache::IntermediateData>(m_intermediateDataObjectsCache, genInfo))
  , m_queue(std::make_shared<FeatureProcessorQueue>())
  , m_translators(std::make_shared<TranslatorCollection>())
{
}

void RawGenerator::ForceReloadCache()
{
  m_intermediateDataObjectsCache.Clear();
  m_cache = std::make_shared<cache::IntermediateData>(m_intermediateDataObjectsCache, m_genInfo);
}

void RawGenerator::GenerateCountries(bool isTests/* = false*/)
{
//  if (!m_genInfo.m_complexHierarchyFilename.empty())
//    m_hierarchyNodesSet = GetOrCreateComplexLoader(m_genInfo.m_complexHierarchyFilename).GetIdsSet();
//  auto const complexFeaturesMixer = std::make_shared<ComplexFeaturesMixer>(m_hierarchyNodesSet);

  AffiliationInterfacePtr affiliation;
  if (isTests)
  {
    affiliation = std::make_shared<feature::SingleAffiliation>(m_genInfo.m_fileName);
  }
  else
  {
    affiliation = std::make_shared<feature::CountriesFilesIndexAffiliation>(
        m_genInfo.m_targetDir, m_genInfo.m_haveBordersForWholeWorld);
  }

  auto processor = CreateProcessor(ProcessorType::Country, affiliation, m_queue);

  /// @todo Better design is to have one Translator that creates FeatureBuilder from OsmElement
  /// and dispatches FB into Coastline, World, Country, City processors.
  /// Now we have at least 2x similar work in OsmElement->GetNameAndType->FeatureBuilder (for Country and World).

  m_translators->Append(CreateTranslator(TranslatorType::Country, processor, m_cache, m_genInfo,
                                         isTests ? nullptr : affiliation));

  m_finalProcessors.emplace(CreateCountryFinalProcessor(affiliation, false));
  m_finalProcessors.emplace(CreatePlacesFinalProcessor(affiliation));
}

void RawGenerator::GenerateWorld(bool cutBordersByWater/* = true */)
{
  auto processor = CreateProcessor(ProcessorType::World, m_queue, m_genInfo.m_popularPlacesFilename);
  m_translators->Append(CreateTranslator(TranslatorType::World, processor, m_cache, m_genInfo));
  m_finalProcessors.emplace(CreateWorldFinalProcessor(cutBordersByWater));
}

void RawGenerator::GenerateCoasts()
{
  auto processor = CreateProcessor(ProcessorType::Coastline, m_queue);
  m_translators->Append(CreateTranslator(TranslatorType::Coastline, processor, m_cache));
  m_finalProcessors.emplace(CreateCoslineFinalProcessor());
}

void RawGenerator::GenerateCustom(std::shared_ptr<TranslatorInterface> const & translator)
{
  m_translators->Append(translator);
}

void RawGenerator::GenerateCustom(
    std::shared_ptr<TranslatorInterface> const & translator,
    std::shared_ptr<FinalProcessorIntermediateMwmInterface> const & finalProcessor)
{
  m_translators->Append(translator);
  m_finalProcessors.emplace(finalProcessor);
}

bool RawGenerator::Execute()
{
  if (!GenerateFilteredFeatures())
    return false;

  m_translators.reset();
  m_cache.reset();
  m_queue.reset();
  m_intermediateDataObjectsCache.Clear();

  while (!m_finalProcessors.empty())
  {
    auto const finalProcessor = m_finalProcessors.top();
    m_finalProcessors.pop();
    finalProcessor->Process();
  }

  LOG(LINFO, ("Final processing is finished."));
  return true;
}

RawGenerator::FinalProcessorPtr RawGenerator::CreateCoslineFinalProcessor()
{
  auto finalProcessor = std::make_shared<CoastlineFinalProcessor>(
      m_genInfo.GetTmpFileName(WORLD_COASTS_FILE_NAME, DATA_FILE_EXTENSION_TMP), m_threadsCount);
  finalProcessor->SetCoastlinesFilenames(
      m_genInfo.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, ".geom"),
      m_genInfo.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, RAW_GEOM_FILE_EXTENSION));
  return finalProcessor;
}

RawGenerator::FinalProcessorPtr RawGenerator::CreateCountryFinalProcessor(
    AffiliationInterfacePtr const & affiliations, bool addAds)
{
  auto finalProcessor = std::make_shared<CountryFinalProcessor>(affiliations, m_genInfo.m_tmpDir, m_threadsCount);
  finalProcessor->SetIsolinesDir(m_genInfo.m_isolinesDir);
  finalProcessor->SetAddressesDir(m_genInfo.m_addressesDir);
  finalProcessor->SetMiniRoundabouts(m_genInfo.GetIntermediateFileName(MINI_ROUNDABOUTS_FILENAME));
  finalProcessor->SetAddrInterpolation(m_genInfo.GetIntermediateFileName(ADDR_INTERPOL_FILENAME));
  if (addAds)
    finalProcessor->SetFakeNodes(base::JoinPath(GetPlatform().ResourcesDir(), MIXED_NODES_FILE));

  if (m_genInfo.m_emitCoasts)
  {
    finalProcessor->SetCoastlines(
        m_genInfo.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, ".geom"),
        m_genInfo.GetTmpFileName(WORLD_COASTS_FILE_NAME));
  }

  finalProcessor->SetCityBoundariesFiles(m_genInfo.GetIntermediateFileName(CITY_BOUNDARIES_COLLECTOR_FILENAME));
  return finalProcessor;
}

RawGenerator::FinalProcessorPtr RawGenerator::CreateWorldFinalProcessor(bool cutBordersByWater)
{
  std::string coastlineGeom;
  if (cutBordersByWater)
  {
    // This file should exist or read exception will be thrown otherwise.
    coastlineGeom  = m_genInfo.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, RAW_GEOM_FILE_EXTENSION);
  }
  auto finalProcessor = std::make_shared<WorldFinalProcessor>(m_genInfo.m_tmpDir, coastlineGeom);

  finalProcessor->SetPopularPlaces(m_genInfo.m_popularPlacesFilename);
  return finalProcessor;
}

RawGenerator::FinalProcessorPtr RawGenerator::CreatePlacesFinalProcessor(AffiliationInterfacePtr const & affiliations)
{
  auto finalProcessor = std::make_shared<FinalProcessorCities>(affiliations, m_genInfo.m_tmpDir, m_threadsCount);
  finalProcessor->SetCityBoundariesFiles(m_genInfo.GetIntermediateFileName(CITY_BOUNDARIES_COLLECTOR_FILENAME),
                                         m_genInfo.m_citiesBoundariesFilename);
  return finalProcessor;
}

bool RawGenerator::GenerateFilteredFeatures()
{
  SourceReader reader =
      m_genInfo.m_osmFileName.empty() ? SourceReader() : SourceReader(m_genInfo.m_osmFileName);

  std::unique_ptr<ProcessorOsmElementsInterface> sourceProcessor;
  switch (m_genInfo.m_osmFileType)
  {
  case feature::GenerateInfo::OsmSourceType::O5M:
    sourceProcessor = std::make_unique<ProcessorOsmElementsFromO5M>(reader);
    break;
  case feature::GenerateInfo::OsmSourceType::XML:
    sourceProcessor = std::make_unique<ProcessorOsmElementsFromXml>(reader);
    break;
  }
  CHECK(sourceProcessor, ());

  TranslatorsPool translators(m_translators, m_threadsCount);
  RawGeneratorWriter rawGeneratorWriter(m_queue, m_genInfo.m_tmpDir);
  rawGeneratorWriter.Run();

  Stats stats(100 * m_threadsCount /* logCallCountThreshold */);

  bool isEnd = false;
  do
  {
    std::vector<OsmElement> elements(m_chunkSize);
    size_t idx = 0;
    while (idx < m_chunkSize && sourceProcessor->TryRead(elements[idx]))
      ++idx;

    isEnd = idx < m_chunkSize;
    stats.Log(elements, reader.Pos(), isEnd/* forcePrint */);

    if (isEnd)
      elements.resize(idx);
    translators.Emit(std::move(elements));

  } while (!isEnd);

  LOG(LINFO, ("Input was processed."));
  if (!translators.Finish())
    return false;

  rawGeneratorWriter.ShutdownAndJoin();
  m_names = rawGeneratorWriter.GetNames();
  /// @todo: compare to the input list of countries loaded in borders::LoadCountriesList().
  if (m_names.empty())
    LOG(LWARNING, ("No feature data " DATA_FILE_EXTENSION_TMP " files were generated for any country!"));
  else
    LOG(LINFO, ("Feature data " DATA_FILE_EXTENSION_TMP " files were written for following countries:", m_names));

  return true;
}
}  // namespace generator
