#include "search_engine.hpp"
#include "category_info.hpp"
#include "result.hpp"
#include "search_query.hpp"

#include "../storage/country_info.hpp"

#include "../indexer/categories_holder.hpp"
#include "../indexer/search_delimiters.hpp"
#include "../indexer/search_string_utils.hpp"
#include "../indexer/mercator.hpp"

#include "../geometry/distance_on_sphere.hpp"

#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"
#include "../std/function.hpp"
#include "../std/map.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"


namespace search
{

class EngineData
{
public:
  EngineData(ModelReaderPtr polyR, ModelReaderPtr countryR)
    : m_infoGetter(polyR, countryR) {}

  multimap<strings::UniString, uint32_t> m_categories;
  vector<pair<strings::UniString, uint8_t> > m_stringsToSuggest;
  storage::CountryInfoGetter m_infoGetter;
};

Engine::Engine(IndexType const * pIndex, CategoriesHolder * pCategories,
               ModelReaderPtr polyR, ModelReaderPtr countryR)
  : m_trackEnable(false), m_pIndex(pIndex), m_pData(new EngineData(polyR, countryR))
{
  if (pCategories)
  {
    InitializeCategoriesAndSuggestStrings(*pCategories);
    delete pCategories;
  }

  m_pQuery.reset(new Query(pIndex,
                           &m_pData->m_categories,
                           &m_pData->m_stringsToSuggest,
                           &m_pData->m_infoGetter));
}

Engine::~Engine()
{
}

void Engine::InitializeCategoriesAndSuggestStrings(CategoriesHolder const & categories)
{
  m_pData->m_categories.clear();
  m_pData->m_stringsToSuggest.clear();

  map<strings::UniString, uint8_t> stringsToSuggest;
  for (CategoriesHolder::const_iterator it = categories.begin(); it != categories.end(); ++it)
  {
    for (size_t i = 0; i < it->m_synonyms.size(); ++i)
    {
      CategoriesHolder::Category::Name const & name = it->m_synonyms[i];
      strings::UniString const uniName = NormalizeAndSimplifyString(name.m_name);

      uint8_t & score = stringsToSuggest[uniName];
      if (score == 0 || score > name.m_prefixLengthToSuggest)
        score = name.m_prefixLengthToSuggest;

      vector<strings::UniString> tokens;
      SplitUniString(uniName, MakeBackInsertFunctor(tokens), Delimiters());
      for (size_t j = 0; j < tokens.size(); ++j)
        for (size_t k = 0; k < it->m_types.size(); ++k)
          m_pData->m_categories.insert(make_pair(tokens[j], it->m_types[k]));
    }
  }

  m_pData->m_stringsToSuggest.assign(stringsToSuggest.begin(), stringsToSuggest.end());
}

void Engine::SetViewport(m2::RectD const & viewport)
{
  m_savedViewport = viewport;

  if (!m_trackEnable)
  {
    m_pQuery->SetViewport(viewport);
    m_pQuery->SetPosition(viewport.Center());
  }
}

void Engine::SetPosition(double lat, double lon)
{
  m2::PointD const oldPos = m_pQuery->GetPosition();

  if (m_trackEnable &&
      ms::DistanceOnEarth(MercatorBounds::YToLat(oldPos.y),
                          MercatorBounds::XToLon(oldPos.x),
                          lat, lon) > 10.0)
  {
    LOG(LINFO, ("Update after Position: ", oldPos, lon, lat));

    m_pQuery->SetPosition(m2::PointD(MercatorBounds::LonToX(lon),
                                     MercatorBounds::LatToY(lat)));

    m_pQuery->SetViewport(MercatorBounds::MetresToXY(lon, lat, 25000));

    RepeatSearch();
  }
}

void Engine::SetPositionSimple(m2::PointD const & pt)
{
  m_pQuery->SetPosition(pt);
}

void Engine::EnablePositionTrack(bool enable)
{
  m_trackEnable = enable;

  if (m_trackEnable)
  {
    LOG(LINFO, ("Enable tracking"));
  }
  else
  {
    LOG(LINFO, ("Disable tracking"));

    m_pQuery->SetPosition(m_savedViewport.Center());
    m_pQuery->SetViewport(m_savedViewport);

    RepeatSearch();
  }
}

void Engine::SetPreferredLanguage(string const & lang)
{
  m_pQuery->SetPreferredLanguage(lang);
}

void Engine::Search(string const & queryText, SearchCallbackT const & f)
{
  LOG(LDEBUG, (queryText));

  m_callback = f;
  m_queryText = queryText;

  Results res;
  m_pQuery->Search(queryText, res);

  f(res);
}

void Engine::RepeatSearch()
{
  if (!m_queryText.empty() && !m_callback.empty())
    Search(m_queryText, m_callback);
}

string Engine::GetCountryFile(m2::PointD const & pt) const
{
  return m_pData->m_infoGetter.GetRegionFile(pt);
}

}  // namespace search
