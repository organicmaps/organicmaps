#include "map/search_mark.hpp"

#include "map/bookmark_manager.hpp"
#include "map/place_page_info.hpp"

#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/scales.hpp"

#include "platform/localization.hpp"
#include "platform/platform.hpp"

#include "base/stl_helpers.hpp"

#include "defines.hpp"

#include <algorithm>
#include <array>
#include <limits>

#include "3party/Alohalytics/src/alohalytics.h"

namespace
{
enum class SearchMarkType
{
  Default = 0,
  Booking,
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

df::ColorConstant const kPoiVisitedMaskColor = "PoiVisitedMask";

float const kVisitedSymbolOpacity = 0.7f;

SearchMarkType SMT(uint8_t type)
{
  return static_cast<SearchMarkType>(type);
}

std::array<std::string, static_cast<size_t>(SearchMarkType::Count)> const kSymbols = {
    "search-result",                        // Default.
    "coloredmark-default-l",                // Booking.
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

std::string const kPriceChips = "price-chips";
std::string const kPriceChipsSelected = "price-chips-selected";
std::string const kPriceChipsDiscount = "price-chips-discount";
std::string const kPriceChipsSelectedDiscount = "price-chips-selected-discount";

std::string const kUGCRatingBadgeName = "search-badge-rating";
std::string const kRatedDefaultSearchIcon = "rated-default-search-result";
std::string const kLocalAdsRatedDefaultSearchIcon = "local_ads-rated-default-search-result";

float const kRatingThreshold = 6.0f;

int constexpr kUGCBadgeMinZoomLevel = 10;

// Offset for price text relative to symbol and adjustment of price chip size
// for better margins of price text into the chip
float constexpr kPriceSpecialOffset = -4.0f;

std::string GetBookingSmallIcon(bool hasLocalAds)
{
  return hasLocalAds ? "local_ads-coloredmark-default-s" : "coloredmark-default-s";
}

std::string GetSymbol(SearchMarkType searchMarkType, bool hasLocalAds, bool isRated)
{
  if (searchMarkType == SearchMarkType::Default && isRated)
    return hasLocalAds ? kLocalAdsRatedDefaultSearchIcon : kRatedDefaultSearchIcon;

  auto const index = static_cast<size_t>(searchMarkType);
  ASSERT_LESS(index, kSymbols.size(), ());
  if (!hasLocalAds)
    return kSymbols[index];

  return "local_ads-" + kSymbols[index];
}

constexpr int GetGoodRatingZoomLevel() { return 1; }
constexpr int GetBadRatingZoomLevel() { return scales::GetUpperScale(); }

bool HasLocalAdsVariant(SearchMarkType searchMarkType)
{
  if (searchMarkType == SearchMarkType::NotFound)
    return false;
  return true;
}

std::string GetPreparingSymbol(SearchMarkType searchMarkType, bool hasLocalAds, bool isRated)
{
  if (!hasLocalAds && searchMarkType == SearchMarkType::Booking)
    return "coloredmark-inactive";
  return GetSymbol(searchMarkType, hasLocalAds, isRated);
}

m2::PointD GetSize(SearchMarkType searchMarkType, bool hasLocalAds, bool isRated,
                   ScreenBase const & modelView)
{
  if (!SearchMarks::HaveSizes())
    return {};

  auto const pixelSize = SearchMarks::GetSize(GetSymbol(searchMarkType, hasLocalAds, isRated))
                             .value_or(m2::PointD::Zero());
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
  double const fs = df::VisualParams::Instance().GetFontScale();

  m_titleDecl.m_anchor = dp::Center;
  m_titleDecl.m_primaryTextFont.m_color = df::GetColorConstant("RatingText");
  m_titleDecl.m_primaryTextFont.m_size = static_cast<float>(10.0 / fs);

  m_ugcTitleDecl.m_anchor = dp::LeftTop;
  m_ugcTitleDecl.m_primaryTextFont.m_color = df::GetColorConstant("UGCRatingText");
  m_ugcTitleDecl.m_primaryTextFont.m_size = static_cast<float>(10.0 / fs);

  m_badgeTitleDecl.m_anchor = dp::Left;
  m_badgeTitleDecl.m_primaryTextFont.m_color = df::GetColorConstant("HotelPriceText");
  m_badgeTitleDecl.m_primaryTextFont.m_size = static_cast<float>(10.0 / fs);
  m_badgeTitleDecl.m_primaryOffset.x = kPriceSpecialOffset;
}

std::string SearchMarkPoint::GetSymbolName() const
{
  std::string name;
  if (m_type >= static_cast<uint8_t>(SearchMarkType::Count))
  {
    ASSERT(false, ("Unknown search mark symbol."));
    name = GetSymbol(SearchMarkType::Default, false /* hasLocalAds */, HasRating());
  }
  else if (m_isPreparing)
  {
    name = GetPreparingSymbol(SMT(m_type), m_hasLocalAds, HasRating());
  }
  else
  {
    name = GetSymbol(SMT(m_type), m_hasLocalAds, HasRating());
  }
  return name;
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> SearchMarkPoint::GetSymbolNames() const
{
  auto const name = GetSymbolName();
  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  if (IsBookingSpecialMark())
  {
    if (HasGoodRating())
    {
      symbol->emplace(GetGoodRatingZoomLevel(), name);
    }
    else
    {
      symbol->emplace(GetGoodRatingZoomLevel(), GetBookingSmallIcon(m_hasLocalAds));
      symbol->emplace(GetBadRatingZoomLevel(), name);
    }
  }
  else
  {
    symbol->emplace(1 /* zoomLevel */, name);
  }
  return symbol;
}

drape_ptr<df::UserPointMark::BageInfo> SearchMarkPoint::GetBadgeInfo() const
{
  if (!HasRating())
    return nullptr;

  auto const badgeName = GetBadgeName();
  if (badgeName.empty())
    return nullptr;

  if (IsBookingSpecialMark() && (HasPrice() || HasPricing()))
  {
    auto badgeInfo = make_unique_dp<BageInfo>();
    if (m_isVisited)
      badgeInfo->m_maskColor = kPoiVisitedMaskColor;
    badgeInfo->m_badgeTitleIndex = 1;
    if (HasGoodRating())
      badgeInfo->m_zoomInfo.emplace(GetGoodRatingZoomLevel(), badgeName);
    else
      badgeInfo->m_zoomInfo.emplace(GetBadRatingZoomLevel(), badgeName);
    return badgeInfo;
  }
  
  if (IsUGCMark())
  {
    auto badgeInfo = make_unique_dp<BageInfo>();
    badgeInfo->m_zoomInfo.emplace(kUGCBadgeMinZoomLevel, badgeName);
    return badgeInfo;
  }
  
  return nullptr;
}

drape_ptr<df::UserPointMark::SymbolOffsets> SearchMarkPoint::GetSymbolOffsets() const
{
  auto const badgeName = GetBadgeName();
  if (badgeName.empty())
    return nullptr;

  auto const name = GetSymbolName();
  auto const iconSz = SearchMarks::GetSize(name).value_or(m2::PointD::Zero());

  float horizontalOffset = 0.0f;
  if (SMT(m_type) != SearchMarkType::Booking && m_hasLocalAds)
  {
    float constexpr kLocalAdsSymbolOffset = 0.4f;
    horizontalOffset = kLocalAdsSymbolOffset;
  }
  SymbolOffsets offsets(scales::UPPER_STYLE_SCALE);
  for (size_t i = 0; i < offsets.size(); i++)
  {
    auto const badgeSz = SearchMarks::GetSize(badgeName).value_or(m2::PointD::Zero());
    offsets[i] = {(0.5f + horizontalOffset) * static_cast<float>(badgeSz.x - iconSz.x), 0.0};
  }

  return make_unique_dp<SymbolOffsets>(offsets);
}

bool SearchMarkPoint::IsMarkAboveText() const
{
  return !IsBookingSpecialMark();
}

float SearchMarkPoint::GetSymbolOpacity() const
{
  return m_isVisited ? kVisitedSymbolOpacity : 1.0f;
}

df::ColorConstant SearchMarkPoint::GetColorConstant() const
{
  if (!IsBookingSpecialMark())
    return {};

  if (SMT(m_type) == SearchMarkType::Booking && m_hasLocalAds)
    return "RatingLocalAds";

  if (!HasRating())
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
  if (!HasRating())
    return nullptr;

  drape_ptr<TitlesInfo> titles;

  if (IsUGCMark())
  {
    titles = make_unique_dp<TitlesInfo>();
    auto titleDecl = m_ugcTitleDecl;
    auto const sz = SearchMarks::GetSize(GetSymbolName());
    if (!sz)
      return nullptr;
    auto constexpr kShadowOffset = 4.0;
    auto const centerOffset = -0.5 * sz->y - kShadowOffset;
    titleDecl.m_primaryOffset.y =
        static_cast<float>(centerOffset / df::VisualParams::Instance().GetVisualScale()) -
        0.5f * titleDecl.m_primaryTextFont.m_size;
    titles->push_back(titleDecl);
  }
  else
  {
    titles = make_unique_dp<TitlesInfo>();
    titles->push_back(m_titleDecl);
    if (IsBookingSpecialMark())
    {
      if (HasPrice())
      {
        dp::TitleDecl & badgeTitleDecl = titles->emplace_back(m_badgeTitleDecl);
        badgeTitleDecl.m_primaryText = m_price;
        badgeTitleDecl.m_primaryTextFont.m_color.PremultiplyAlpha(GetSymbolOpacity());
      }
      else if (HasPricing())
      {
        dp::TitleDecl & badgeTitleDecl = titles->emplace_back(m_badgeTitleDecl);
        badgeTitleDecl.m_primaryText.assign(static_cast<size_t>(m_pricing), '$');
        badgeTitleDecl.m_primaryTextFont.m_color.PremultiplyAlpha(GetSymbolOpacity());
      }
    }
  }

  return titles;
}

int SearchMarkPoint::GetMinTitleZoom() const
{
  if (IsBookingSpecialMark() && (HasPrice() || HasPricing()))
  {
    if (HasGoodRating())
      return GetGoodRatingZoomLevel();
    else
      return GetBadRatingZoomLevel();
  }
  if (IsUGCMark())
    return kUGCBadgeMinZoomLevel;
  return GetGoodRatingZoomLevel();
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
  m_ugcTitleDecl.m_primaryText = m_titleDecl.m_primaryText;
}

void SearchMarkPoint::SetPricing(int pricing)
{
  SetAttributeValue(m_pricing, pricing);
}

void SearchMarkPoint::SetPrice(std::string && price)
{
  SetAttributeValue(m_price, std::move(price));
}

void SearchMarkPoint::SetSale(bool hasSale)
{
  SetAttributeValue(m_hasSale, hasSale);
}

void SearchMarkPoint::SetVisited(bool isVisited)
{
  SetAttributeValue(m_isVisited, isVisited);
}

void SearchMarkPoint::SetAvailable(bool isAvailable)
{
  SetAttributeValue(m_isAvailable, isAvailable);
}

void SearchMarkPoint::SetReason(std::string const & reason)
{
  SetAttributeValue(m_reason, reason);
}

bool SearchMarkPoint::IsBookingSpecialMark() const
{
  return (SMT(m_type) == SearchMarkType::Booking) && !m_isPreparing;
}

bool SearchMarkPoint::HasGoodRating() const { return m_rating >= kRatingThreshold; }

bool SearchMarkPoint::HasPrice() const { return !m_price.empty(); }

bool SearchMarkPoint::HasPricing() const { return m_pricing > 0; }

bool SearchMarkPoint::HasRating() const { return m_rating > kInvalidRatingValue; }

bool SearchMarkPoint::IsUGCMark() const { return SMT(m_type) != SearchMarkType::Booking; }

bool SearchMarkPoint::IsSelected() const
{
  return false;  // TODO(tomilov): add sane condition
}

bool SearchMarkPoint::HasSale() const { return m_hasSale; }

std::string SearchMarkPoint::GetBadgeName() const
{
  if (!HasRating())
    return {};

  std::string badgeName;
  if (IsBookingSpecialMark())
  {
    if (IsSelected())
      badgeName = HasSale() ? kPriceChipsSelectedDiscount : kPriceChipsSelected;
    else
      badgeName = HasSale() ? kPriceChipsDiscount : kPriceChips;
  }
  else if (IsUGCMark())
  {
    badgeName = kUGCRatingBadgeName;
  }

  if (badgeName.empty() || !SearchMarks::GetSize(badgeName))
    return {};
  
  return badgeName;
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
  for (size_t t = 0; t < searchMarkTypesCount; ++t)
  {
    auto const searchMarkType = SMT(t);
    symbols.emplace_back(GetSymbol(searchMarkType, false /* hasLocalAds */, false /* isRated */));
    symbols.emplace_back(GetSymbol(searchMarkType, false /* hasLocalAds */, true /* isRated */));
    if (HasLocalAdsVariant(searchMarkType))
    {
      symbols.emplace_back(GetSymbol(searchMarkType, true /* hasLocalAds */, false /* isRated */));
      symbols.emplace_back(GetSymbol(searchMarkType, true /* hasLocalAds */, true /* isRated */));
    }
  }

  symbols.emplace_back(kPriceChips);
  symbols.emplace_back(kPriceChipsSelected);
  symbols.emplace_back(kPriceChipsDiscount);
  symbols.emplace_back(kPriceChipsSelectedDiscount);

  symbols.emplace_back(kUGCRatingBadgeName);

  base::SortUnique(symbols);

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
  m2::PointD markSize;
  auto measurer = [&dimension, &markSize](SearchMarkType searchMarkType,
                                          bool hasLocalAds,
                                          ScreenBase const & modelView)
  {
    for (size_t j = 0; j < 2; ++j)
    {
      markSize = ::GetSize(searchMarkType, true /* hasLocalAds */,
                           j > 0 /* isRated */, modelView);
      dimension = std::max(dimension, std::max(markSize.x, markSize.y));
    }
  };

  for (size_t i = 0; i < static_cast<size_t>(SearchMarkType::Count); ++i)
  {
    auto const searchMarkType = SMT(i);
    measurer(searchMarkType, false /* hasLocalAds */, modelView);
    if (HasLocalAdsVariant(searchMarkType))
      measurer(searchMarkType, true /* hasLocalAds */, modelView);
  }
  return dimension;
}

// static
std::optional<m2::PointD> SearchMarks::GetSize(std::string const & symbolName)
{
  auto const it = m_searchMarksSizes.find(symbolName);
  if (it == m_searchMarksSizes.end())
    return {};
  return m2::PointD(it->second);
}

void SearchMarks::SetPreparingState(std::vector<FeatureID> const & features, bool isPreparing)
{
  ProcessMarks([&features, isPreparing](SearchMarkPoint * mark)
  {
    ASSERT(std::is_sorted(features.begin(), features.end()), ());
    if (std::binary_search(features.cbegin(), features.cend(), mark->GetFeatureID()))
      mark->SetPreparing(isPreparing);
  });
}

void SearchMarks::SetSales(std::vector<FeatureID> const & features, bool hasSale)
{
  ProcessMarks([&features, hasSale](SearchMarkPoint * mark)
  {
    ASSERT(std::is_sorted(features.begin(), features.end()), ());
    if (std::binary_search(features.cbegin(), features.cend(), mark->GetFeatureID()))
      mark->SetSale(hasSale);
  });
}

void SearchMarks::SetPrices(std::vector<FeatureID> const & features, std::vector<std::string> && prices)
{
  ProcessMarks([&features, &prices](SearchMarkPoint * mark)
  {
    ASSERT(std::is_sorted(features.begin(), features.end()), ());
    auto const it = std::lower_bound(features.cbegin(), features.cend(), mark->GetFeatureID());

    if (it == features.cend() || *it != mark->GetFeatureID())
      return;

    auto const index = std::distance(features.cbegin(), it);

    ASSERT_LESS(index, prices.size(), ());
    mark->SetPrice(std::move(prices[index]));
  });
}

void SearchMarks::OnActivate(FeatureID const & featureId)
{
  ProcessMarks([this, &featureId](SearchMarkPoint * mark)
  {
    if (featureId != mark->GetFeatureID())
      return;

    auto const unavailableIt = m_unavailable.find(featureId);
    if (unavailableIt != m_unavailable.cend())
    {
      mark->SetAvailable(false);

      auto const & reasonKey = unavailableIt->second;

      if (!reasonKey.empty())
      {
        mark->SetReason(platform::GetLocalizedString(reasonKey));
        alohalytics::Stats::Instance().LogEvent("Search_Map_Notification",  {{"message", reasonKey}});
      }
    }
  });
}

void SearchMarks::OnDeactivate(FeatureID const & featureId)
{
  ProcessMarks([this, &featureId](SearchMarkPoint * mark)
  {
    if (featureId != mark->GetFeatureID())
      return;

    mark->SetVisited(true);
    m_visited.insert(featureId);

    mark->SetAvailable(true);
    mark->SetReason({});
  });
}

bool SearchMarks::IsVisited(FeatureID const & id) const
{
  return m_visited.find(id) != m_visited.cend();
}

void SearchMarks::ClearVisited()
{
  m_visited.clear();
}

void SearchMarks::SetUnavailable(FeatureID const & id, std::string const & reasonKey)
{
  m_unavailable.insert_or_assign(id, reasonKey);
}

bool SearchMarks::IsUnavailable(FeatureID const & id) const
{
  return m_unavailable.find(id) != m_unavailable.cend();
}

void SearchMarks::MarkUnavailableIfNeeded(SearchMarkPoint * mark) const
{
  auto const unavailableIt = m_unavailable.find(mark->GetFeatureID());
  if (unavailableIt == m_unavailable.cend())
    return;

  mark->SetAvailable(false);
  mark->SetReason(unavailableIt->second);
}

void SearchMarks::ClearUnavailable()
{
  m_unavailable.clear();
}

void SearchMarks::ProcessMarks(std::function<void(SearchMarkPoint *)> && processor)
{
  if (m_bmManager == nullptr || processor == nullptr)
    return;

  auto editSession = m_bmManager->GetEditSession();
  for (auto markId : m_bmManager->GetUserMarkIds(UserMark::Type::SEARCH))
  {
    auto * mark = editSession.GetMarkForEdit<SearchMarkPoint>(markId);
    processor(mark);
  }
}
