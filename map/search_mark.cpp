#include "map/search_mark.hpp"
#include "map/bookmark_manager.hpp"
#include "map/place_page_info.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "platform/platform.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace
{
enum class SearchMarkType
{
  Default = 0,
  Booking,
  UGC,
  Cafe,
  Bakery,
  Bar,
  Pub,
  Restaurant,
  FastFood,
  Casino,
  Cinema,
  Marketplace,
  Nightclub,
  Playground,
  ShopAlcohol,
  ShopButcher,
  ShopClothes,
  ShopConfectionery,
  ShopConvenience,
  ShopCosmetics,
  ShopDepartmentStore,
  ShopGift,
  ShopGreengrocer,
  ShopJewelry,
  ShopMall,
  ShopSeafood,
  ShopShoes,
  ShopSports,
  ShopSupermarket,
  ShopToys,
  ThemePark,
  WaterPark,
  Zoo,

  NotFound, // Service value used in developer tools.
  Count
};

static_assert(static_cast<uint32_t>(SearchMarkType::Count) <= std::numeric_limits<uint8_t>::max(),
              "Change SearchMarkPoint::m_type type.");

SearchMarkType SMT(uint8_t type)
{
  return static_cast<SearchMarkType>(type);
}

std::array<std::string, static_cast<size_t>(SearchMarkType::Count)> const kSymbols = {
    "search-result",                        // Default.
    "coloredmark-default-l",                // Booking.
    "coloredmark-default-l",                // UGC.
    "search-result-cafe",                   // Cafe.
    "search-result-bakery",                 // Bakery.
    "search-result-bar",                    // Bar.
    "search-result-pub",                    // Pub.
    "search-result-restaurant",             // Restaurant.
    "search-result-fastfood",               // FastFood.
    "search-result-casino",                 // Casino.
    "search-result-cinema",                 // Cinema.
    "search-result-marketplace",            // Marketplace.
    "search-result-nightclub",              // Nightclub.
    "search-result-playground",             // Playground.
    "search-result-shop-alcohol",           // ShopAlcohol.
    "search-result-shop-butcher",           // ShopButcher.
    "search-result-shop-clothes",           // ShopClothes.
    "search-result-shop-confectionery",     // ShopConfectionery.
    "search-result-shop-convenience",       // ShopConvenience.
    "search-result-shop-cosmetics",         // ShopCosmetics.
    "search-result-shop-department_store",  // ShopDepartmentStore.
    "search-result-shop-gift",              // ShopGift.
    "search-result-shop-greengrocer",       // ShopGreengrocer.
    "search-result-shop-jewelry",           // ShopJewelry.
    "search-result-shop-mall",              // ShopMall.
    "search-result-shop-seafood",           // ShopSeafood.
    "search-result-shop-shoes",             // ShopShoes.
    "search-result-shop-sports",            // ShopSports.
    "search-result-shop-supermarket",       // ShopSupermarket.
    "search-result-shop-toys",              // ShopToys.
    "search-result-theme-park",             // ThemePark.
    "search-result-water-park",             // WaterPark.
    "search-result-zoo",                    // Zoo.

    "non-found-search-result",  // NotFound.
};

std::string const kSaleBadgeName = "searchbooking-sale-1";

float const kRatingThreshold = 6.0f;
float const kMetricThreshold = 0.38f;

inline bool HasNoRating(float rating)
{
  return fabs(rating) < 1e-5;
}

float CalculateAggregateMetric(float rating, int pricing)
{
  float const p1 = base::clamp(rating, 0.0f, 10.0f) / 10.0f;
  float const p2 = (3 - base::clamp(pricing, 0, 3)) / 3.0f;
  float const s = p1 + p2;
  if (fabs(s) < 1e-5)
    return 0.0f;
  return 2.0f * p1 * p2 / s;
}

std::string GetBookingBadgeName(int pricing)
{
  if (pricing == 0)
    return {};

  return std::string("searchbooking-ext-") + strings::to_string(pricing);
}

bool NeedShowBookingBadge(float rating, int pricing)
{
  if (pricing == 0)
    return false;
  auto const metric = CalculateAggregateMetric(rating, pricing);
  return metric >= kMetricThreshold;
}

std::string GetBookingSmallIcon(SearchMarkType type, bool hasLocalAds)
{
  if (type != SearchMarkType::Booking)
    return {};

  return hasLocalAds ? "search-adv" : "coloredmark-default-s";
}

std::string GetSymbol(SearchMarkType searchMarkType, bool hasLocalAds)
{
  auto const index = static_cast<size_t>(searchMarkType);
  ASSERT_LESS(index, kSymbols.size(), ());
  if (!hasLocalAds)
    return kSymbols[index];

  if (searchMarkType == SearchMarkType::Booking)
    return "searchbookingadv-default-l";

  return "local_ads-" + kSymbols[index];
}

bool HasLocalAdsVariant(SearchMarkType searchMarkType)
{
  if (searchMarkType == SearchMarkType::UGC || searchMarkType == SearchMarkType::NotFound)
    return false;
  return true;
}

std::string GetPreparingSymbol(SearchMarkType searchMarkType, bool hasLocalAds)
{
  if (!hasLocalAds &&
      (searchMarkType == SearchMarkType::Booking || searchMarkType == SearchMarkType::UGC))
  {
    return "coloredmark-inactive";
  }
  return GetSymbol(searchMarkType, hasLocalAds);
}

m2::PointD GetSize(SearchMarkType searchMarkType, bool hasLocalAds, ScreenBase const & modelView)
{
  if (!SearchMarks::HaveSizes())
    return {};

  auto const pixelSize =
      SearchMarks::GetSize(GetSymbol(searchMarkType, hasLocalAds)).get_value_or({});
  double const pixelToMercator = modelView.GetScale();
  return {pixelToMercator * pixelSize.x, pixelToMercator * pixelSize.y};
}

class SearchMarkTypeChecker
{
public:
  static SearchMarkTypeChecker & Instance()
  {
    static SearchMarkTypeChecker checker;
    return checker;
  }

  SearchMarkType GetSearchMarkType(uint32_t type) const
  {
    auto const it = std::lower_bound(m_searchMarkTypes.cbegin(), m_searchMarkTypes.cend(),
                                     Type(type, SearchMarkType::Count), base::LessBy(&Type::first));
    if (it == m_searchMarkTypes.cend() || it->first != type)
      return SearchMarkType::Default;

    return it->second;
  }

private:
  using Type = std::pair<uint32_t, SearchMarkType>;

  SearchMarkTypeChecker()
  {
    auto const & c = classif();
    std::vector<std::pair<std::vector<std::string>, SearchMarkType>> const table = {
      {{"amenity", "cafe"},          SearchMarkType::Cafe},
      {{"shop", "bakery"},           SearchMarkType::Bakery},
      {{"amenity", "bar"},           SearchMarkType::Bar},
      {{"amenity", "pub"},           SearchMarkType::Pub},
      {{"amenity", "biergarten"},    SearchMarkType::Pub},
      {{"amenity", "restaurant"},    SearchMarkType::Restaurant},
      {{"amenity", "fast_food"},     SearchMarkType::FastFood},
      {{"amenity", "casino"},        SearchMarkType::Casino},
      {{"amenity", "cinema"},        SearchMarkType::Cinema},
      {{"amenity", "marketplace"},   SearchMarkType::Marketplace},
      {{"amenity", "nightclub"},     SearchMarkType::Nightclub},
      {{"leisure", "playground"},    SearchMarkType::Playground},
      {{"shop", "alcohol"},          SearchMarkType::ShopAlcohol},
      {{"shop", "beverages"},        SearchMarkType::ShopAlcohol},
      {{"shop", "wine"},             SearchMarkType::ShopAlcohol},
      {{"shop", "butcher"},          SearchMarkType::ShopButcher},
      {{"shop", "clothes"},          SearchMarkType::ShopClothes},
      {{"shop", "confectionery"},    SearchMarkType::ShopConfectionery},
      {{"shop", "convenience"},      SearchMarkType::ShopConvenience},
      {{"shop", "variety_store"},    SearchMarkType::ShopConvenience},
      {{"shop", "cosmetics"},        SearchMarkType::ShopCosmetics},
      {{"shop", "department_store"}, SearchMarkType::ShopDepartmentStore},
      {{"shop", "gift"},             SearchMarkType::ShopGift},
      {{"shop", "greengrocer"},      SearchMarkType::ShopGreengrocer},
      {{"shop", "jewelry"},          SearchMarkType::ShopJewelry},
      {{"shop", "mall"},             SearchMarkType::ShopMall},
      {{"shop", "seafood"},          SearchMarkType::ShopSeafood},
      {{"shop", "shoes"},            SearchMarkType::ShopShoes},
      {{"shop", "sports"},           SearchMarkType::ShopSports},
      {{"shop", "supermarket"},      SearchMarkType::ShopSupermarket},
      {{"shop", "toys"},             SearchMarkType::ShopToys},
      {{"tourism", "theme_park"},    SearchMarkType::ThemePark},
      {{"leisure", "water_park"},    SearchMarkType::WaterPark},
      {{"tourism", "zoo"},           SearchMarkType::Zoo}
    };

    m_searchMarkTypes.reserve(table.size());
    for (auto const & p : table)
      m_searchMarkTypes.push_back({c.GetTypeByPath(p.first), p.second});

    std::sort(m_searchMarkTypes.begin(), m_searchMarkTypes.end(), base::LessBy(&Type::first));
  }

  std::vector<Type> m_searchMarkTypes;
};

SearchMarkType GetSearchMarkType(uint32_t type)
{
  auto const & checker = SearchMarkTypeChecker::Instance();
  return checker.GetSearchMarkType(type);
}
}  // namespace

SearchMarkPoint::SearchMarkPoint(m2::PointD const & ptOrg)
  : UserMark(ptOrg, UserMark::Type::SEARCH)
{
  m_titleDecl.m_anchor = dp::Center;
  m_titleDecl.m_primaryTextFont.m_color = df::GetColorConstant("RatingText");
  m_titleDecl.m_primaryTextFont.m_size =
      static_cast<float>(12.0 / df::VisualParams::Instance().GetFontScale());
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> SearchMarkPoint::GetSymbolNames() const
{
  std::string name;
  if (m_type >= static_cast<uint8_t>(SearchMarkType::Count))
  {
    ASSERT(false, ("Unknown search mark symbol."));
    name = GetSymbol(SearchMarkType::Default, false /* hasLocalAds */);
  }
  else if (m_isPreparing)
  {
    name = GetPreparingSymbol(SMT(m_type), m_hasLocalAds);
  }
  else
  {
    name = GetSymbol(SMT(m_type), m_hasLocalAds);
  }

  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  if (IsMarkWithRating())
  {
    symbol->insert(std::make_pair(
        1 /* zoomLevel */,
        m_rating < kRatingThreshold ? GetBookingSmallIcon(SMT(m_type), m_hasLocalAds) : name));
    symbol->insert(std::make_pair(17 /* zoomLevel */, name));
  }
  else
  {
    symbol->insert(std::make_pair(1 /* zoomLevel */, name));
  }
  return symbol;
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> SearchMarkPoint::GetBadgeNames() const
{
  if (!IsBookingSpecialMark())
    return nullptr;

  auto const badgeName = m_hasSale ? kSaleBadgeName : GetBookingBadgeName(m_pricing);
  if (badgeName.empty() || !SearchMarks::GetSize(badgeName))
    return nullptr;

  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  if (NeedShowBookingBadge(m_rating, m_pricing) && m_rating >= kRatingThreshold)
    symbol->insert(std::make_pair(10 /* zoomLevel */, badgeName));
  else
    symbol->insert(std::make_pair(17 /* zoomLevel */, badgeName));
  return symbol;
}

drape_ptr<df::UserPointMark::SymbolOffsets> SearchMarkPoint::GetSymbolOffsets() const
{
  if (!IsBookingSpecialMark())
    return nullptr;

  auto const badgeName = m_hasSale ? kSaleBadgeName : GetBookingBadgeName(m_pricing);
  if (badgeName.empty() || !SearchMarks::GetSize(badgeName))
    return nullptr;

  auto const name = GetSymbol(SearchMarkType::Booking, false /* hasLocalAds */);
  auto const iconSz = SearchMarks::GetSize(name).get_value_or({});

  SymbolOffsets offsets(scales::UPPER_STYLE_SCALE);
  for (size_t i = 0; i < offsets.size(); i++)
  {
    auto const badgeSz = SearchMarks::GetSize(badgeName).get_value_or({});
    offsets[i] = {0.5f * static_cast<float>(badgeSz.x - iconSz.x), 0.0};
  }

  return make_unique_dp<SymbolOffsets>(offsets);
}

df::ColorConstant SearchMarkPoint::GetColorConstant() const
{
  if (!IsMarkWithRating())
    return {};

  if (SMT(m_type) == SearchMarkType::Booking && m_hasLocalAds)
    return "RatingLocalAds";

  if (HasNoRating(m_rating))
    return "RatingNone";
  if (m_rating < 2.0f)
    return "RatingHorrible";
  if (m_rating < 4.0f)
    return "RatingBad";
  if (m_rating < 6.0f)
    return "RatingNormal";
  if (m_rating < 8.0f)
    return "RatingGood";
  return "RatingExcellent";
}

drape_ptr<df::UserPointMark::TitlesInfo> SearchMarkPoint::GetTitleDecl() const
{
  if (!IsMarkWithRating() || fabs(m_rating) < 1e-5)
    return nullptr;

  auto titles = make_unique_dp<TitlesInfo>();
  titles->push_back(m_titleDecl);
  return titles;
}

int SearchMarkPoint::GetMinTitleZoom() const
{
  if (IsMarkWithRating() && m_rating < kRatingThreshold)
    return 17;
  return 1;
}

df::DepthLayer SearchMarkPoint::GetDepthLayer() const
{
  return df::DepthLayer::SearchMarkLayer;
}

void SearchMarkPoint::SetFoundFeature(FeatureID const & feature)
{
  SetAttributeValue(m_featureID, feature);
}

void SearchMarkPoint::SetMatchedName(std::string const & name)
{
  SetAttributeValue(m_matchedName, name);
}

void SearchMarkPoint::SetFromType(uint32_t type, bool hasLocalAds)
{
  SetAttributeValue(m_hasLocalAds, hasLocalAds);
  SetAttributeValue(m_type, static_cast<uint8_t>(GetSearchMarkType(type)));
}

void SearchMarkPoint::SetBookingType(bool hasLocalAds)
{
  SetAttributeValue(m_hasLocalAds, hasLocalAds);
  SetAttributeValue(m_type, static_cast<uint8_t>(SearchMarkType::Booking));
}

void SearchMarkPoint::SetUGCType()
{
  SetAttributeValue(m_hasLocalAds, false);
  SetAttributeValue(m_type, static_cast<uint8_t>(SearchMarkType::UGC));
}

void SearchMarkPoint::SetNotFoundType()
{
  SetAttributeValue(m_hasLocalAds, false);
  SetAttributeValue(m_type, static_cast<uint8_t>(SearchMarkType::NotFound));
}

void SearchMarkPoint::SetPreparing(bool isPreparing)
{
  SetAttributeValue(m_isPreparing, isPreparing);
}

void SearchMarkPoint::SetRating(float rating)
{
  SetAttributeValue(m_rating, rating);
  m_titleDecl.m_primaryText = place_page::rating::GetRatingFormatted(rating);
}

void SearchMarkPoint::SetPricing(int pricing)
{
  SetAttributeValue(m_pricing, pricing);
}

void SearchMarkPoint::SetSale(bool hasSale)
{
  SetAttributeValue(m_hasSale, hasSale);
}

bool SearchMarkPoint::IsBookingSpecialMark() const
{
  return (SMT(m_type) == SearchMarkType::Booking) && !m_isPreparing;
}

bool SearchMarkPoint::IsUGCMark() const
{
  return SMT(m_type) == SearchMarkType::UGC;
}

bool SearchMarkPoint::IsMarkWithRating() const
{
  return IsBookingSpecialMark() || IsUGCMark();
}

// static
std::map<std::string, m2::PointF> SearchMarks::m_searchMarksSizes;

SearchMarks::SearchMarks()
  : m_bmManager(nullptr)
{}

void SearchMarks::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
  if (engine == nullptr)
    return;

  std::vector<std::string> symbols;
  auto const searchMarkTypesCount = static_cast<size_t>(SearchMarkType::Count);
  symbols.reserve(searchMarkTypesCount * 2);
  for (size_t t = 0; t < searchMarkTypesCount; ++t)
  {
    auto const searchMarkType = SMT(t);
    symbols.push_back(GetSymbol(searchMarkType, false /* hasLocalAds */));
    if (HasLocalAdsVariant(searchMarkType))
      symbols.push_back(GetSymbol(searchMarkType, true /* hasLocalAds */));
  }

  for (int pricing = 1; pricing <= 3; pricing++)
    symbols.push_back(GetBookingBadgeName(pricing));
  symbols.push_back(kSaleBadgeName);

  m_drapeEngine.SafeCall(&df::DrapeEngine::RequestSymbolsSize, symbols,
                         [](std::map<std::string, m2::PointF> && sizes)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [sizes = std::move(sizes)]() mutable
    {
      m_searchMarksSizes = std::move(sizes);
    });
  });
}

void SearchMarks::SetBookmarkManager(BookmarkManager * bmManager)
{
  m_bmManager = bmManager;
}

double SearchMarks::GetMaxDimension(ScreenBase const & modelView) const
{
  double dimension = 0.0;
  for (size_t i = 0; i < static_cast<size_t>(SearchMarkType::Count); ++i)
  {
    auto const searchMarkType = SMT(i);
    m2::PointD markSize = ::GetSize(searchMarkType, false /* hasLocalAds */, modelView);
    dimension = std::max(dimension, std::max(markSize.x, markSize.y));
    if (HasLocalAdsVariant(searchMarkType))
    {
      markSize = ::GetSize(searchMarkType, true /* hasLocalAds */, modelView);
      dimension = std::max(dimension, std::max(markSize.x, markSize.y));
    }
  }
  return dimension;
}

// static
boost::optional<m2::PointD> SearchMarks::GetSize(std::string const & symbolName)
{
  auto const it = m_searchMarksSizes.find(symbolName);
  if (it == m_searchMarksSizes.end())
    return {};
  return m2::PointD(it->second);
}

void SearchMarks::SetPreparingState(std::vector<FeatureID> const & features, bool isPreparing)
{
  FilterAndProcessMarks(features, [isPreparing](SearchMarkPoint * mark)
  {
    mark->SetPreparing(isPreparing);
  });
}

void SearchMarks::SetSales(std::vector<FeatureID> const & features, bool hasSale)
{
  FilterAndProcessMarks(features, [hasSale](SearchMarkPoint * mark)
  {
    mark->SetSale(hasSale);
  });
}

void SearchMarks::FilterAndProcessMarks(std::vector<FeatureID> const & features,
                                        std::function<void(SearchMarkPoint *)> && processor)
{
  if (m_bmManager == nullptr || processor == nullptr)
    return;

  ASSERT(std::is_sorted(features.begin(), features.end()), ());

  auto editSession = m_bmManager->GetEditSession();
  for (auto markId : m_bmManager->GetUserMarkIds(UserMark::Type::SEARCH))
  {
    auto * mark = editSession.GetMarkForEdit<SearchMarkPoint>(markId);
    if (std::binary_search(features.begin(), features.end(), mark->GetFeatureID()))
      processor(mark);
  }
}
