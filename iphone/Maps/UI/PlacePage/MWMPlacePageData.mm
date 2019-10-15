#import "MWMPlacePageData.h"
#import "MWMDiscoveryCityGalleryObjects.h"
#import "MWMDiscoveryGuideViewModel.h"
#import "MWMBannerHelpers.h"
#import "MWMUGCViewModel.h"

#import <CoreApi/CoreApi.h>

#include "map/utils.hpp"

#include "3party/opening_hours/opening_hours.hpp"

using namespace place_page;

namespace
{
NSString * const kUserDefaultsLatLonAsDMSKey = @"UserDefaultsLatLonAsDMS";
}  // namespace

@interface MWMPlacePageData ()

@property(copy, nonatomic) NSString * cachedMinPrice;
@property(nonatomic) id<MWMBanner> nativeAd;
@property(copy, nonatomic) NSArray<MWMGalleryItemModel *> * photos;
@property(nonatomic) NSNumberFormatter * currencyFormatter;
@property(nonatomic, readwrite) MWMUGCViewModel * ugc;
@property(nonatomic, readwrite) MWMDiscoveryCityGalleryObjects *promoGallery;
@property(nonatomic) NSInteger bookingDiscount;
@property(nonatomic) BOOL isSmartDeal;

@end

@implementation MWMPlacePageData
{
  std::vector<Sections> m_sections;
  std::vector<PreviewRows> m_previewRows;
  std::vector<MetainfoRows> m_metainfoRows;
  std::vector<AdRows> m_adRows;
  std::vector<ButtonsRows> m_buttonsRows;
  std::vector<HotelPhotosRow> m_hotelPhotosRows;
  std::vector<HotelDescriptionRow> m_hotelDescriptionRows;
  std::vector<HotelFacilitiesRow> m_hotelFacilitiesRows;
  std::vector<HotelReviewsRow> m_hotelReviewsRows;
  std::vector<PromoCatalogRow> m_promoCatalogRows;

  booking::HotelInfo m_hotelInfo;
}

- (place_page::Info const &)getRawData { return GetFramework().GetCurrentPlacePageInfo(); }

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
  
  if ([[self placeDescription] length] && ![[self bookmarkDescription] length] && !self.isPromoCatalog) {
    m_sections.push_back(Sections::Description);
  }

  place_page::Info const & info = [self getRawData];
  
  // It's bookmark.
  if (info.IsBookmark())
    m_sections.push_back(Sections::Bookmark);

  // There is always at least coordinate meta field.
  m_sections.push_back(Sections::Metainfo);
  [self fillMetaInfoSection];
  
  if ([self roadType] != RoadWarningMarkType::Count) { return; }

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
    case taxi::Provider::Rutaxi: provider = kStatVezet; break;
    case taxi::Provider::Count: LOG(LERROR, ("Incorrect taxi provider")); break;
    }
    [Statistics logEvent:kStatPlacepageTaxiShow withParameters:@{ @"provider" : provider }];
  }

  if (info.ShouldShowAddPlace() || info.ShouldShowEditPlace() ||
      info.ShouldShowAddBusiness() || info.IsSponsored())
  {
    m_sections.push_back(Sections::Buttons);
    [self fillButtonsSection];
  }

  if (info.ShouldShowUGC())
    [self addUGCSections];
}

- (void)addUGCSections
{
  place_page::Info const & info = [self getRawData];
  NSAssert(info.ShouldShowUGC(), @"");

  __weak auto wself = self;
  GetFramework().GetUGC(
      info.GetID(), [wself](ugc::UGC const & ugc, ugc::UGCUpdate const & update) {
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
  auto const api = GetFramework().GetBookingApi(platform::GetCurrentNetworkPolicy());
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

  place_page::Info const & info = [self getRawData];
  if ([MWMNetworkPolicy sharedPolicy].canUseNetwork && info.HasBanner())
  {
    __weak auto wSelf = self;
    [[MWMBannersCache cache]
        getWithCoreBanners:banner_helpers::MatchPriorityBanners(info.GetBanners())
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
  auto const availableProperties = [self getRawData].AvailableProperties();
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

  place_page::Info const & info = [self getRawData];
  auto const address = info.GetAddress();
  if (!address.empty())
    m_metainfoRows.push_back(MetainfoRows::Address);

  m_metainfoRows.push_back(MetainfoRows::Coordinate);

  switch (info.GetLocalAdsStatus())
  {
  case place_page::LocalAdsStatus::NotAvailable: break;
  case place_page::LocalAdsStatus::Candidate:
    m_metainfoRows.push_back(MetainfoRows::LocalAdsCandidate);
    break;
  case place_page::LocalAdsStatus::Customer:
    m_metainfoRows.push_back(MetainfoRows::LocalAdsCustomer);
    [self logLocalAdsEvent:local_ads::EventType::OpenInfo];
    break;
  case place_page::LocalAdsStatus::Hidden:
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

  place_page::Info const & info = [self getRawData];
  
  if (info.ShouldShowAddPlace())
    m_buttonsRows.push_back(ButtonsRows::AddPlace);

  if (info.ShouldShowEditPlace())
    m_buttonsRows.push_back(ButtonsRows::EditPlace);

  if (info.ShouldShowAddBusiness())
    m_buttonsRows.push_back(ButtonsRows::AddBusiness);
}

- (float)ratingRawValue
{
  place_page::Info const & info = [self getRawData];
  return info.GetRatingRawValue();
}

- (void)fillOnlineBookingSections
{
  if (!self.isBooking)
    return;

  auto api = GetFramework().GetBookingApi(platform::GetCurrentNetworkPolicy());
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
}

- (NSInteger)bookmarkSectionPosition {
  if (self.promoGallery != nil) {
    return 2;
  } else {
    return 1;
  }
}

#pragma mark - Update bookmark status

- (void)updateBookmarkStatus:(BOOL)isBookmark
{
  auto & f = GetFramework();
  auto & bmManager = f.GetBookmarkManager();
  place_page::Info const & info = [self getRawData];
  if (isBookmark)
  {
    auto const categoryId = f.LastEditedBMCategory();
    kml::BookmarkData bmData;
    bmData.m_name = info.FormatNewBookmarkName();
    bmData.m_color.m_predefinedColor = f.LastEditedBMColor();
    bmData.m_point = self.mercator;
    if (info.IsFeature())
      SaveFeatureTypes(info.GetTypes(), bmData);
    auto editSession = bmManager.GetEditSession();
    auto const * bookmark = editSession.CreateBookmark(std::move(bmData), categoryId);

    auto buildInfo = info.GetBuildInfo();
    buildInfo.m_match = place_page::BuildInfo::Match::Everything;
    buildInfo.m_userMarkId = bookmark->GetId();
    f.UpdatePlacePageInfoForCurrentSelection(buildInfo);
    
    m_sections.insert(m_sections.begin() + [self bookmarkSectionPosition], Sections::Bookmark);
  }
  else
  {
    auto const bookmarkId = info.GetBookmarkId();
    [[MWMBookmarksManager sharedManager] deleteBookmark:bookmarkId];

    auto buildInfo = info.GetBuildInfo();
    buildInfo.m_match = place_page::BuildInfo::Match::FeatureOnly;
    buildInfo.m_userMarkId = kml::kInvalidMarkId;
    f.UpdatePlacePageInfoForCurrentSelection(buildInfo);
    
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

- (BOOL)isPopular { return [self getRawData].GetPopularity() > 0; }
- (storage::CountryId const &)countryId { return [self getRawData].GetCountryId(); }
- (FeatureID const &)featureId { return [self getRawData].GetID(); }
- (NSString *)title { return @([self getRawData].GetTitle().c_str()); }
- (NSString *)subtitle { return @([self getRawData].GetSubtitle().c_str()); }
- (NSString *)placeDescription
{
  NSString * descr = @([self getRawData].GetDescription().c_str());
  if (descr.length > 0)
    descr = [NSString stringWithFormat:@"<html><body>%@</body></html>", descr];

  return descr;
}

- (place_page::OpeningHours)schedule;
{
  using type = place_page::OpeningHours;
  auto const raw = [self getRawData].GetOpeningHours();
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
  auto const ratingRaw = [self getRawData].GetRatingRawValue();
  return [[MWMUGCRatingValueType alloc]
      initWithValue:@(rating::GetRatingFormatted(ratingRaw).c_str())
               type:[MWMPlacePageData ratingValueType:rating::GetImpress(ratingRaw)]];
}

- (NSString *)bookingPricing
{
  ASSERT(self.isBooking, ("Only for booking.com hotels"));
  return self.cachedMinPrice.length ? self.cachedMinPrice : @([self getRawData].GetApproximatePricing().c_str());
}

- (NSURL *)sponsoredURL
{
  place_page::Info const & info = [self getRawData];
  // There are sponsors without URL. For such psrtners we do not show special button.
  if (info.IsSponsored() && !info.GetSponsoredUrl().empty())
  {
    auto urlString = [@(info.GetSponsoredUrl().c_str())
        stringByAddingPercentEncodingWithAllowedCharacters:NSCharacterSet
                                                               .URLQueryAllowedCharacterSet];
    auto url = [NSURL URLWithString:urlString];
    return url;
  }
  return nil;
}

- (NSURL *)deepLink
{
  place_page::Info const & info = [self getRawData];
  auto const & link = info.GetSponsoredDeepLink();
  if (info.IsSponsored() && !link.empty())
    return [NSURL URLWithString:@(link.c_str())];
  
  return nil;
}

- (NSURL *)sponsoredDescriptionURL
{
  place_page::Info const & info = [self getRawData];
  return info.IsSponsored()
             ? [NSURL URLWithString:@(info.GetSponsoredDescriptionUrl().c_str())]
             : nil;
}

- (NSURL *)sponsoredMoreURL
{
  place_page::Info const & info = [self getRawData];
  return info.IsSponsored()
             ? [NSURL URLWithString:@(info.GetSponsoredMoreUrl().c_str())]
             : nil;
}

- (NSURL *)sponsoredReviewURL
{
  place_page::Info const & info = [self getRawData];
  return info.IsSponsored()
             ? [NSURL URLWithString:@(info.GetSponsoredReviewUrl().c_str())]
             : nil;
}

- (NSURL *)bookingSearchURL
{
  auto const & url = [self getRawData].GetBookingSearchUrl();
  return url.empty() ? nil : [NSURL URLWithString:@(url.c_str())];
}

- (NSString *)sponsoredId
{
  place_page::Info const & info = [self getRawData];
  return info.IsSponsored()
             ? @(info.GetMetadata().Get(feature::Metadata::FMD_SPONSORED_ID).c_str())
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

- (NSURL *)URLToAllReviews { return [NSURL URLWithString:@([self getRawData].GetSponsoredReviewUrl().c_str())]; }
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
  return [self getRawData].GetRawApproximatePricing();
}

- (boost::optional<ftypes::IsHotelChecker::Type>)hotelType
{
  return [self getRawData].GetHotelType();
}

#pragma mark - Partners

- (NSString *)partnerName
{
  return self.isPartner ? @([self getRawData].GetPartnerName().c_str()) : nil;
}

- (int)partnerIndex
{
  return self.isPartner ? [self getRawData].GetPartnerIndex() : -1;
}

#pragma mark - UGC

- (ftraits::UGCRatingCategories)ugcRatingCategories { return [self getRawData].GetRatingCategories(); }

- (void)setUGCUpdateFrom:(MWMUGCReviewModel *)reviewModel
                language:(NSString *)language
           resultHandler:(void (^)(BOOL))resultHandler {
  using namespace ugc;
  auto appInfo = AppInfo.sharedInfo;
  auto const locale =
      static_cast<uint8_t>(StringUtf8Multilang::GetLangIndex(appInfo.twoLetterLanguageId.UTF8String));
  std::vector<uint8_t> keyboardLanguages;
  // TODO: Set the list of used keyboard languages (not only the recent one).
  auto twoLetterInputLanguage = languages::Normalize(language.UTF8String);
  keyboardLanguages.emplace_back(StringUtf8Multilang::GetLangIndex(twoLetterInputLanguage));

  KeyboardText t{reviewModel.text.UTF8String, locale, keyboardLanguages};
  Ratings r;
  for (MWMUGCRatingStars * star in reviewModel.ratings)
    r.emplace_back(star.title.UTF8String, star.value);

  UGCUpdate update{r, t, std::chrono::system_clock::now()};
  
  place_page::Info const & info = [self getRawData];
  GetFramework().GetUGCApi()->SetUGCUpdate(info.GetID(), update,
  [resultHandler, info](Storage::SettingResult const result)
  {
    if (result != Storage::SettingResult::Success)
    {
      resultHandler(NO);
      return;
    }

    resultHandler(YES);
    GetFramework().UpdatePlacePageInfoForCurrentSelection();
    
    utils::RegisterEyeEventIfPossible(eye::MapObject::Event::Type::UgcSaved,
                                      GetFramework().GetCurrentPosition(), info);
  });
}

#pragma mark - Bookmark

- (NSString *)externalTitle
{
  place_page::Info const & info = [self getRawData];
  return info.GetSecondaryTitle().empty() ? nil : @(info.GetSecondaryTitle().c_str());
}

- (kml::PredefinedColor)bookmarkColor
{
  place_page::Info const & info = [self getRawData];
  return info.IsBookmark() ? info.GetBookmarkData().m_color.m_predefinedColor : kml::PredefinedColor::None;
}

- (NSString *)bookmarkDescription
{
  place_page::Info const & info = [self getRawData];
  return info.IsBookmark() ? @(GetPreferredBookmarkStr(info.GetBookmarkData().m_description).c_str()) : nil;
}

- (NSString *)bookmarkCategory
{
  place_page::Info const & info = [self getRawData];
  return info.IsBookmark() ? @(info.GetBookmarkCategoryName().c_str()) : nil;
}

- (kml::MarkId)bookmarkId
{
  place_page::Info const & info = [self getRawData];
  return info.GetBookmarkId();
}

- (kml::MarkGroupId)bookmarkCategoryId
{
  place_page::Info const & info = [self getRawData];
  return info.GetBookmarkCategoryId();
}

- (BOOL)isBookmarkEditable
{
  return self.isBookmark && [[MWMBookmarksManager sharedManager] isCategoryEditable:self.bookmarkCategoryId];
}

#pragma mark - Local Ads
- (NSString *)localAdsURL { return @([self getRawData].GetLocalAdsUrl().c_str()); }
- (void)logLocalAdsEvent:(local_ads::EventType)type
{
  place_page::Info const & info = [self getRawData];
  auto const status = info.GetLocalAdsStatus();
  if (status != place_page::LocalAdsStatus::Customer && status != place_page::LocalAdsStatus::Hidden)
    return;
  auto const featureID = info.GetID();
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
  return [self getRawData].ReachableByTaxiProviders();
}

#pragma mark - Getters

- (RouteMarkType)routeMarkType { return [self getRawData].GetRouteMarkType(); }
- (size_t)intermediateIndex { return [self getRawData].GetIntermediateIndex(); }
- (NSString *)address { return @([self getRawData].GetAddress().c_str()); }
- (NSString *)apiURL { return @([self getRawData].GetApiUrl().c_str()); }
- (std::vector<Sections> const &)sections { return m_sections; }
- (std::vector<PreviewRows> const &)previewRows { return m_previewRows; }
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
  place_page::Info const & info = [self getRawData];
  switch (row)
  {
  case MetainfoRows::ExtendedOpeningHours: return nil;
  case MetainfoRows::OpeningHours: return @(info.GetOpeningHours().c_str());
  case MetainfoRows::Phone: return @(info.GetPhone().c_str());
  case MetainfoRows::Address: return @(info.GetAddress().c_str());
  case MetainfoRows::Website: return @(info.GetWebsite().c_str());
  case MetainfoRows::Email: return @(info.GetEmail().c_str());
  case MetainfoRows::Cuisine:
    return @(strings::JoinStrings(info.GetLocalizedCuisines(), Info::kSubtitleSeparator).c_str());
  case MetainfoRows::Operator: return @(info.GetOperator().c_str());
  case MetainfoRows::Internet: return L(@"WiFi_available");
  case MetainfoRows::LocalAdsCandidate: return L(@"create_campaign_button");
  case MetainfoRows::LocalAdsCustomer: return L(@"view_campaign_button");
  case MetainfoRows::Coordinate:
    return @(info.GetFormattedCoordinate(
                     [NSUserDefaults.standardUserDefaults boolForKey:kUserDefaultsLatLonAsDMSKey])
                 .c_str());
  }
}
- (std::vector<place_page::PromoCatalogRow> const &)promoCatalogRows { return m_promoCatalogRows; }

#pragma mark - Helpers

- (NSString *)phoneNumber { return @([self getRawData].GetPhone().c_str()); }
- (BOOL)isBookmark { return [self getRawData].IsBookmark(); }
- (BOOL)isApi { return [self getRawData].HasApiUrl(); }
- (BOOL)isBooking { return [self getRawData].GetSponsoredType() == SponsoredType::Booking; }
- (BOOL)isOpentable { return [self getRawData].GetSponsoredType() == SponsoredType::Opentable; }
- (BOOL)isPartner { return [self getRawData].GetSponsoredType() == SponsoredType::Partner; }
- (BOOL)isHolidayObject { return [self getRawData].GetSponsoredType() == SponsoredType::Holiday; }
- (BOOL)isPromoCatalog { return self.isLargeToponim || self.isSightseeing; }
- (BOOL)isLargeToponim { return [self getRawData].GetSponsoredType() == SponsoredType::PromoCatalogCity; }
- (BOOL)isSightseeing { return [self getRawData].GetSponsoredType() == SponsoredType::PromoCatalogSightseeings; }
- (BOOL)isBookingSearch { return ![self getRawData].GetBookingSearchUrl().empty(); }
- (BOOL)isMyPosition { return [self getRawData].IsMyPosition(); }
- (BOOL)isHTMLDescription { return strings::IsHTML(GetPreferredBookmarkStr([self getRawData].GetBookmarkData().m_description)); }
- (BOOL)isRoutePoint { return [self getRawData].IsRoutePoint(); }
- (RoadWarningMarkType)roadType { return [self getRawData].GetRoadType(); }
- (BOOL)isPreviewPlus { return [self getRawData].GetOpeningMode() == place_page::OpeningMode::PreviewPlus; }
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

- (m2::PointD const &)mercator { return [self getRawData].GetMercator(); }
- (ms::LatLon)latLon { return [self getRawData].GetLatLon(); }
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
  for (auto const & s : [self getRawData].GetRawTypes())
    [result addObject:@(s.c_str())];
  return [result componentsJoinedByString:@", "];
}

#pragma mark - Promo Gallery

- (void)fillPromoCatalogSection {
  if (!self.isPromoCatalog || self.isBookmark || [self bookmarkDescription].length > 0) {
    return;
  }
  
  [self reguestPromoCatalog];
}

- (void)reguestPromoCatalog {
  auto const canUseNetwork = platform::GetCurrentNetworkPolicy();
  auto const api = GetFramework().GetPromoApi(canUseNetwork);
  m_promoCatalogRows.clear();
  
  auto const row = canUseNetwork.CanUse() ? PromoCatalogRow::GuidesRequestError : PromoCatalogRow::GuidesNoInternetError;
  if (!api) {
    m_promoCatalogRows.push_back(row);
    m_sections.insert(m_sections.begin() + 1, Sections::Description);
    if (self.refreshPromoCallback) {
      self.refreshPromoCallback([NSIndexSet indexSetWithIndex:1]);
    }
    [Statistics logEvent:kStatPlacepageSponsoredError
          withParameters:@{
                           kStatProvider: kStatMapsmeGuides,
                           kStatPlacement: kStatPlacePage,
                           kStatError: kStatNoInternet
                           }];
  } else {
    __weak __typeof(self) weakSelf = self;
    auto const resultHandler = [weakSelf](promo::CityGallery const & cityGallery) {
      __strong __typeof(self) self = weakSelf;
      if (self == nil) {
        return;
      }

      if (cityGallery.IsEmpty()) {
        if ([[self placeDescription] length] && ![[self bookmarkDescription] length]) {
          m_sections.insert(m_sections.begin() + 1, Sections::Description);
          if (self.refreshPromoCallback) {
            self.refreshPromoCallback([NSIndexSet indexSetWithIndex:1]);
          }
        }
      } else {
        self.promoGallery = [[MWMDiscoveryCityGalleryObjects alloc] initWithGalleryResults:cityGallery];
        m_sections.insert(m_sections.begin() + 1, Sections::PromoCatalog);
        m_promoCatalogRows.push_back(PromoCatalogRow::Guides);
        [Statistics logEvent:kStatPlacepageSponsoredShow
              withParameters:@{
                               kStatProvider: kStatMapsmeGuides,
                               kStatPlacement: self.isLargeToponim ? kStatPlacePageToponims : kStatPlacePageSightSeeing,
                               kStatState: kStatOnline,
                               kStatCount: @(cityGallery.m_items.size())
                               }];
        NSMutableIndexSet *insertedSections = [NSMutableIndexSet indexSetWithIndex:1];
        if (self.promoGallery.count > 1 && [[self placeDescription] length] && ![[self bookmarkDescription] length]) {
          NSInteger infoIndex = self.isBookmark ? 3 : 2;
          m_sections.insert(m_sections.begin() + infoIndex, Sections::Description);
          [insertedSections addIndex:infoIndex];
        }
        if (self.refreshPromoCallback) {
          self.refreshPromoCallback([insertedSections copy]);
        }
      }
    };
    
    auto const errorHandler = [weakSelf]() {
      __strong __typeof(self) self = weakSelf;
      if (self == nil) {
        return;
      }

      if ([[self placeDescription] length] && ![[self bookmarkDescription] length]) {
        m_sections.insert(m_sections.begin() + 1, Sections::Description);
        if (self.refreshPromoCallback) {
          self.refreshPromoCallback([NSIndexSet indexSetWithIndex:1]);
        }
      }
    };

    auto appInfo = AppInfo.sharedInfo;
    auto locale = appInfo.twoLetterLanguageId.UTF8String;
    place_page::Info const & info = [self getRawData];
    if (info.GetSponsoredType() == SponsoredType::PromoCatalogCity) {
      api->GetCityGallery(self.mercator, locale, UTM::LargeToponymsPlacepageGallery, resultHandler, errorHandler);
    } else {
      api->GetPoiGallery(self.mercator, locale,
                         info.GetRawTypes(),
                         [MWMFrameworkHelper isWiFiConnected],
                         UTM::SightseeingsPlacepageGallery,
                         resultHandler,
                         errorHandler);
    }
  }
}

- (MWMDiscoveryGuideViewModel *)guideAtIndex:(NSUInteger)index {
  promo::CityGallery::Item const &item = [self.promoGallery galleryItemAtIndex:index];
  return [[MWMDiscoveryGuideViewModel alloc] initWithTitle:@(item.m_name.c_str())
                                                  subtitle:@(item.m_author.m_name.c_str())
                                                     label:@(item.m_luxCategory.m_name.c_str())
                                             labelHexColor:@(item.m_luxCategory.m_color.c_str())
                                                  imageURL:@(item.m_imageUrl.c_str())];
}

@end
