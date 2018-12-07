#import "MWMPlacePageData.h"
#import "AppInfo.h"
#import "LocaleTranslator.h"
#import "MWMBannerHelpers.h"
#import "MWMBookmarksManager.h"
#import "MWMNetworkPolicy.h"
#import "MWMUGCViewModel.h"
#import "SwiftBridge.h"

#include "Framework.h"

#include "local_ads/event.hpp"

#include "map/bookmark_helpers.hpp"

#include "platform/preferred_languages.hpp"

#include "partners_api/booking_api.hpp"
#include "partners_api/booking_block_params.hpp"

#include "3party/opening_hours/opening_hours.hpp"

#include <string>
#include <utility>

using namespace place_page;

namespace
{
NSString * const kUserDefaultsLatLonAsDMSKey = @"UserDefaultsLatLonAsDMS";
}  // namespace

@interface MWMPlacePageData ()

@property(copy, nonatomic) NSString * cachedMinPrice;
@property(nonatomic) id<MWMBanner> nativeAd;
@property(copy, nonatomic) NSArray<MWMGalleryItemModel *> * photos;
@property(copy, nonatomic) NSArray<MWMViatorItemModel *> * viatorItems;
@property(nonatomic) NSNumberFormatter * currencyFormatter;
@property(nonatomic, readwrite) MWMUGCViewModel * ugc;
@property(nonatomic) NSInteger bookingDiscount;
@property(nonatomic) BOOL isSmartDeal;

@end

@implementation MWMPlacePageData
{
  Info m_info;

  std::vector<Sections> m_sections;
  std::vector<PreviewRows> m_previewRows;
  std::vector<SpecialProject> m_specialProjectRows;
  std::vector<MetainfoRows> m_metainfoRows;
  std::vector<AdRows> m_adRows;
  std::vector<ButtonsRows> m_buttonsRows;
  std::vector<HotelPhotosRow> m_hotelPhotosRows;
  std::vector<HotelDescriptionRow> m_hotelDescriptionRows;
  std::vector<HotelFacilitiesRow> m_hotelFacilitiesRows;
  std::vector<HotelReviewsRow> m_hotelReviewsRows;

  booking::HotelInfo m_hotelInfo;
}

- (instancetype)initWithPlacePageInfo:(Info const &)info
{
  self = [super init];
  if (self)
    m_info = info;

  return self;
}

- (place_page::Info const &)getRawData { return m_info; }

#pragma mark - Filling sections

- (void)fillSections
{
  m_sections.clear();
  m_previewRows.clear();
  m_metainfoRows.clear();
  m_adRows.clear();
  m_buttonsRows.clear();
  m_hotelPhotosRows.clear();
  m_hotelDescriptionRows.clear();
  m_hotelReviewsRows.clear();
  m_hotelFacilitiesRows.clear();

  m_sections.push_back(Sections::Preview);
  [self fillPreviewSection];

  if ([[self placeDescription] length] && ![[self bookmarkDescription] length])
    m_sections.push_back(Sections::Description);
  
  // It's bookmark.
  if (m_info.IsBookmark())
    m_sections.push_back(Sections::Bookmark);

  // There is always at least coordinate meta field.
  m_sections.push_back(Sections::Metainfo);
  [self fillMetaInfoSection];

  auto const & taxiProviders = [self taxiProviders];
  if (!taxiProviders.empty())
  {
    m_sections.push_back(Sections::Ad);
    m_adRows.push_back(AdRows::Taxi);

    NSString * provider = nil;
    switch (taxiProviders.front())
    {
    case taxi::Provider::Uber: provider = kStatUber; break;
    case taxi::Provider::Yandex: provider = kStatYandex; break;
    case taxi::Provider::Maxim: provider = kStatMaxim; break;
    case taxi::Provider::Rutaxi: provider = kStatRutaxi; break;
    case taxi::Provider::Count: LOG(LERROR, ("Incorrect taxi provider")); break;
    }
    [Statistics logEvent:kStatPlacepageTaxiShow withParameters:@{ @"provider" : provider }];
  }

  if (m_info.ShouldShowAddPlace() || m_info.ShouldShowEditPlace() ||
      m_info.ShouldShowAddBusiness() || m_info.IsSponsored())
  {
    m_sections.push_back(Sections::Buttons);
    [self fillButtonsSection];
  }

  if (m_info.ShouldShowUGC())
    [self addUGCSections];
}

- (void)addUGCSections
{
  NSAssert(m_info.ShouldShowUGC(), @"");

  __weak auto wself = self;
  GetFramework().GetUGC(
      m_info.GetID(), [wself](ugc::UGC const & ugc, ugc::UGCUpdate const & update) {
        __strong auto self = wself;
        self.ugc = [[MWMUGCViewModel alloc] initWithUGC:ugc update:update];

        {
          auto & previewRows = self->m_previewRows;
          auto it = find(previewRows.begin(), previewRows.end(), PreviewRows::Address);
          if (it == previewRows.end())
            it = find(previewRows.begin(), previewRows.end(), PreviewRows::Space);

          previewRows.insert(it, PreviewRows::Review);
          self.refreshPreviewCallback();
        }

        auto & sections = self->m_sections;
        auto const begin = sections.begin();
        auto const end = sections.end();

        auto it = find(begin, end, Sections::Buttons);
        NSUInteger const position = std::distance(begin, it);
        NSUInteger length = 0;

        if (!self.ugc.isUGCEmpty)
        {
          length = 1;
          if (!self.ugc.isAggregatedRatingEmpty)
          {
            it = sections.insert(it, Sections::UGCRating) + 1;
            length++;
          }
          if (self.ugc.isUGCUpdateEmpty)
          {
            it = sections.insert(it, Sections::UGCAddReview) + 1;
            length++;
          }

          it = sections.insert(it, Sections::UGCReviews) + 1;
        }
        else if (self.ugc.isUGCUpdateEmpty)
        {
          it = sections.insert(it, Sections::UGCAddReview);
          length = 1;
        }
        else
        {
          it = sections.insert(it, Sections::UGCReviews);
          length = 1;
        }

        self.sectionsAreReadyCallback({position, length}, self, YES /* It's a section */);
      });
}

- (void)requestBookingData
{
  network_policy::CallPartnersApi([self](auto const & canUseNetwork) {
    auto const api = GetFramework().GetBookingApi(canUseNetwork);
    if (!api)
      return;

    std::string const currency = self.currencyFormatter.currencyCode.UTF8String;

    auto const func = [self, currency](std::string const & hotelId,
                                       booking::Blocks const & blocks) {
      if (currency != blocks.m_currency)
        return;

      NSNumberFormatter * decimalFormatter = [[NSNumberFormatter alloc] init];
      decimalFormatter.numberStyle = NSNumberFormatterDecimalStyle;

      auto const price = blocks.m_totalMinPrice == booking::BlockInfo::kIncorrectPrice
                              ? ""
                              : std::to_string(blocks.m_totalMinPrice);

      NSNumber * currencyNumber = [decimalFormatter
                                   numberFromString:[@(price.c_str())
                                                     stringByReplacingOccurrencesOfString:@"."
                                                     withString:decimalFormatter
                                                     .decimalSeparator]];
      NSString * currencyString = [self.currencyFormatter stringFromNumber:currencyNumber];

      self.cachedMinPrice = [NSString stringWithCoreFormat:L(@"place_page_starting_from")
                                                 arguments:@[currencyString]];
      self.bookingDiscount = blocks.m_maxDiscount;
      self.isSmartDeal = blocks.m_hasSmartDeal;
      if (self.bookingDataUpdatedCallback)
        self.bookingDataUpdatedCallback();
    };

    auto params = booking::BlockParams::MakeDefault();
    params.m_hotelId = self.sponsoredId.UTF8String;
    params.m_currency = currency;
    api->GetBlockAvailability(std::move(params), func);
  });
}

- (void)fillPreviewSection
{
  if (self.title.length) m_previewRows.push_back(PreviewRows::Title);
  if (self.externalTitle.length) m_previewRows.push_back(PreviewRows::ExternalTitle);
  if (self.subtitle.length || self.isMyPosition) m_previewRows.push_back(PreviewRows::Subtitle);
  if (self.schedule != OpeningHours::Unknown) m_previewRows.push_back(PreviewRows::Schedule);
  if (self.isBooking)
  {
    m_previewRows.push_back(PreviewRows::Review);
    [self requestBookingData];
  }

  if (self.address.length) m_previewRows.push_back(PreviewRows::Address);
  if (self.hotelType)
  {
    m_previewRows.push_back(PreviewRows::Space);
    m_previewRows.push_back(PreviewRows::SearchSimilar);
  }

  m_previewRows.push_back(PreviewRows::Space);
  NSAssert(!m_previewRows.empty(), @"Preview row's can't be empty!");

  if (network_policy::CanUseNetwork() && m_info.HasBanner() && ![self isViator])
  {
    __weak auto wSelf = self;
    [[MWMBannersCache cache]
        getWithCoreBanners:banner_helpers::MatchPriorityBanners(m_info.GetBanners())
                 cacheOnly:NO
                   loadNew:YES
                completion:^(id<MWMBanner> ad, BOOL isAsync) {
                  __strong auto self = wSelf;
                  if (!self)
                    return;

                  self.nativeAd = ad;
                  self->m_previewRows.push_back(PreviewRows::Banner);
                  if (isAsync)
                    self.bannerIsReadyCallback();
                }];
  }
}

- (void)fillMetaInfoSection
{
  using namespace osm;
  auto const availableProperties = m_info.AvailableProperties();
  // We can't match each metadata property to its UI field and thats why we need to use our own
  // enum.
  for (auto const p : availableProperties)
  {
    switch (p)
    {
    case Props::OpeningHours: m_metainfoRows.push_back(MetainfoRows::OpeningHours); break;
    case Props::Phone: m_metainfoRows.push_back(MetainfoRows::Phone); break;
    case Props::Website:
      if (!self.isBooking)
        m_metainfoRows.push_back(MetainfoRows::Website);
      break;
    case Props::Email: m_metainfoRows.push_back(MetainfoRows::Email); break;
    case Props::Cuisine: m_metainfoRows.push_back(MetainfoRows::Cuisine); break;
    case Props::Operator: m_metainfoRows.push_back(MetainfoRows::Operator); break;
    case Props::Internet: m_metainfoRows.push_back(MetainfoRows::Internet); break;

    case Props::Wikipedia:
    case Props::Elevation:
    case Props::Stars:
    case Props::Flats:
    case Props::BuildingLevels:
    case Props::Level:
    case Props::Fax: break;
    }
  }

  auto const address = m_info.GetAddress();
  if (!address.empty())
    m_metainfoRows.push_back(MetainfoRows::Address);

  m_metainfoRows.push_back(MetainfoRows::Coordinate);

  switch (m_info.GetLocalAdsStatus())
  {
  case place_page::LocalAdsStatus::NotAvailable: break;
  case place_page::LocalAdsStatus::Candidate:
    m_metainfoRows.push_back(MetainfoRows::LocalAdsCandidate);
    break;
  case place_page::LocalAdsStatus::Customer:
    m_metainfoRows.push_back(MetainfoRows::LocalAdsCustomer);
    [self logLocalAdsEvent:local_ads::EventType::OpenInfo];
    break;
  }
}

- (void)fillButtonsSection
{
  // We don't have to show edit, add place or business if it's booking object.
  if (self.isBooking)
  {
    m_buttonsRows.push_back(ButtonsRows::HotelDescription);
    return;
  }

  if (m_info.ShouldShowAddPlace())
    m_buttonsRows.push_back(ButtonsRows::AddPlace);

  if (m_info.ShouldShowEditPlace())
    m_buttonsRows.push_back(ButtonsRows::EditPlace);

  if (m_info.ShouldShowAddBusiness())
    m_buttonsRows.push_back(ButtonsRows::AddBusiness);
}

- (void)insertSpecialProjectsSectionWithProject:(SpecialProject)project
{
  auto const begin = m_sections.begin();
  auto const end = m_sections.end();

  if (std::find(begin, end, Sections::SpecialProjects) != end)
    return;

  m_sections.insert(find(begin, end, Sections::Preview) + 1, Sections::SpecialProjects);
  m_specialProjectRows.emplace_back(project);

  self.sectionsAreReadyCallback({1, 1}, self, YES);
}

- (void)fillOnlineViatorSection
{
  if (!self.isViator)
    return;

  network_policy::CallPartnersApi([self](auto const & canUseNetwork) {
    auto api = GetFramework().GetViatorApi(canUseNetwork);
    if (!api)
      return;

    std::string const currency = self.currencyFormatter.currencyCode.UTF8String;
    std::string const viatorId = [self sponsoredId].UTF8String;

    __weak auto wSelf = self;
    api->GetTop5Products(
        viatorId, currency, [wSelf, viatorId](std::string const & destId,
                                              std::vector<viator::Product> const & products) {
          __strong auto self = wSelf;
          if (!self || viatorId != destId)
            return;
          NSMutableArray<MWMViatorItemModel *> * items = [@[] mutableCopy];
          for (auto const & p : products)
          {
            auto imageURL = [NSURL URLWithString:@(p.m_photoUrl.c_str())];
            auto pageURL = [NSURL URLWithString:@(p.m_pageUrl.c_str())];
            if (!pageURL)
              continue;
            std::string const ratingFormatted = rating::GetRatingFormatted(p.m_rating);
            auto const ratingValue = rating::GetImpress(p.m_rating);
            auto item = [[MWMViatorItemModel alloc]
                initWithImageURL:imageURL
                         pageURL:pageURL
                           title:@(p.m_title.c_str())
                 ratingFormatted:@(ratingFormatted.c_str())
                      ratingType:static_cast<MWMRatingSummaryViewValueType>(ratingValue)
                        duration:@(p.m_duration.c_str())
                           price:@(p.m_priceFormatted.c_str())];
            [items addObject:item];
          }

          dispatch_async(dispatch_get_main_queue(), [items, self] {
            self.viatorItems = items;

            [self insertSpecialProjectsSectionWithProject:SpecialProject::Viator];
          });
        });
  });
}

- (float)ratingRawValue
{
  return m_info.GetRatingRawValue();
}

- (void)fillOnlineBookingSections
{
  if (!self.isBooking)
    return;

  network_policy::CallPartnersApi([self](auto const & canUseNetwork) {
    auto api = GetFramework().GetBookingApi(canUseNetwork);
    if (!api)
      return;

    std::string const hotelId = self.sponsoredId.UTF8String;
    __weak auto wSelf = self;
    api->GetHotelInfo(
        hotelId, [[AppInfo sharedInfo] twoLetterLanguageId].UTF8String,
        [wSelf, hotelId](booking::HotelInfo const & hotelInfo) {
          __strong auto self = wSelf;
          if (!self || hotelId != hotelInfo.m_hotelId)
            return;

          dispatch_async(dispatch_get_main_queue(), [self, hotelInfo] {
            m_hotelInfo = hotelInfo;

            auto & sections = self->m_sections;
            auto const begin = sections.begin();
            auto const end = sections.end();

            NSUInteger const position = find(begin, end, Sections::Bookmark) != end ? 2 : 1;
            NSUInteger length = 0;
            auto it = m_sections.begin() + position;

            if (!hotelInfo.m_photos.empty())
            {
              it = sections.insert(it, Sections::HotelPhotos) + 1;
              m_hotelPhotosRows.emplace_back(HotelPhotosRow::Regular);
              length++;
            }

            if (!hotelInfo.m_description.empty())
            {
              it = sections.insert(it, Sections::HotelDescription) + 1;
              m_hotelDescriptionRows.emplace_back(HotelDescriptionRow::Regular);
              m_hotelDescriptionRows.emplace_back(HotelDescriptionRow::ShowMore);
              length++;
            }

            auto const & facilities = hotelInfo.m_facilities;
            if (!facilities.empty())
            {
              it = sections.insert(it, Sections::HotelFacilities) + 1;
              auto & facilitiesRows = self->m_hotelFacilitiesRows;
              auto const size = facilities.size();
              auto constexpr maxNumberOfHotelCellsInPlacePage = 3UL;

              if (size > maxNumberOfHotelCellsInPlacePage)
              {
                facilitiesRows.insert(facilitiesRows.begin(), maxNumberOfHotelCellsInPlacePage,
                                      HotelFacilitiesRow::Regular);
                facilitiesRows.emplace_back(HotelFacilitiesRow::ShowMore);
              }
              else
              {
                facilitiesRows.insert(facilitiesRows.begin(), size, HotelFacilitiesRow::Regular);
              }

              length++;
            }

            auto const & reviews = hotelInfo.m_reviews;
            if (!reviews.empty())
            {
              sections.insert(it, Sections::HotelReviews);
              auto const size = reviews.size();
              auto & reviewsRows = self->m_hotelReviewsRows;

              reviewsRows.emplace_back(HotelReviewsRow::Header);
              reviewsRows.insert(reviewsRows.end(), size, HotelReviewsRow::Regular);
              reviewsRows.emplace_back(HotelReviewsRow::ShowMore);
              length++;
            }

            self.sectionsAreReadyCallback({position, length}, self, YES /* It's a section */);
          });
        });
  });
}

#pragma mark - Update bookmark status

- (void)updateBookmarkStatus:(BOOL)isBookmark
{
  auto & f = GetFramework();
  auto & bmManager = f.GetBookmarkManager();
  if (isBookmark)
  {
    auto const categoryId = f.LastEditedBMCategory();
    kml::BookmarkData bmData;
    bmData.m_name = m_info.FormatNewBookmarkName();
    bmData.m_color.m_predefinedColor = f.LastEditedBMColor();
    bmData.m_point = self.mercator;
    if (m_info.IsFeature())
      SaveFeatureTypes(m_info.GetTypes(), bmData);
    auto editSession = bmManager.GetEditSession();
    auto const * bookmark = editSession.CreateBookmark(std::move(bmData), categoryId);
    f.FillBookmarkInfo(*bookmark, m_info);
    m_sections.insert(m_sections.begin() + 1, Sections::Bookmark);
  }
  else
  {
    auto const bookmarkId = m_info.GetBookmarkId();
    auto const * bookmark = bmManager.GetBookmark(bookmarkId);
    if (bookmark)
    {
      f.ResetBookmarkInfo(*bookmark, m_info);
      [[MWMBookmarksManager sharedManager] deleteBookmark:bookmarkId];
    }

    m_sections.erase(remove(m_sections.begin(), m_sections.end(), Sections::Bookmark));
  }
}

#pragma mark - Dealloc

- (void)dealloc
{
// TODO: Submit ugc.
//  [self.reviewViewModel submit];
  auto nativeAd = self.nativeAd;
  if (nativeAd)
    [[MWMBannersCache cache] bannerIsOutOfScreenWithCoreBanner:nativeAd];
}

#pragma mark - Getters

- (BOOL)isPopular { return m_info.GetPopularity() > 0; }
- (storage::TCountryId const &)countryId { return m_info.GetCountryId(); }
- (FeatureID const &)featureId { return m_info.GetID(); }
- (NSString *)title { return @(m_info.GetTitle().c_str()); }
- (NSString *)subtitle { return @(m_info.GetSubtitle().c_str()); }
- (NSString *)placeDescription
{
  NSString * descr = @(m_info.GetDescription().c_str());
  if (descr.length > 0)
    descr = [NSString stringWithFormat:@"<html><body>%@</body></html>", descr];

  return descr;
}

- (place_page::OpeningHours)schedule;
{
  using type = place_page::OpeningHours;
  auto const raw = m_info.GetOpeningHours();
  if (raw.empty())
    return type::Unknown;

  auto const t = time(nullptr);
  osmoh::OpeningHours oh(raw);
  if (!oh.IsValid())
    return type::Unknown;
  if (oh.IsTwentyFourHours())
    return type::AllDay;
  if (oh.IsOpen(t))
    return type::Open;
  if (oh.IsClosed(t))
    return type::Closed;

  return type::Unknown;
}

#pragma mark - Sponsored

- (MWMUGCRatingValueType *)bookingRating
{
  if (!self.isBooking)
    return nil;
  auto const ratingRaw = m_info.GetRatingRawValue();
  return [[MWMUGCRatingValueType alloc]
      initWithValue:@(rating::GetRatingFormatted(ratingRaw).c_str())
               type:[MWMPlacePageData ratingValueType:rating::GetImpress(ratingRaw)]];
}

- (NSString *)bookingPricing
{
  ASSERT(self.isBooking, ("Only for booking.com hotels"));
  return self.cachedMinPrice.length ? self.cachedMinPrice : @(m_info.GetApproximatePricing().c_str());
}

- (NSURL *)sponsoredURL
{
  // There are sponsors without URL. For such psrtners we do not show special button.
  if (m_info.IsSponsored() && !m_info.GetSponsoredUrl().empty())
  {
    auto urlString = [@(m_info.GetSponsoredUrl().c_str())
        stringByAddingPercentEncodingWithAllowedCharacters:NSCharacterSet
                                                               .URLQueryAllowedCharacterSet];
    auto url = [NSURL URLWithString:urlString];
    return url;
  }
  return nil;
}

- (NSURL *)deepLink
{
  auto const & link = m_info.GetSponsoredDeepLink();
  if (m_info.IsSponsored() && !link.empty())
    return [NSURL URLWithString:@(link.c_str())];
  
  return nil;
}

- (NSURL *)sponsoredDescriptionURL
{
  return m_info.IsSponsored()
             ? [NSURL URLWithString:@(m_info.GetSponsoredDescriptionUrl().c_str())]
             : nil;
}

- (NSURL *)bookingSearchURL
{
  auto const & url = m_info.GetBookingSearchUrl();
  return url.empty() ? nil : [NSURL URLWithString:@(url.c_str())];
}

- (NSString *)sponsoredId
{
  return m_info.IsSponsored()
             ? @(m_info.GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID).c_str())
             : nil;
}

- (NSNumberFormatter *)currencyFormatter
{
  if (!_currencyFormatter)
  {
    _currencyFormatter = [[NSNumberFormatter alloc] init];
    if (_currencyFormatter.currencyCode.length != 3)
      _currencyFormatter.locale = [[NSLocale alloc] initWithLocaleIdentifier:@"en_US"];

    _currencyFormatter.numberStyle = NSNumberFormatterCurrencyStyle;
    _currencyFormatter.maximumFractionDigits = 0;
  }
  return _currencyFormatter;
}

- (NSString *)hotelDescription { return @(m_hotelInfo.m_description.c_str()); }
- (std::vector<booking::HotelFacility> const &)facilities { return m_hotelInfo.m_facilities; }
- (std::vector<booking::HotelReview> const &)hotelReviews { return m_hotelInfo.m_reviews; }
- (NSUInteger)numberOfHotelReviews { return m_hotelInfo.m_scoreCount; }

- (NSURL *)URLToAllReviews { return [NSURL URLWithString:@(m_info.GetSponsoredReviewUrl().c_str())]; }
- (NSArray<MWMGalleryItemModel *> *)photos
{
  if (_photos)
    return _photos;

  NSMutableArray<MWMGalleryItemModel *> * res = [@[] mutableCopy];
  for (auto const & p : m_hotelInfo.m_photos)
  {
    auto big = [NSURL URLWithString:@(p.m_original.c_str())];
    auto preview = [NSURL URLWithString:@(p.m_small.c_str())];
    if (!big || !preview)
      continue;
    
    auto photo = [[MWMGalleryItemModel alloc] initWithImageURL:big previewURL:preview];
    [res addObject:photo];
  }

  self.photos = res;
  return _photos;
}

- (boost::optional<int>)hotelRawApproximatePricing
{
  return m_info.GetRawApproximatePricing();
}

- (boost::optional<ftypes::IsHotelChecker::Type>)hotelType
{
  return m_info.GetHotelType();
}

#pragma mark - Partners

- (NSString *)partnerName
{
  return self.isPartner ? @(m_info.GetPartnerName().c_str()) : nil;
}

- (int)partnerIndex
{
  return self.isPartner ? m_info.GetPartnerIndex() : -1;
}

#pragma mark - UGC

- (ftraits::UGCRatingCategories)ugcRatingCategories { return m_info.GetRatingCategories(); }

- (void)setUGCUpdateFrom:(MWMUGCReviewModel *)reviewModel
{
  using namespace ugc;
  auto appInfo = AppInfo.sharedInfo;
  auto const locale =
      static_cast<uint8_t>(StringUtf8Multilang::GetLangIndex(appInfo.twoLetterLanguageId.UTF8String));
  std::vector<uint8_t> keyboardLanguages;
  // TODO: Set the list of used keyboard languages (not only the recent one).
  auto lastInputLanguage = appInfo.twoLetterInputLanguage;
  keyboardLanguages.emplace_back(StringUtf8Multilang::GetLangIndex(lastInputLanguage.UTF8String));

  KeyboardText t{reviewModel.text.UTF8String, locale, keyboardLanguages};
  Ratings r;
  for (MWMUGCRatingStars * star in reviewModel.ratings)
    r.emplace_back(star.title.UTF8String, star.value);

  UGCUpdate update{r, t, std::chrono::system_clock::now()};
  auto & f = GetFramework();
  f.GetUGCApi()->SetUGCUpdate(m_info.GetID(), update);
  f.UpdatePlacePageInfoForCurrentSelection();
}

#pragma mark - Bookmark

- (NSString *)externalTitle
{
  return m_info.GetSecondaryTitle().empty() ? nil : @(m_info.GetSecondaryTitle().c_str());
}

- (kml::PredefinedColor)bookmarkColor
{
  return m_info.IsBookmark() ? m_info.GetBookmarkData().m_color.m_predefinedColor : kml::PredefinedColor::None;
}

- (NSString *)bookmarkDescription
{
  return m_info.IsBookmark() ? @(GetPreferredBookmarkStr(m_info.GetBookmarkData().m_description).c_str()) : nil;
}

- (NSString *)bookmarkCategory
{
  return m_info.IsBookmark() ? @(m_info.GetBookmarkCategoryName().c_str()) : nil;
}

- (kml::MarkId)bookmarkId
{
  return m_info.GetBookmarkId();
}

- (kml::MarkGroupId)bookmarkCategoryId
{
  return m_info.GetBookmarkCategoryId();
}

- (BOOL)isBookmarkFromCatalog
{
  return self.isBookmark && [[MWMBookmarksManager sharedManager] isCategoryFromCatalog:self.bookmarkCategoryId];
}

#pragma mark - Local Ads
- (NSString *)localAdsURL { return @(m_info.GetLocalAdsUrl().c_str()); }
- (void)logLocalAdsEvent:(local_ads::EventType)type
{
  if (m_info.GetLocalAdsStatus() != place_page::LocalAdsStatus::Customer)
    return;
  auto const featureID = m_info.GetID();
  auto const & mwmInfo = featureID.m_mwmId.GetInfo();
  if (!mwmInfo)
    return;
  auto & f = GetFramework();
  auto location = [MWMLocationManager lastLocation];
  auto event = local_ads::Event(type, mwmInfo->GetVersion(), mwmInfo->GetCountryName(),
                                featureID.m_index, f.GetDrawScale(),
                                local_ads::Clock::now(), location.coordinate.latitude,
                                location.coordinate.longitude, location.horizontalAccuracy);
  f.GetLocalAdsManager().GetStatistics().RegisterEvent(std::move(event));
}

#pragma mark - Taxi
- (std::vector<taxi::Provider::Type> const &)taxiProviders
{
  return m_info.ReachableByTaxiProviders();
}

#pragma mark - Getters

- (RouteMarkType)routeMarkType { return m_info.GetRouteMarkType(); }
- (size_t)intermediateIndex { return m_info.GetIntermediateIndex(); }
- (NSString *)address { return @(m_info.GetAddress().c_str()); }
- (NSString *)apiURL { return @(m_info.GetApiUrl().c_str()); }
- (std::vector<Sections> const &)sections { return m_sections; }
- (std::vector<PreviewRows> const &)previewRows { return m_previewRows; }
- (std::vector<place_page::SpecialProject> const &)specialProjectRows
{
  return m_specialProjectRows;
}
- (std::vector<MetainfoRows> const &)metainfoRows { return m_metainfoRows; }
- (std::vector<MetainfoRows> &)mutableMetainfoRows { return m_metainfoRows; }
- (std::vector<AdRows> const &)adRows { return m_adRows; }
- (std::vector<ButtonsRows> const &)buttonsRows { return m_buttonsRows; }
- (std::vector<HotelPhotosRow> const &)photosRows { return m_hotelPhotosRows; }
- (std::vector<HotelDescriptionRow> const &)descriptionRows { return m_hotelDescriptionRows; }
- (std::vector<HotelFacilitiesRow> const &)hotelFacilitiesRows { return m_hotelFacilitiesRows; }
- (std::vector<HotelReviewsRow> const &)hotelReviewsRows { return m_hotelReviewsRows; }
- (NSString *)stringForRow:(MetainfoRows)row
{
  switch (row)
  {
  case MetainfoRows::ExtendedOpeningHours: return nil;
  case MetainfoRows::OpeningHours: return @(m_info.GetOpeningHours().c_str());
  case MetainfoRows::Phone: return @(m_info.GetPhone().c_str());
  case MetainfoRows::Address: return @(m_info.GetAddress().c_str());
  case MetainfoRows::Website: return @(m_info.GetWebsite().c_str());
  case MetainfoRows::Email: return @(m_info.GetEmail().c_str());
  case MetainfoRows::Cuisine:
    return @(strings::JoinStrings(m_info.GetLocalizedCuisines(), Info::kSubtitleSeparator).c_str());
  case MetainfoRows::Operator: return @(m_info.GetOperator().c_str());
  case MetainfoRows::Internet: return L(@"WiFi_available");
  case MetainfoRows::LocalAdsCandidate: return L(@"create_campaign_button");
  case MetainfoRows::LocalAdsCustomer: return L(@"view_campaign_button");
  case MetainfoRows::Coordinate:
    return @(m_info
                 .GetFormattedCoordinate(
                     [NSUserDefaults.standardUserDefaults boolForKey:kUserDefaultsLatLonAsDMSKey])
                 .c_str());
  }
}

#pragma mark - Helpers

- (NSString *)phoneNumber { return @(m_info.GetPhone().c_str()); }
- (BOOL)isBookmark { return m_info.IsBookmark(); }
- (BOOL)isApi { return m_info.HasApiUrl(); }
- (BOOL)isBooking { return m_info.GetSponsoredType() == SponsoredType::Booking; }
- (BOOL)isOpentable { return m_info.GetSponsoredType() == SponsoredType::Opentable; }
- (BOOL)isViator { return m_info.GetSponsoredType() == SponsoredType::Viator; }
- (BOOL)isPartner { return m_info.GetSponsoredType() == SponsoredType::Partner; }
- (BOOL)isHolidayObject { return m_info.GetSponsoredType() == SponsoredType::Holiday; }
- (BOOL)isBookingSearch { return !m_info.GetBookingSearchUrl().empty(); }
- (BOOL)isMyPosition { return m_info.IsMyPosition(); }
- (BOOL)isHTMLDescription { return strings::IsHTML(GetPreferredBookmarkStr(m_info.GetBookmarkData().m_description)); }
- (BOOL)isRoutePoint { return m_info.IsRoutePoint(); }
- (BOOL)isPreviewExtended { return m_info.IsPreviewExtended(); }
- (BOOL)isPartnerAppInstalled
{
  // TODO(): Load list of registered schemas from plist.
  return [UIApplication.sharedApplication canOpenURL:self.deepLink];
}

+ (MWMRatingSummaryViewValueType)ratingValueType:(rating::Impress)impress
{
  switch (impress)
  {
  case rating::Impress::None: return MWMRatingSummaryViewValueTypeNoValue;
  case rating::Impress::Horrible: return MWMRatingSummaryViewValueTypeHorrible;
  case rating::Impress::Bad: return MWMRatingSummaryViewValueTypeBad;
  case rating::Impress::Normal: return MWMRatingSummaryViewValueTypeNormal;
  case rating::Impress::Good: return MWMRatingSummaryViewValueTypeGood;
  case rating::Impress::Excellent: return MWMRatingSummaryViewValueTypeExcellent;
  }
}
#pragma mark - Coordinates

- (m2::PointD const &)mercator { return m_info.GetMercator(); }
- (ms::LatLon)latLon { return m_info.GetLatLon(); }
+ (void)toggleCoordinateSystem
{
  // TODO: Move changing latlon's mode to the settings.
  NSUserDefaults * ud = NSUserDefaults.standardUserDefaults;
  [ud setBool:![ud boolForKey:kUserDefaultsLatLonAsDMSKey] forKey:kUserDefaultsLatLonAsDMSKey];
  [ud synchronize];
}

#pragma mark - Stats

- (NSString *)statisticsTags
{
  NSMutableArray<NSString *> * result = [@[] mutableCopy];
  for (auto const & s : m_info.GetRawTypes())
    [result addObject:@(s.c_str())];
  return [result componentsJoinedByString:@", "];
}

@end
