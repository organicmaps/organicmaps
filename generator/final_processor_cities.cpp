#include "generator/final_processor_cities.hpp"
#include "generator/filter_world.hpp"
#include "generator/final_processor_utils.hpp"
#include "generator/place_processor.hpp"

#include <mutex>

namespace generator
{

FinalProcessorCities::FinalProcessorCities(AffiliationInterfacePtr const & affiliation, std::string const & mwmPath,
                                           size_t threadsCount)
  : FinalProcessorIntermediateMwmInterface(FinalProcessorPriority::Places)
  , m_temporaryMwmPath(mwmPath)
  , m_affiliation(affiliation)
  , m_threadsCount(threadsCount)
{}

void FinalProcessorCities::Process()
{
  using namespace feature;

  std::mutex mutex;
  auto const & localityChecker = ftypes::IsLocalityChecker::Instance();

  PlaceProcessor processor(m_boundariesCollectorFile);
  ForEachMwmTmp(m_temporaryMwmPath, [&](auto const & country, auto const & path)
  {
    if (!m_affiliation->HasCountryByName(country))
      return;

    std::vector<FeatureBuilder> cities;
    FeatureBuilderWriter writer(path, true /* mangleName */);
    ForEachFeatureRawFormat(path, [&](FeatureBuilder && fb, uint64_t)
    {
      if (localityChecker.GetType(fb.GetTypes()) < ftypes::LocalityType::City)
        writer.Write(fb);
      else
        cities.emplace_back(std::move(fb));
    });

    std::lock_guard<std::mutex> lock(mutex);
    for (auto & city : cities)
      processor.Add(std::move(city));
  }, m_threadsCount);

  auto const & result = processor.ProcessPlaces();
  AppendToMwmTmp(result, *m_affiliation, m_temporaryMwmPath, m_threadsCount);

  {
    FeatureBuilderWriter writer(base::JoinPath(m_temporaryMwmPath, WORLD_FILE_NAME DATA_FILE_EXTENSION_TMP),
                                FileWriter::Op::OP_APPEND);
    for (auto const & fb : result)
      if (FilterWorld::IsGoodScale(fb))
        writer.Write(fb);
  }

  if (!m_boundariesOutFile.empty())
  {
    LOG(LINFO, ("Dumping cities boundaries to", m_boundariesOutFile));
    SerializeBoundariesTable(m_boundariesOutFile, processor.GetTable());
  }
}

}  // namespace generator
