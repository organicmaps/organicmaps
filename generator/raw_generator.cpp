#include "generator/raw_generator.hpp"

#include "generator/osm_source.hpp"
#include "generator/processor_factory.hpp"
#include "generator/raw_generator_writer.hpp"
#include "generator/translator_factory.hpp"
#include "generator/translators_pool.hpp"

#include "base/thread_pool_computational.hpp"

#include "defines.hpp"

namespace generator
{
RawGenerator::RawGenerator(feature::GenerateInfo & genInfo, size_t threadsCount, size_t chankSize)
  : m_genInfo(genInfo)
  , m_threadsCount(threadsCount)
  , m_chankSize(chankSize)
  , m_cache(std::make_shared<generator::cache::IntermediateData>(genInfo))
  , m_queue(std::make_shared<FeatureProcessorQueue>())
  , m_translators(std::make_shared<TranslatorCollection>())
{
}

void RawGenerator::ForceReloadCache()
{
  m_cache = std::make_shared<cache::IntermediateData>(m_genInfo, true /* forceReload */);
}

std::shared_ptr<FeatureProcessorQueue> RawGenerator::GetQueue()
{
  return m_queue;
}

void RawGenerator::GenerateCountries(bool disableAds)
{
  auto processor = CreateProcessor(ProcessorType::Country, m_queue, m_genInfo.m_targetDir, "",
                                   m_genInfo.m_haveBordersForWholeWorld);
  auto const translatorType = disableAds ? TranslatorType::Country : TranslatorType::CountryWithAds;
  m_translators->Append(CreateTranslator(translatorType, processor, m_cache, m_genInfo));
  m_finalProcessors.emplace(CreateCountryFinalProcessor());
}

void RawGenerator::GenerateWorld(bool disableAds)
{
  auto processor = CreateProcessor(ProcessorType::World, m_queue, m_genInfo.m_popularPlacesFilename);
  auto const translatorType = disableAds ? TranslatorType::World : TranslatorType::WorldWithAds;
  m_translators->Append(CreateTranslator(translatorType, processor, m_cache, m_genInfo));
  m_finalProcessors.emplace(CreateWorldFinalProcessor());
}

void RawGenerator::GenerateCoasts()
{
  auto processor = CreateProcessor(ProcessorType::Coastline, m_queue);
  m_translators->Append(CreateTranslator(TranslatorType::Coastline, processor, m_cache));
  m_finalProcessors.emplace(CreateCoslineFinalProcessor());
}

void RawGenerator::GenerateRegionFeatures(string const & filename)
{
  auto processor = CreateProcessor(ProcessorType::Simple, m_queue, filename);
  m_translators->Append(CreateTranslator(TranslatorType::Regions, processor, m_cache, m_genInfo));
}

void RawGenerator::GenerateStreetsFeatures(string const & filename)
{
  auto processor = CreateProcessor(ProcessorType::Simple, m_queue, filename);
  m_translators->Append(CreateTranslator(TranslatorType::Streets, processor, m_cache));
}

void RawGenerator::GenerateGeoObjectsFeatures(string const & filename)
{
  auto processor = CreateProcessor(ProcessorType::Simple, m_queue, filename);
  m_translators->Append(CreateTranslator(TranslatorType::GeoObjects, processor, m_cache));
}

void RawGenerator::GenerateCustom(std::shared_ptr<TranslatorInterface> const & translator)
{
  m_translators->Append(translator);
}

void RawGenerator::GenerateCustom(std::shared_ptr<TranslatorInterface> const & translator,
                                  std::shared_ptr<FinalProcessorIntermediateMwmInterface> const & finalProcessor)
{
  m_translators->Append(translator);
  m_finalProcessors.emplace(finalProcessor);
}

bool RawGenerator::Execute()
{
  if (!GenerateFilteredFeatures())
    return false;

  while (!m_finalProcessors.empty())
  {
    base::thread_pool::computational::ThreadPool threadPool(m_threadsCount);
    do
    {
      auto const & finalProcessor = m_finalProcessors.top();
      threadPool.SubmitWork([finalProcessor{finalProcessor}]() {
        finalProcessor->Process();
      });
      m_finalProcessors.pop();
      if (m_finalProcessors.empty() || *finalProcessor != *m_finalProcessors.top())
        break;
    }
    while (true);
  }

  LOG(LINFO, ("Final processing is finished."));
  return true;
}

std::vector<std::string> RawGenerator::GetNames() const
{
  return m_names;
}

RawGenerator::FinalProcessorPtr RawGenerator::CreateCoslineFinalProcessor()
{
  auto finalProcessor = make_shared<CoastlineFinalProcessor>(
                          m_genInfo.GetTmpFileName(WORLD_COASTS_FILE_NAME, DATA_FILE_EXTENSION_TMP));
  finalProcessor->SetCoastlinesFilenames(
        m_genInfo.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, ".geom"),
        m_genInfo.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, RAW_GEOM_FILE_EXTENSION));
  return finalProcessor;
}

RawGenerator::FinalProcessorPtr RawGenerator::CreateCountryFinalProcessor()
{
  auto finalProcessor = make_shared<CountryFinalProcessor>(m_genInfo.m_targetDir, m_genInfo.m_tmpDir,
                                                           m_genInfo.m_haveBordersForWholeWorld,
                                                           m_threadsCount);
  finalProcessor->SetBooking(m_genInfo.m_bookingDataFilename);
  finalProcessor->SetCitiesAreas(m_genInfo.GetIntermediateFileName(CITIES_AREAS_TMP_FILENAME));
  finalProcessor->SetPromoCatalog(m_genInfo.m_promoCatalogCitiesFilename);
  if (m_genInfo.m_emitCoasts)
  {
    finalProcessor->SetCoastlines(m_genInfo.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, ".geom"),
                                  m_genInfo.GetTmpFileName(WORLD_COASTS_FILE_NAME));
  }

  finalProcessor->DumpCitiesBoundaries(m_genInfo.m_citiesBoundariesFilename);
  return finalProcessor;
}

RawGenerator::FinalProcessorPtr RawGenerator::CreateWorldFinalProcessor()
{
  auto finalProcessor = make_shared<WorldFinalProcessor>(
                          m_genInfo.m_tmpDir,
                          m_genInfo.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, RAW_GEOM_FILE_EXTENSION));
  finalProcessor->SetPopularPlaces(m_genInfo.m_popularPlacesFilename);
  finalProcessor->SetCitiesAreas(m_genInfo.GetIntermediateFileName(CITIES_AREAS_TMP_FILENAME));
  finalProcessor->SetPromoCatalog(m_genInfo.m_promoCatalogCitiesFilename);
  return finalProcessor;
}

bool RawGenerator::GenerateFilteredFeatures()
{
  SourceReader reader = m_genInfo.m_osmFileName.empty() ? SourceReader()
                                                        : SourceReader(m_genInfo.m_osmFileName);

  unique_ptr<ProcessorOsmElementsInterface> sourseProcessor;
  switch (m_genInfo.m_osmFileType) {
  case feature::GenerateInfo::OsmSourceType::O5M:
    sourseProcessor = make_unique<ProcessorOsmElementsFromO5M>(reader);
    break;
  case feature::GenerateInfo::OsmSourceType::XML:
    sourseProcessor = make_unique<ProcessorXmlElementsFromXml>(reader);
    break;
  }

  TranslatorsPool translators(m_translators, m_cache, m_threadsCount - 1 /* copyCount */);
  RawGeneratorWriter rawGeneratorWriter(m_queue, m_genInfo.m_tmpDir);
  rawGeneratorWriter.Run();

  size_t element_pos = 0;
  std::vector<OsmElement> elements(m_chankSize);
  while(sourseProcessor->TryRead(elements[element_pos]))
  {
    if (++element_pos != m_chankSize)
      continue;

    translators.Emit(std::move(elements));
    elements = vector<OsmElement>(m_chankSize);
    element_pos = 0;
  }
  elements.resize(element_pos);
  translators.Emit(std::move(elements));

  LOG(LINFO, ("Input was processed."));
  if (!translators.Finish())
    return false;

  m_names = rawGeneratorWriter.GetNames();
  LOG(LINFO, ("Names:", m_names));
  return true;
}
}  // namespace generator
