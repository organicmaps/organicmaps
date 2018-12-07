#import "MWMPlacePageLayout.h"
#import "MWMBookmarkCell.h"
#import "MWMPlaceDescriptionCell.h"
#import "MWMOpeningHoursLayoutHelper.h"
#import "MWMPPPreviewLayoutHelper.h"
#import "MWMPPReviewCell.h"
#import "MWMPPView.h"
#import "MWMPlacePageButtonCell.h"
#import "MWMPlacePageCellUpdateProtocol.h"
#import "MWMPlacePageData.h"
#import "MWMPlacePageRegularCell.h"
#import "MWMUGCViewModel.h"
#import "MWMiPadPlacePageLayoutImpl.h"
#import "MWMiPhonePlacePageLayoutImpl.h"
#import "MapViewController.h"
#import "SwiftBridge.h"

#include "partners_api/booking_api.hpp"

#include "storage/storage_defines.hpp"

namespace
{
using place_page::MetainfoRows;

map<MetainfoRows, Class> const kMetaInfoCells = {
    {MetainfoRows::Website, [MWMPlacePageLinkCell class]},
    {MetainfoRows::Address, [MWMPlacePageInfoCell class]},
    {MetainfoRows::Email, [MWMPlacePageLinkCell class]},
    {MetainfoRows::Phone, [MWMPlacePageLinkCell class]},
    {MetainfoRows::Cuisine, [MWMPlacePageInfoCell class]},
    {MetainfoRows::Operator, [MWMPlacePageInfoCell class]},
    {MetainfoRows::Coordinate, [MWMPlacePageInfoCell class]},
    {MetainfoRows::Internet, [MWMPlacePageInfoCell class]}};
}  // namespace

@interface MWMPlacePageLayout ()<UITableViewDataSource, MWMPlacePageCellUpdateProtocol,
                                 MWMPlacePageViewUpdateProtocol>

@property(weak, nonatomic) MWMPlacePageData * data;

@property(weak, nonatomic) UIView * ownerView;
@property(weak, nonatomic)
    id<MWMPlacePageLayoutDelegate, MWMPlacePageButtonsProtocol, MWMActionBarProtocol>
        delegate;
@property(weak, nonatomic) id<MWMPlacePageLayoutDataSource> dataSource;
@property(nonatomic) IBOutlet MWMPPView * placePageView;

@property(nonatomic) MWMPlacePageActionBar * actionBar;

@property(nonatomic) BOOL isPlacePageButtonsEnabled;
@property(nonatomic) BOOL isDownloaderViewShown;

@property(nonatomic) id<MWMPlacePageLayoutImpl> layoutImpl;

@property(nonatomic) MWMPPPreviewLayoutHelper * previewLayoutHelper;
@property(nonatomic) MWMOpeningHoursLayoutHelper * openingHoursLayoutHelper;

@property(weak, nonatomic) MWMPlacePageTaxiCell * taxiCell;
@property(weak, nonatomic) MWMPPViatorCarouselCell * viatorCell;

@property(nonatomic) BOOL buttonsSectionEnabled;

@end

@implementation MWMPlacePageLayout

- (instancetype)initWithOwnerView:(UIView *)view
                         delegate:(id<MWMPlacePageLayoutDelegate, MWMPlacePageButtonsProtocol,
                                      MWMActionBarProtocol>)delegate
                       dataSource:(id<MWMPlacePageLayoutDataSource>)dataSource
{
  self = [super init];
  if (self)
  {
    _ownerView = view;
    _delegate = delegate;
    _dataSource = dataSource;
    [NSBundle.mainBundle loadNibNamed:[MWMPPView className] owner:self options:nil];
    _placePageView.delegate = self;

    auto tableView = _placePageView.tableView;
    _previewLayoutHelper = [[MWMPPPreviewLayoutHelper alloc] initWithTableView:tableView];
    [self registerCells];
  }
  return self;
}

- (void)registerCells
{
  auto tv = self.placePageView.tableView;
  [tv registerWithCellClass:[MWMPlacePageButtonCell class]];
  [tv registerWithCellClass:[MWMPlaceDescriptionCell class]];
  [tv registerWithCellClass:[MWMBookmarkCell class]];
  [tv registerWithCellClass:[MWMPPHotelDescriptionCell class]];
  [tv registerWithCellClass:[MWMPPHotelCarouselCell class]];
  [tv registerWithCellClass:[MWMPPViatorCarouselCell class]];
  [tv registerWithCellClass:[MWMPPReviewHeaderCell class]];
  [tv registerWithCellClass:[MWMPPReviewCell class]];
  [tv registerWithCellClass:[MWMPPFacilityCell class]];
  [tv registerWithCellClass:[MWMPlacePageTaxiCell class]];
  [tv registerWithCellClass:[MWMUGCSummaryRatingCell class]];
  [tv registerWithCellClass:[MWMUGCAddReviewCell class]];
  [tv registerWithCellClass:[MWMUGCYourReviewCell class]];
  [tv registerWithCellClass:[MWMUGCReviewCell class]];

  // Register all meta info cells.
  for (auto const & pair : kMetaInfoCells)
    [tv registerWithCellClass:pair.second];
}

- (UIView *)shareAnchor { return self.actionBar.shareAnchor; }
- (void)showWithData:(MWMPlacePageData *)data
{
  self.data = data;

  [self.layoutImpl onShow];
  [self checkCellsVisible];

  dispatch_async(dispatch_get_main_queue(), ^{
    [data fillOnlineBookingSections];
    [data fillOnlineViatorSection];
  });
}

- (void)rotateDirectionArrowToAngle:(CGFloat)angle
{
  [self.previewLayoutHelper rotateDirectionArrowToAngle:angle];
}

- (void)setDistanceToObject:(NSString *)distance
{
  [self.previewLayoutHelper setDistanceToObject:distance];
}

- (void)setSpeedAndAltitude:(NSString *)speedAndAltitude
{
  [self.previewLayoutHelper setSpeedAndAltitude:speedAndAltitude];
}

- (void)setButtonsSectionEnabled:(BOOL)buttonsSectionEnabled
{
  if (_buttonsSectionEnabled == buttonsSectionEnabled)
    return;
  _buttonsSectionEnabled = buttonsSectionEnabled;
  dispatch_async(dispatch_get_main_queue(), ^{
    auto data = self.data;
    auto tv = self.placePageView.tableView;
    if (!data || !tv)
      return;

    auto const & sections = data.sections;
    auto const it = find(sections.begin(), sections.end(), place_page::Sections::Buttons);
    if (it == sections.end())
      return;
    NSInteger const sectionNumber = distance(sections.begin(), it);
    [tv reloadSections:[NSIndexSet indexSetWithIndex:sectionNumber]
        withRowAnimation:UITableViewRowAnimationNone];
  });
}

- (MWMPlacePageActionBar *)actionBar
{
  if (!_actionBar)
  {
    _actionBar = [MWMPlacePageActionBar actionBarWithDelegate:self.delegate];
    self.layoutImpl.actionBar = _actionBar;
  }
  return _actionBar;
}

- (void)close
{
  [self.layoutImpl onClose];
}

- (void)mwm_refreshUI
{
  [self.placePageView mwm_refreshUI];
  [self.actionBar mwm_refreshUI];
}

- (void)reloadBookmarkSection:(BOOL)isBookmark
{
  auto data = self.data;
  if (!data)
    return;
  auto tv = self.placePageView.tableView;
  if (!tv)
    return;
  [tv update:^{
    auto set =
        [NSIndexSet indexSetWithIndex:static_cast<NSInteger>(place_page::Sections::Bookmark)];
    if (isBookmark)
      [tv insertSections:set withRowAnimation:UITableViewRowAnimationAutomatic];
    else
      [tv deleteSections:set withRowAnimation:UITableViewRowAnimationAutomatic];

    auto const & previewRows = data.previewRows;
    auto const previewIT =
        std::find(previewRows.cbegin(), previewRows.cend(), place_page::PreviewRows::Subtitle);
    if (previewIT != previewRows.cend())
    {
      auto previewIP =
          [NSIndexPath indexPathForRow:std::distance(previewRows.cbegin(), previewIT)
                             inSection:static_cast<NSInteger>(place_page::Sections::Preview)];
      [tv reloadRowsAtIndexPaths:@[ previewIP ] withRowAnimation:UITableViewRowAnimationAutomatic];
    }
  }];
}

#pragma mark - Downloader event

- (void)processDownloaderEventWithStatus:(storage::NodeStatus)status progress:(CGFloat)progress
{
  auto data = self.data;
  if (!data)
    return;

  using namespace storage;

  switch (status)
  {
  case NodeStatus::OnDiskOutOfDate:
  case NodeStatus::Undefined:
  {
    self.buttonsSectionEnabled = NO;
    self.actionBar.isAreaNotDownloaded = NO;
    break;
  }
  case NodeStatus::Downloading:
  {
    self.actionBar.isAreaNotDownloaded = YES;
    self.actionBar.downloadingProgress = progress;
    break;
  }
  case NodeStatus::InQueue:
  case NodeStatus::Applying:
  {
    self.actionBar.isAreaNotDownloaded = YES;
    self.actionBar.downloadingState = MWMCircularProgressStateSpinner;
    break;
  }
  case NodeStatus::Error:
  {
    self.actionBar.isAreaNotDownloaded = YES;
    self.actionBar.downloadingState = MWMCircularProgressStateFailed;
    break;
  }
  case NodeStatus::OnDisk:
  {
    self.buttonsSectionEnabled = YES;
    self.actionBar.isAreaNotDownloaded = NO;
    break;
  }
  case NodeStatus::Partly:
  case NodeStatus::NotDownloaded:
  {
    self.buttonsSectionEnabled = NO;
    self.actionBar.isAreaNotDownloaded = YES;
    self.actionBar.downloadingState = MWMCircularProgressStateNormal;
    break;
  }
  }
}

#pragma mark - AvailableArea / PlacePageArea

- (void)updateAvailableArea:(CGRect)frame { [self.layoutImpl updateAvailableArea:frame]; }
#pragma mark - UITableViewDelegate & UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
  auto data = self.data;
  if (!data)
    return 0;
  return data.sections.size();
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
  using namespace place_page;

  auto data = self.data;
  if (!data)
    return 0;
  switch (data.sections[section])
  {
  case Sections::Bookmark: return 1;
  case Sections::Description: return 1;
  case Sections::Preview: return data.previewRows.size();
  case Sections::SpecialProjects: return data.specialProjectRows.size();
  case Sections::Metainfo: return data.metainfoRows.size();
  case Sections::Ad: return data.adRows.size();
  case Sections::Buttons: return data.buttonsRows.size();
  case Sections::HotelPhotos: return data.photosRows.size();
  case Sections::HotelDescription: return data.descriptionRows.size();
  case Sections::HotelFacilities: return data.hotelFacilitiesRows.size();
  case Sections::HotelReviews: return data.hotelReviewsRows.size();
  case Sections::UGCRating: return data.ugc.ratingCellsCount;
  case Sections::UGCAddReview: return data.ugc.addReviewCellsCount;
  case Sections::UGCReviews: return data.ugc.reviewRows.size();
  }
}

- (UITableViewCell *)tableView:(UITableView *)tableView
         cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
  using namespace place_page;

  auto data = self.data;
  if (!data)
    return [[UITableViewCell alloc] init];

  id<MWMPlacePageButtonsProtocol> delegate = self.delegate;
  switch (data.sections[indexPath.section])
  {
  case Sections::Preview:
  {
    return [self.previewLayoutHelper cellForRowAtIndexPath:indexPath withData:data];
    }
    case Sections::Description:
    {
      Class cls = [MWMPlaceDescriptionCell class];
      auto c = (MWMPlaceDescriptionCell *)([tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [c configureWithDescription:data.placeDescription delegate:delegate];
      return c;
    }
    case Sections::Bookmark:
    {
      Class cls = [MWMBookmarkCell class];
      auto c = static_cast<MWMBookmarkCell *>(
          [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [c configureWithText:data.bookmarkDescription
        updateCellDelegate:self
      editBookmarkDelegate:delegate
                    isHTML:data.isHTMLDescription
                isEditable:!data.isBookmarkFromCatalog];
      return c;
    }
    case Sections::Metainfo:
    {
      auto const row = data.metainfoRows[indexPath.row];
      switch (row)
      {
      case MetainfoRows::OpeningHours:
      case MetainfoRows::ExtendedOpeningHours:
      {
        auto const & metaInfo = data.metainfoRows;
        NSAssert(std::find(metaInfo.cbegin(), metaInfo.cend(), MetainfoRows::OpeningHours) !=
                     metaInfo.cend(),
                 @"OpeningHours is not available");
        return [self.openingHoursLayoutHelper cellForRowAtIndexPath:indexPath];
      }
      case MetainfoRows::Phone:
      case MetainfoRows::Address:
      case MetainfoRows::Website:
      case MetainfoRows::Email:
      case MetainfoRows::Cuisine:
      case MetainfoRows::Operator:
      case MetainfoRows::Internet:
      case MetainfoRows::Coordinate:
      {
        Class cls = kMetaInfoCells.at(row);
        auto c = static_cast<MWMPlacePageRegularCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWithRow:row data:data];
        return c;
      }
      case MetainfoRows::LocalAdsCustomer:
      case MetainfoRows::LocalAdsCandidate:
      {
        Class cls = [MWMPlacePageButtonCell class];
        auto c = static_cast<MWMPlacePageButtonCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWithTitle:[data stringForRow:row]
                     action:^{
                       [delegate openLocalAdsURL];
                     }
              isInsetButton:NO];
        return c;
      }
      }
    }
    case Sections::Ad:
    {
      Class cls = [MWMPlacePageTaxiCell class];
      auto c = static_cast<MWMPlacePageTaxiCell *>(
          [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      auto const & taxiProviders = [data taxiProviders];
      NSAssert(!taxiProviders.empty(), @"TaxiProviders can not be empty");
      auto const & provider = taxiProviders.front();
      auto type = MWMPlacePageTaxiProviderTaxi;
      switch (provider)
      {
      case taxi::Provider::Uber: type = MWMPlacePageTaxiProviderUber; break;
      case taxi::Provider::Yandex: type = MWMPlacePageTaxiProviderYandex; break;
      case taxi::Provider::Maxim: type = MWMPlacePageTaxiProviderMaxim; break;
      case taxi::Provider::Rutaxi: type = MWMPlacePageTaxiProviderRutaxi; break;
      case taxi::Provider::Count: LOG(LERROR, ("Incorrect taxi provider")); break;
      }
      [c configWithType:type delegate:delegate];
      self.taxiCell = c;
      return c;
    }
    case Sections::Buttons:
    {
      Class cls = [MWMPlacePageButtonCell class];
      auto c = static_cast<MWMPlacePageButtonCell *>(
          [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      auto const row = data.buttonsRows[indexPath.row];

      [c configForRow:row
            withAction:^{
              switch (row)
              {
              case ButtonsRows::AddPlace: [delegate addPlace]; break;
              case ButtonsRows::EditPlace: [delegate editPlace]; break;
              case ButtonsRows::AddBusiness: [delegate addBusiness]; break;
              case ButtonsRows::HotelDescription: [delegate book:NO]; break;
              case ButtonsRows::Other: NSAssert(false, @"Incorrect row");
              }
            }];
      // Hotel description button is always enabled.
      c.enabled = self.buttonsSectionEnabled || (row == ButtonsRows::HotelDescription);
      return c;
    }
    case Sections::SpecialProjects:
    {
      switch (data.specialProjectRows[indexPath.row])
      {
      case SpecialProject::Viator:
      {
        Class cls = [MWMPPViatorCarouselCell class];
        auto c = static_cast<MWMPPViatorCarouselCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWith:data.viatorItems delegate:delegate];
        self.viatorCell = c;
        return c;
      }
      }
    }
    case Sections::HotelPhotos:
    {
      Class cls = [MWMPPHotelCarouselCell class];
      auto c = static_cast<MWMPPHotelCarouselCell *>(
          [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      [c configWith:data.photos delegate:delegate];
      return c;
    }
    case Sections::HotelFacilities:
    {
      auto const row = data.hotelFacilitiesRows[indexPath.row];
      switch (row)
      {
      case HotelFacilitiesRow::Regular:
      {
        Class cls = [MWMPPFacilityCell class];
        auto c = static_cast<MWMPPFacilityCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWith:@(data.facilities[indexPath.row].m_name.c_str())];
        return c;
      }
      case HotelFacilitiesRow::ShowMore:
        Class cls = [MWMPlacePageButtonCell class];
        auto c = static_cast<MWMPlacePageButtonCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWithTitle:L(@"booking_show_more")
                     action:^{
                       [delegate showAllFacilities];
                     }
              isInsetButton:NO];
        return c;
      }
    }
    case Sections::HotelReviews:
    {
      auto const row = data.hotelReviewsRows[indexPath.row];
      switch (row)
      {
      case HotelReviewsRow::Header:
      {
        Class cls = [MWMPPReviewHeaderCell class];
        auto c = static_cast<MWMPPReviewHeaderCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWithRating:data.bookingRating numberOfReviews:data.numberOfHotelReviews];
        return c;
      }
      case HotelReviewsRow::Regular:
      {
        Class cls = [MWMPPReviewCell class];
        auto c = static_cast<MWMPPReviewCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWithReview:data.hotelReviews[indexPath.row - 1]];
        return c;
      }
      case HotelReviewsRow::ShowMore:
      {
        Class cls = [MWMPlacePageButtonCell class];
        auto c = static_cast<MWMPlacePageButtonCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);

        [c configWithTitle:L(@"reviews_on_bookingcom")
                     action:^{
                       [delegate book:NO];
                     }
              isInsetButton:NO];
        return c;
      }
      }
    }
    case Sections::HotelDescription:
    {
      auto const row = data.descriptionRows[indexPath.row];
      switch (row)
      {
      case HotelDescriptionRow::Regular:
      {
        Class cls = [MWMPPHotelDescriptionCell class];
        auto c = static_cast<MWMPPHotelDescriptionCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWith:data.hotelDescription delegate:self];
        return c;
      }
      case HotelDescriptionRow::ShowMore:
      {
        Class cls = [MWMPlacePageButtonCell class];
        auto c = static_cast<MWMPlacePageButtonCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWithTitle:L(@"more_on_bookingcom")
                     action:^{
                       [delegate book:NO];
                       ;
                     }
              isInsetButton:NO];
        return c;
      }
      }
    }
    case Sections::UGCRating:
    {
      Class cls = [MWMUGCSummaryRatingCell class];
      auto c = static_cast<MWMUGCSummaryRatingCell *>(
          [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      auto ugc = data.ugc;
      [c configWithReviewsCount:[ugc numberOfRatings]
                   summaryRating:[ugc summaryRating]
                         ratings:[ugc ratings]];
      return c;
    }
    case Sections::UGCAddReview:
    {
      Class cls = [MWMUGCAddReviewCell class];
      auto c = static_cast<MWMUGCAddReviewCell *>(
          [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
      c.onRateTap = ^(MWMRatingSummaryViewValueType value) {
        [delegate showUGCAddReview:value fromSource:MWMUGCReviewSourcePlacePage];
      };
      return c;
    }
    case Sections::UGCReviews:
    {
      auto ugc = data.ugc;
      auto const & reviewRows = ugc.reviewRows;
      using namespace ugc::view_model;
      auto onUpdate = ^{
        [tableView refresh];
      };

      switch (reviewRows[indexPath.row])
      {
      case ReviewRow::YourReview:
      {
        Class cls = [MWMUGCYourReviewCell class];
        auto c = static_cast<MWMUGCYourReviewCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWithYourReview:static_cast<MWMUGCYourReview *>([ugc reviewWithIndex:indexPath.row])
                        onUpdate:onUpdate];
        return c;
      }
      case ReviewRow::Review:
      {
        Class cls = [MWMUGCReviewCell class];
        auto c = static_cast<MWMUGCReviewCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWithReview:static_cast<MWMUGCReview *>([ugc reviewWithIndex:indexPath.row])
                    onUpdate:onUpdate];
        return c;
      }
      case ReviewRow::MoreReviews:
      {
        Class cls = [MWMPlacePageButtonCell class];
        auto c = static_cast<MWMPlacePageButtonCell *>(
            [tableView dequeueReusableCellWithCellClass:cls indexPath:indexPath]);
        [c configWithTitle:L(@"placepage_more_reviews_button")
                     action:^{
                       [delegate openReviews:ugc];
                     }
              isInsetButton:NO];
        return c;
      }
      }
    }
  }
}

- (void)checkCellsVisible
{
  auto data = self.data;
  if (!data)
    return;

  auto const checkCell = ^(UITableViewCell * cell, MWMVoidBlock onHit) {
    if (!cell)
      return;
    auto taxiBottom = CGPointMake(cell.width / 2, cell.height);
    auto mainView = [MapViewController sharedController].view;
    auto actionBar = self.actionBar;
    BOOL const isInMainView =
        [mainView pointInside:[cell convertPoint:taxiBottom toView:mainView] withEvent:nil];
    BOOL const isInActionBar =
        [actionBar pointInside:[cell convertPoint:taxiBottom toView:actionBar] withEvent:nil];
    if (isInMainView && !isInActionBar)
      onHit();
  };

  checkCell(self.taxiCell, ^{
    self.taxiCell = nil;

    auto const & taxiProviders = [data taxiProviders];
    if (taxiProviders.empty())
    {
      NSAssert(NO, @"Taxi is shown but providers are empty.");
      return;
    }
    NSString * provider = nil;
    switch (taxiProviders.front())
    {
    case taxi::Provider::Uber: provider = kStatUber; break;
    case taxi::Provider::Yandex: provider = kStatYandex; break;
    case taxi::Provider::Maxim: provider = kStatMaxim; break;
    case taxi::Provider::Rutaxi: provider = kStatRutaxi; break;
    case taxi::Provider::Count: LOG(LERROR, ("Incorrect taxi provider")); break;
    }
    [Statistics logEvent:kStatPlacepageTaxiShow
          withParameters:@{kStatProvider: provider, kStatPlacement: kStatPlacePage}];
  });

  checkCell(self.viatorCell, ^{
    self.viatorCell = nil;
    
    auto viatorItems = data.viatorItems;
    if (viatorItems.count == 0)
    {
      NSAssert(NO, @"Viator is shown but items are empty.");
      return;
    }
    [Statistics logEvent:kStatPlacepageSponsoredShow
          withParameters:@{kStatProvider: kStatViator, kStatPlacement: kStatPlacePage}];
  });
}

#pragma mark - MWMOpeningHoursLayoutHelper

- (MWMOpeningHoursLayoutHelper *)openingHoursLayoutHelper
{
  if (!_openingHoursLayoutHelper)
    _openingHoursLayoutHelper =
        [[MWMOpeningHoursLayoutHelper alloc] initWithTableView:self.placePageView.tableView];
  return _openingHoursLayoutHelper;
}

#pragma mark - MWMPlacePageLayoutImpl

- (id<MWMPlacePageLayoutImpl>)layoutImpl
{
  if (!_layoutImpl)
  {
    auto const Impl =
        IPAD ? [MWMiPadPlacePageLayoutImpl class] : [MWMiPhonePlacePageLayoutImpl class];
    _layoutImpl = [[Impl alloc] initOwnerView:self.ownerView
                                placePageView:self.placePageView
                                     delegate:self.delegate];
    if ([_layoutImpl respondsToSelector:@selector(setPreviewLayoutHelper:)])
      [_layoutImpl setPreviewLayoutHelper:self.previewLayoutHelper];
  }
  return _layoutImpl;
}

#pragma mark - MWMPlacePageCellUpdateProtocol

- (void)cellUpdated
{
  auto const update = @selector(update);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:update object:nil];
  [self performSelector:update withObject:nil afterDelay:0.1];
}

- (void)update
{
  auto data = self.data;
  if (data)
    [self.placePageView.tableView refresh];
}

#pragma mark - MWMPlacePageViewUpdateProtocol

- (void)updateLayout
{
  auto const sel = @selector(updateContentLayout);
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:sel object:nil];
  [self performSelector:sel withObject:nil afterDelay:0.1];
}

- (void)updateContentLayout { [self.layoutImpl updateContentLayout]; }
#pragma mark - Properties

- (void)setData:(MWMPlacePageData *)data
{
  if (_data == data)
    return;
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  _data = data;

  if (!data)
    return;

  data.refreshPreviewCallback = ^{
    auto tv = self.placePageView.tableView;
    [tv reloadSections:[NSIndexSet indexSetWithIndex:0]
        withRowAnimation:UITableViewRowAnimationFade];
    dispatch_async(dispatch_get_main_queue(), ^{
      [self.previewLayoutHelper notifyHeightWashChanded];
    });
  };

  data.sectionsAreReadyCallback = ^(NSRange const & range, MWMPlacePageData * d, BOOL isSection) {
    if (![self.data isEqual:d])
      return;
    
    auto tv = self.placePageView.tableView;
    if (isSection) {
      [tv insertSections:[NSIndexSet indexSetWithIndexesInRange:range]
        withRowAnimation:UITableViewRowAnimationAutomatic];
    }
    else
    {
      NSMutableArray<NSIndexPath *> * indexPaths = [@[] mutableCopy];
      for (auto i = 1; i < range.length + 1; i++)
        [indexPaths addObject:[NSIndexPath indexPathForRow:i inSection:range.location]];
      
      [tv insertRowsAtIndexPaths:indexPaths withRowAnimation:UITableViewRowAnimationAutomatic];
    }
  };

  data.bannerIsReadyCallback = ^{
    [self.previewLayoutHelper insertRowAtTheEnd];
  };

  [self.actionBar configureWithData:data];
  [self.previewLayoutHelper configWithData:data];
  auto const & metaInfo = data.metainfoRows;
  auto const hasOpeningHours =
      std::find(metaInfo.cbegin(), metaInfo.cend(), MetainfoRows::OpeningHours) != metaInfo.cend();
  if (hasOpeningHours)
    [self.openingHoursLayoutHelper configWithData:data];

  [self.placePageView.tableView reloadData];

  self.buttonsSectionEnabled = YES;
}

@end
