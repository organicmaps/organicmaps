#import <CoreApi/MWMMapNodeAttributes.h>
#import <CoreApi/MWMStorage.h>
#import <CoreApi/PlacePageData.h>
#import <CoreApi/PlacePageOSMContributionData.h>
#import <CoreApi/TrackInfo.h>
#import <XCTest/XCTest.h>

#import "MWMCircularProgress.h"
#import "MWMMapDownloadDialog.h"

#include "map/place_page_info.hpp"

@interface PlacePageOSMContributionData (ContributionTesting)
- (instancetype)initWithRawData:(place_page::Info const &)rawData mapStatus:(storage::Status)mapStatus;
@end

@interface PlacePageData (ProgressTesting)
- (void)processCountry:(NSString *)countryId progress:(MWMStorageProgress)progress;
@end

@interface MWMMapDownloadDialog (ProgressTesting)
- (void)configDialog;
- (void)showDownloading:(CGFloat)progress;
- (void)processCountry:(NSString *)countryId progress:(MWMStorageProgress)progress;
@end

@interface MapDownloadProgressSpy : MWMMapDownloadDialog
@property(nonatomic) NSMutableArray<NSNumber *> * received;
@end

@implementation MapDownloadProgressSpy
- (MWMCircularProgress *)progress
{
  // This callback spy is not loaded from the nib and has no progress wrapper.
  return nil;
}
- (void)configDialog
{}
- (void)showDownloading:(CGFloat)progress
{
  [self.received addObject:@(progress)];
}
@end

@interface MWMDownloaderProgressTests : XCTestCase
@end

@implementation MWMDownloaderProgressTests

- (void)testContributionOffersMapUpdateWhenEditingNeedsAnUpdate
{
  place_page::Info info;
  info.SetCountryId("Slovenia_East");
  for (auto const status :
       {storage::Status::NotDownloaded, storage::Status::OnDiskOutOfDate, storage::Status::DownloadFailed})
  {
    auto data = [[PlacePageOSMContributionData alloc] initWithRawData:info mapStatus:status];
    XCTAssertNotNil(data);
    XCTAssertEqual(data.state, PlacePageOSMContributionStateShouldUpdateMap);
  }
}

- (void)testContributionWaitsWhileTheMapIsDownloading
{
  place_page::Info info;
  info.SetCountryId("Slovenia_East");
  for (bool const canEdit : {false, true})
  {
    info.SetCanEditOrAdd(canEdit);
    for (auto const status : {storage::Status::Downloading, storage::Status::Applying, storage::Status::InQueue})
    {
      auto data = [[PlacePageOSMContributionData alloc] initWithRawData:info mapStatus:status];
      XCTAssertEqual(data.state, PlacePageOSMContributionStateMapIsDownloading);
    }
  }
}

- (void)testContributionKeepsEditingAnEditableOutdatedMap
{
  place_page::Info info;
  info.SetCountryId("Slovenia_East");
  info.SetCanEditOrAdd(true);
  for (auto const status : {storage::Status::OnDisk, storage::Status::OnDiskOutOfDate, storage::Status::DownloadFailed})
  {
    auto data = [[PlacePageOSMContributionData alloc] initWithRawData:info mapStatus:status];
    XCTAssertEqual(data.state, PlacePageOSMContributionStateCanAddOrEditPlace);
    XCTAssertTrue(data.canEditPlace);
    XCTAssertTrue(data.showAddPlace);
  }
}

- (void)testOnMapDialogShowsOnlyMapProgress
{
  auto dialog = [[MapDownloadProgressSpy alloc] initWithFrame:CGRectZero];
  dialog.received = [NSMutableArray array];
  [dialog processViewportCountryEvent:"Slovenia_East"];
  [dialog processCountry:@"Slovenia_East" progress:MWMStorageProgress{1, 10, 2, 20}];
  auto parent = [[UIView alloc] initWithFrame:CGRectZero];
  [parent addSubview:dialog];
  [dialog processCountry:@"Other" progress:MWMStorageProgress{1, 10, 2, 20}];
  [dialog processCountry:@"Slovenia_East" progress:MWMStorageProgress{0, 0, 0, 0}];
  [dialog processCountry:@"Slovenia_East" progress:MWMStorageProgress{5, 10, 100, 100}];
  [dialog processCountry:@"Slovenia_East" progress:MWMStorageProgress{10, 10, 0, 999}];
  [dialog processCountry:@"Slovenia_East" progress:MWMStorageProgress{0, 0, 15, 20}];
  XCTAssertEqualObjects(dialog.received, (@[@0.5, @1]));
  XCTAssertEqual(dialog.superview, parent);
}

- (void)testPlacePageCombinesMapAndTerrainProgress
{
  auto data = [[PlacePageData alloc] initWithTrackInfo:TrackInfo.emptyInfo elevationInfo:nil];
  auto attrs = [[MWMStorage sharedStorage] attributesForCountry:@"Slovenia_East"];
  [data setValue:attrs forKey:@"mapNodeAttributes"];
  NSMutableArray<NSArray<NSNumber *> *> * received = [NSMutableArray array];
  data.onMapNodeProgressUpdate =
      ^(uint64_t downloaded, uint64_t total) { [received addObject:@[@(downloaded), @(total)]]; };
  [data processCountry:@"Other" progress:MWMStorageProgress{1, 10, 2, 20}];
  [data processCountry:@"Slovenia_East" progress:MWMStorageProgress{0, 0, 0, 0}];
  [data processCountry:@"Slovenia_East" progress:MWMStorageProgress{10, 10, 5, 20}];
  [data processCountry:@"Slovenia_East" progress:MWMStorageProgress{0, 0, 15, 20}];
  XCTAssertEqualObjects(received, (@[@[@15, @30], @[@15, @20]]));
}

@end
