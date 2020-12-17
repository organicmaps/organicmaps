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

enum class SearchMarkPoint::SearchMarkType : uint32_t
{
  Default = 0,
  Booking,
  Hotel,
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

  NotFound,  // Service value used in developer tools.
  Count
};

using SearchMarkType = SearchMarkPoint::SearchMarkType;

namespace
{
df::ColorConstant const kPoiVisitedMaskColor = "PoiVisitedMask";

float const kVisitedSymbolOpacity = 0.7f;
float const kOutOfFiltersSymbolOpacity = 0.4f;
float const kOutOfFiltersTextOpacity = 0.54f;

std::string const kColoredmarkSmall = "coloredmark-default-s";

std::string const kSmallChip = "chips-s";
std::string const kSmallSelectedChip = "chips-selected-s";

std::string const kPriceChip = "price-chips";
std::string const kPriceChipSelected = "price-chips-selected";
std::string const kPriceChipDiscount = "price-chips-discount";
std::string const kPriceChipSelectedDiscount = "price-chips-selected-discount";

std::string const kRatedDefaultSearchIcon = "rated-default-search-result";
std::string const kRatedDefaultSearchIconAds = "local_ads_rated-default-search-result";

std::string const kBookingNoRatingSearchIcon = "norating-default-l";
std::string const & kOsmHotelSearchIcon = kBookingNoRatingSearchIcon;

std::string const kSelectionChipSmall = "selection-chips-s";

std::array<std::string, static_cast<size_t>(SearchMarkType::Count)> const kSymbols = {
    "search-result",                        // Default.
    "coloredmark-default-l",                // Booking.
    "coloredmark-default-l",                // Hotel
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

float constexpr kFontSize = 10.0f;

float const kRatingThreshold = 6.0f;

int constexpr kWorldZoomLevel = 1;
int constexpr kUGCBadgeMinZoomLevel = scales::GetUpperCountryScale();
int constexpr kGoodRatingZoomLevel = kWorldZoomLevel;
int constexpr kBadRatingZoomLevel = scales::GetUpperComfortScale();

std::string GetSymbol(SearchMarkType searchMarkType, bool hasLocalAds, bool hasRating)
{
  if (searchMarkType == SearchMarkType::Default && hasRating)
    return hasLocalAds ? kRatedDefaultSearchIconAds : kRatedDefaultSearchIcon;

  if (searchMarkType == SearchMarkType::Booking && !hasRating)
    return kBookingNoRatingSearchIcon;

  if (searchMarkType == SearchMarkType::Hotel && !hasLocalAds)
    return kOsmHotelSearchIcon;

  auto const index = static_cast<size_t>(searchMarkType);
  ASSERT_LESS(index, kSymbols.size(), ());
  if (hasLocalAds)
    return "local_ads_" + kSymbols[index];

  return kSymbols[index];
}

bool HasLocalAdsVariant(SearchMarkType searchMarkType)
{
  return searchMarkType != SearchMarkType::NotFound;
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
    auto const it = std::partition_point(m_searchMarkTypes.cbegin(), m_searchMarkTypes.cend(),
                                         [type](auto && t) { return t.first < type; });
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

    std::sort(m_searchMarkTypes.begin(), m_searchMarkTypes.end());
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
}

m2::PointD SearchMarkPoint::GetPixelOffset() const
{
  if (!IsBookingSpecialMark() && !IsHotel())
    return {0.0, 4.0};
  return {};
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> SearchMarkPoint::GetSymbolNames() const
{
  auto const symbolName = GetSymbolName();
  if (symbolName.empty())
    return nullptr;

  auto symbolZoomInfo = make_unique_dp<SymbolNameZoomInfo>();
  if (IsBookingSpecialMark())
  {
    if (!m_isAvailable)
    {
      symbolZoomInfo->emplace(kWorldZoomLevel, symbolName);
    }
    else if (m_isPreparing)
    {
      symbolZoomInfo->emplace(kWorldZoomLevel, symbolName);
    }
    else
    {
      if (HasGoodRating())
      {
        symbolZoomInfo->emplace(kGoodRatingZoomLevel, symbolName);
      }
      else  // bad or no rating
      {
        symbolZoomInfo->emplace(kWorldZoomLevel, kColoredmarkSmall);
        symbolZoomInfo->emplace(kBadRatingZoomLevel, symbolName);
      }
    }
    return symbolZoomInfo;
  }

  if (IsHotel() && !HasRating())
  {
    symbolZoomInfo->emplace(kWorldZoomLevel, kColoredmarkSmall);
    symbolZoomInfo->emplace(kBadRatingZoomLevel, symbolName);
  }
  else
  {
    symbolZoomInfo->emplace(kWorldZoomLevel, symbolName);
  }
  return symbolZoomInfo;
}

drape_ptr<df::UserPointMark::BageInfo> SearchMarkPoint::GetBadgeInfo() const
{
  auto const badgeName = GetBadgeName();
  if (badgeName.empty())
    return nullptr;

  std::string const badgeMaskColor = m_isVisited ? kPoiVisitedMaskColor : "";

  if (IsBookingSpecialMark())
  {
    if (!m_isAvailable)
    {
      if (HasReason() && m_isSelected)
      {
        auto badgeInfo = make_unique_dp<BageInfo>();
        badgeInfo->m_maskColor = badgeMaskColor;
        badgeInfo->m_badgeTitleIndex = 0;
        badgeInfo->m_zoomInfo.emplace(kWorldZoomLevel, badgeName);
        return badgeInfo;
      }
    }
    else if (m_isPreparing)
    {
      if (m_isSelected)
      {
        auto badgeInfo = make_unique_dp<BageInfo>();
        badgeInfo->m_maskColor = badgeMaskColor;
        badgeInfo->m_zoomInfo.emplace(kWorldZoomLevel, badgeName);
        return badgeInfo;
      }
    }
    else if (HasPrice() || HasPricing() || m_isSelected)
    {
      auto badgeInfo = make_unique_dp<BageInfo>();

      badgeInfo->m_maskColor = badgeMaskColor;

      if (HasPrice() || HasPricing())
        badgeInfo->m_badgeTitleIndex = 1;

      if (HasGoodRating())
      {
        badgeInfo->m_zoomInfo.emplace(kGoodRatingZoomLevel, badgeName);
      }
      else  // bad or no rating
      {
        if (m_isSelected)
          badgeInfo->m_zoomInfo.emplace(kWorldZoomLevel, kSelectionChipSmall);
        badgeInfo->m_zoomInfo.emplace(kBadRatingZoomLevel, badgeName);
      }

      return badgeInfo;
    }
    return nullptr;
  }

  if (HasRating())
  {
    auto badgeInfo = make_unique_dp<BageInfo>();

    badgeInfo->m_maskColor = badgeMaskColor;

    badgeInfo->m_badgeTitleIndex = 0;

    badgeInfo->m_zoomInfo.emplace(kUGCBadgeMinZoomLevel, badgeName);
    return badgeInfo;
  }
  
  return nullptr;
}

drape_ptr<df::UserPointMark::SymbolOffsets> SearchMarkPoint::GetSymbolOffsets() const
{
  m2::PointF offset;
  if (!IsBookingSpecialMark() && !IsHotel())
    offset = m2::PointF{0.0, 1.0};
  return make_unique_dp<SymbolOffsets>(static_cast<size_t>(scales::UPPER_STYLE_SCALE), offset);
}

bool SearchMarkPoint::IsMarkAboveText() const
{
  return !IsBookingSpecialMark();
}

float SearchMarkPoint::GetSymbolOpacity() const
{
  if (!m_isAvailable)
    return kOutOfFiltersSymbolOpacity;
  return m_isVisited ? kVisitedSymbolOpacity : 1.0f;
}

df::ColorConstant SearchMarkPoint::GetColorConstant() const
{
  if (IsBookingSpecialMark())
  {
    if (!m_isAvailable)
      return m_isSelected ? "SearchmarkSelectedNotAvailable" : "SearchmarkNotAvailable";
    if (m_isPreparing)
      return "SearchmarkPreparing";
    if (m_hasLocalAds)
      return "RatingGood";
    if (!HasRating())
      return "RatingNone";
    if (!HasGoodRating())
      return "RatingBad";
    return "RatingGood";
  }

  return "SearchmarkDefault";
}

drape_ptr<df::UserPointMark::TitlesInfo> SearchMarkPoint::GetTitleDecl() const
{
  drape_ptr<TitlesInfo> titles;

  double const fs = df::VisualParams::Instance().GetFontScale();
  float const fontSize = static_cast<float>(kFontSize / fs);
  if (IsBookingSpecialMark())
  {
    if (!m_isAvailable)
    {
      if (HasReason() && m_isSelected)
      {
        titles = make_unique_dp<TitlesInfo>();

        dp::TitleDecl & reasonTitleDecl = titles->emplace_back();
        reasonTitleDecl.m_anchor = dp::Left;
        reasonTitleDecl.m_forceNoWrap = true;
        reasonTitleDecl.m_primaryTextFont.m_color = df::GetColorConstant("HotelPriceText");
        reasonTitleDecl.m_primaryTextFont.m_color.PremultiplyAlpha(kOutOfFiltersTextOpacity);
        reasonTitleDecl.m_primaryTextFont.m_size = fontSize;

        reasonTitleDecl.m_primaryText = m_reason;
      }
    }
    else if (m_isPreparing)
    {
      /* do nothing */;
    }
    else
    {
      titles = make_unique_dp<TitlesInfo>();

      dp::TitleDecl & ratingTitleDecl = titles->emplace_back();
      ratingTitleDecl.m_anchor = dp::Center;
      ratingTitleDecl.m_forceNoWrap = true;
      ratingTitleDecl.m_primaryTextFont.m_color = df::GetColorConstant("RatingText");
      ratingTitleDecl.m_primaryTextFont.m_color.PremultiplyAlpha(GetSymbolOpacity());
      ratingTitleDecl.m_primaryTextFont.m_size = fontSize;

      ratingTitleDecl.m_primaryText = place_page::rating::GetRatingFormatted(m_rating);

      if (HasPrice() || HasPricing())
      {
        dp::TitleDecl & badgeTitleDecl = titles->emplace_back();
        badgeTitleDecl.m_anchor = dp::Left;
        badgeTitleDecl.m_forceNoWrap = true;
        badgeTitleDecl.m_primaryTextFont.m_color = df::GetColorConstant("HotelPriceText");
        badgeTitleDecl.m_primaryTextFont.m_color.PremultiplyAlpha(GetSymbolOpacity());
        badgeTitleDecl.m_primaryTextFont.m_size = fontSize;

        if (HasPrice())
        {
          badgeTitleDecl.m_primaryText = m_price;
        }
        else if (HasPricing())
        {
          badgeTitleDecl.m_primaryText.assign(static_cast<size_t>(m_pricing), '$');
        }
      }
    }
    return titles;
  }

  if (HasRating())
  {
    titles = make_unique_dp<TitlesInfo>();

    dp::TitleDecl & ugcRatingTitleDecl = titles->emplace_back();
    ugcRatingTitleDecl.m_anchor = dp::Left;
    ugcRatingTitleDecl.m_forceNoWrap = true;
    ugcRatingTitleDecl.m_primaryTextFont.m_color = df::GetColorConstant("UGCRatingText");
    ugcRatingTitleDecl.m_primaryTextFont.m_color.PremultiplyAlpha(GetSymbolOpacity());
    ugcRatingTitleDecl.m_primaryTextFont.m_size = fontSize;

    ugcRatingTitleDecl.m_primaryText = place_page::rating::GetRatingFormatted(m_rating);
  }
  return titles;
}

int SearchMarkPoint::GetMinTitleZoom() const
{
  if (IsBookingSpecialMark())
  {
    if (!m_isAvailable)
    {
      if (HasReason() && m_isSelected)
        return kWorldZoomLevel;
    }
    else if (m_isPreparing)
    {
      return kWorldZoomLevel;
    }
    else
    {
      if (HasGoodRating())
        return kGoodRatingZoomLevel;
      else
        return kBadRatingZoomLevel;
    }
    return kWorldZoomLevel;
  }

  return kUGCBadgeMinZoomLevel;
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
  SetAttributeValue(m_type, GetSearchMarkType(type));
}

void SearchMarkPoint::SetBookingType(bool hasLocalAds)
{
  SetAttributeValue(m_hasLocalAds, hasLocalAds);
  SetAttributeValue(m_type, SearchMarkType::Booking);
}

void SearchMarkPoint::SetHotelType(bool hasLocalAds)
{
  SetAttributeValue(m_hasLocalAds, hasLocalAds);
  SetAttributeValue(m_type, SearchMarkType::Hotel);
}

void SearchMarkPoint::SetNotFoundType()
{
  SetAttributeValue(m_hasLocalAds, false);
  SetAttributeValue(m_type, SearchMarkType::NotFound);
}

void SearchMarkPoint::SetPreparing(bool isPreparing)
{
  SetAttributeValue(m_isPreparing, isPreparing);
}

void SearchMarkPoint::SetRating(float rating)
{
  SetAttributeValue(m_rating, rating);
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

void SearchMarkPoint::SetSelected(bool isSelected)
{
  SetAttributeValue(m_isSelected, isSelected);
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

bool SearchMarkPoint::IsSelected() const { return m_isSelected; }

bool SearchMarkPoint::IsAvailable() const { return m_isAvailable; }

std::string const & SearchMarkPoint::GetReason() const { return m_reason; }

bool SearchMarkPoint::IsBookingSpecialMark() const { return m_type == SearchMarkType::Booking; }

bool SearchMarkPoint::IsHotel() const { return m_type == SearchMarkType::Hotel; }

bool SearchMarkPoint::HasRating() const { return m_rating > kInvalidRatingValue; }

bool SearchMarkPoint::HasGoodRating() const { return m_rating >= kRatingThreshold; }

bool SearchMarkPoint::HasPrice() const { return !m_price.empty(); }

bool SearchMarkPoint::HasPricing() const { return m_pricing > 0; }

bool SearchMarkPoint::HasReason() const { return !m_reason.empty(); }

std::string SearchMarkPoint::GetSymbolName() const
{
  std::string symbolName;
  if (!SearchMarks::HaveSizes())
    return symbolName;

  if (m_type >= SearchMarkType::Count)
  {
    ASSERT(false, ("Unknown search mark symbol."));
    symbolName = GetSymbol(SearchMarkType::Default, false /* hasLocalAds */, HasRating());
  }
  else
  {
    if (IsBookingSpecialMark())
    {
      if (!m_isAvailable)
        symbolName = kColoredmarkSmall;
      else if (m_isPreparing)
        symbolName = kColoredmarkSmall;
      else
        symbolName = GetSymbol(m_type, m_hasLocalAds, HasRating());
    }
    else
    {
      symbolName = GetSymbol(m_type, m_hasLocalAds, HasRating());
    }
  }

  if (symbolName.empty() || !SearchMarks::GetSize(symbolName))
    return {};

  return symbolName;
}

std::string SearchMarkPoint::GetBadgeName() const
{
  std::string badgeName;
  if (!SearchMarks::HaveSizes())
    return badgeName;

  if (IsBookingSpecialMark())
  {
    if (!m_isAvailable)
    {
      if (HasReason() && m_isSelected)
        badgeName = kSmallSelectedChip;
    }
    else if (m_isPreparing)
    {
      if (m_isSelected)
        badgeName = kSelectionChipSmall;
    }
    else
    {
      if (m_isSelected)
        badgeName = m_hasSale ? kPriceChipSelectedDiscount : kPriceChipSelected;
      else
        badgeName = m_hasSale ? kPriceChipDiscount : kPriceChip;
    }
  }
  else
  {
    if (HasRating())
      badgeName = kPriceChip;
  }

  if (badgeName.empty() || !SearchMarks::GetSize(badgeName))
    return {};
  
  return badgeName;
}

// static
std::map<std::string, m2::PointF> SearchMarks::m_searchMarkSizes;

SearchMarks::SearchMarks()
  : m_bmManager(nullptr)
{}

void SearchMarks::SetDrapeEngine(ref_ptr<df::DrapeEngine> engine)
{
  m_drapeEngine.Set(engine);
  if (engine == nullptr)
    return;

  std::vector<std::string> symbols;
  auto const searchMarkTypesCount = static_cast<uint32_t>(SearchMarkType::Count);
  for (uint32_t t = 0; t < searchMarkTypesCount; ++t)
  {
    auto const searchMarkType = static_cast<SearchMarkType>(t);
    symbols.push_back(GetSymbol(searchMarkType, false /* hasLocalAds */, false /* isRated */));
    symbols.push_back(GetSymbol(searchMarkType, false /* hasLocalAds */, true /* isRated */));
    if (HasLocalAdsVariant(searchMarkType))
    {
      symbols.push_back(GetSymbol(searchMarkType, true /* hasLocalAds */, false /* isRated */));
      symbols.push_back(GetSymbol(searchMarkType, true /* hasLocalAds */, true /* isRated */));
    }
  }

  symbols.push_back(kColoredmarkSmall);

  symbols.push_back(kSmallChip);
  symbols.push_back(kSmallSelectedChip);

  symbols.push_back(kPriceChip);
  symbols.push_back(kPriceChipSelected);
  symbols.push_back(kPriceChipDiscount);
  symbols.push_back(kPriceChipSelectedDiscount);

  symbols.push_back(kRatedDefaultSearchIcon);
  symbols.push_back(kRatedDefaultSearchIconAds);

  symbols.push_back(kBookingNoRatingSearchIcon);

  symbols.push_back(kSelectionChipSmall);

  base::SortUnique(symbols);

  m_drapeEngine.SafeCall(&df::DrapeEngine::RequestSymbolsSize, symbols,
                         [this](std::map<std::string, m2::PointF> && sizes)
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [this, sizes = std::move(sizes)]() mutable
    {
      m_searchMarkSizes = std::move(sizes);
      UpdateMaxDimension();
    });
  });
}

void SearchMarks::SetBookmarkManager(BookmarkManager * bmManager)
{
  m_bmManager = bmManager;
}

m2::PointD SearchMarks::GetMaxDimension(ScreenBase const & modelView) const
{
  double const pixelToMercator = modelView.GetScale();
  CHECK_GREATER_OR_EQUAL(pixelToMercator, 0.0, ());
  CHECK_GREATER_OR_EQUAL(m_maxDimension.x, 0.0, ());
  CHECK_GREATER_OR_EQUAL(m_maxDimension.y, 0.0, ());
  return m_maxDimension * pixelToMercator;
}

// static
std::optional<m2::PointD> SearchMarks::GetSize(std::string const & symbolName)
{
  auto const it = m_searchMarkSizes.find(symbolName);
  if (it == m_searchMarkSizes.end())
    return {};
  return m2::PointD(it->second);
}

void SearchMarks::SetPreparingState(std::vector<FeatureID> const & features, bool isPreparing)
{
  if (features.empty())
    return;

  ProcessMarks([&features, isPreparing](SearchMarkPoint * mark) -> base::ControlFlow
  {
    ASSERT(std::is_sorted(features.cbegin(), features.cend()), ());
    if (std::binary_search(features.cbegin(), features.cend(), mark->GetFeatureID()))
      mark->SetPreparing(isPreparing);
    return base::ControlFlow::Continue;
  });
}

void SearchMarks::SetSales(std::vector<FeatureID> const & features, bool hasSale)
{
  if (features.empty())
    return;

  ProcessMarks([&features, hasSale](SearchMarkPoint * mark) -> base::ControlFlow
  {
    ASSERT(std::is_sorted(features.cbegin(), features.cend()), ());
    if (std::binary_search(features.cbegin(), features.cend(), mark->GetFeatureID()))
      mark->SetSale(hasSale);
    return base::ControlFlow::Continue;
  });
}

void SearchMarks::SetPrices(std::vector<FeatureID> const & features, std::vector<std::string> && prices)
{
  if (features.empty() || prices.empty())
    return;

  ProcessMarks([&features, &prices](SearchMarkPoint * mark) -> base::ControlFlow
  {
    ASSERT(std::is_sorted(features.cbegin(), features.cend()), ());
    auto const it = std::lower_bound(features.cbegin(), features.cend(), mark->GetFeatureID());
    if (it != features.cend() && *it == mark->GetFeatureID())
    {
      auto const index = std::distance(features.cbegin(), it);
      ASSERT_LESS(static_cast<size_t>(index), prices.size(), ());
      mark->SetPrice(std::move(prices[index]));
    }
    return base::ControlFlow::Continue;
  });
}

bool SearchMarks::IsThereSearchMarkForFeature(FeatureID const & featureId) const
{
  for (auto const markId : m_bmManager->GetUserMarkIds(UserMark::Type::SEARCH))
  {
    if (m_bmManager->GetUserMark(markId)->GetFeatureID() == featureId)
      return true;
  }
  return false;
}

void SearchMarks::OnActivate(FeatureID const & featureId)
{
  m_selectedFeature = featureId;
  m_visitedSearchMarks.erase(featureId);
  ProcessMarks([&featureId](SearchMarkPoint * mark) -> base::ControlFlow
  {
    if (featureId != mark->GetFeatureID())
      return base::ControlFlow::Continue;
    mark->SetVisited(false);
    mark->SetSelected(true);
    if (!mark->IsAvailable())
    {
      if (auto const & reasonKey = mark->GetReason(); !reasonKey.empty())
      {
        alohalytics::Stats::Instance().LogEvent("Search_Map_Notification",
                                                {{"message", reasonKey}});
      }
    }
    return base::ControlFlow::Break;
  });
}

void SearchMarks::OnDeactivate(FeatureID const & featureId)
{
  m_selectedFeature = {};
  m_visitedSearchMarks.insert(featureId);
  ProcessMarks([&featureId](SearchMarkPoint * mark) -> base::ControlFlow
  {
    if (featureId != mark->GetFeatureID())
      return base::ControlFlow::Continue;
    mark->SetVisited(true);
    mark->SetSelected(false);
    return base::ControlFlow::Break;
  });
}

void SearchMarks::SetUnavailable(SearchMarkPoint & mark, std::string const & reasonKey)
{
  {
    std::scoped_lock<std::mutex> lock(m_lock);
    m_unavailable.insert_or_assign(mark.GetFeatureID(), reasonKey);
  }
  mark.SetAvailable(false);
  mark.SetReason(platform::GetLocalizedString(reasonKey));
}

void SearchMarks::SetUnavailable(std::vector<FeatureID> const & features,
                                 std::string const & reasonKey)
{
  if (features.empty())
    return;

  ProcessMarks([this, &features, &reasonKey](SearchMarkPoint * mark) -> base::ControlFlow
  {
    ASSERT(std::is_sorted(features.cbegin(), features.cend()), ());
    if (std::binary_search(features.cbegin(), features.cend(), mark->GetFeatureID()))
      SetUnavailable(*mark, reasonKey);
    return base::ControlFlow::Continue;
  });
}

bool SearchMarks::IsUnavailable(FeatureID const & id) const
{
  std::scoped_lock<std::mutex> lock(m_lock);
  return m_unavailable.find(id) != m_unavailable.cend();
}

void SearchMarks::SetVisited(FeatureID const & id)
{
  m_visitedSearchMarks.insert(id);
}

bool SearchMarks::IsVisited(FeatureID const & id) const
{
  return m_visitedSearchMarks.find(id) != m_visitedSearchMarks.cend();
}

void SearchMarks::SetSelected(FeatureID const & id)
{
  m_selectedFeature = id;
}

bool SearchMarks::IsSelected(FeatureID const & id) const
{
  return id == m_selectedFeature;
}

void SearchMarks::ClearTrackedProperties()
{
  {
    std::scoped_lock<std::mutex> lock(m_lock);
    m_unavailable.clear();
  }
  m_selectedFeature = {};
}

void SearchMarks::ProcessMarks(
    std::function<base::ControlFlow(SearchMarkPoint *)> && processor) const
{
  if (m_bmManager == nullptr || processor == nullptr)
    return;

  auto editSession = m_bmManager->GetEditSession();
  for (auto markId : m_bmManager->GetUserMarkIds(UserMark::Type::SEARCH))
  {
    auto * mark = editSession.GetMarkForEdit<SearchMarkPoint>(markId);
    if (processor(mark) == base::ControlFlow::Break)
      break;
  }
}

void SearchMarks::UpdateMaxDimension()
{
  for (auto const & [symbolName, symbolSize] : m_searchMarkSizes)
  {
    UNUSED_VALUE(symbolName);
    if (m_maxDimension.x < symbolSize.x)
      m_maxDimension.x = symbolSize.x;
    if (m_maxDimension.y < symbolSize.y)
      m_maxDimension.y = symbolSize.y;
  }

  // factor to roughly account for the width addition of price/pricing text
  double constexpr kBadgeTextFactor = 2.5;
  m_maxDimension.x *= kBadgeTextFactor;
}
