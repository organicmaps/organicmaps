#import <CoreApi/MWMStorage.h>
#import <CoreApi/PlacePageData.h>
#import <CoreApi/TrackInfo.h>
#import <XCTest/XCTest.h>

#import "MWMAutoupdateController.h"
#import "MWMMapDownloadDialog.h"

@interface MWMAutoupdateController (ProgressTesting)
- (void)dismiss;
- (void)updateProcessStatus:(storage::CountryId const &)countryId;
- (void)processCountryEvent:(NSString *)countryId;
@end

@interface AutoupdateProgressSpy : MWMAutoupdateController
@property(nonatomic) BOOL dismissed;
@end

@implementation AutoupdateProgressSpy
- (void)dismiss
{
  self.dismissed = YES;
}
- (void)updateProcessStatus:(storage::CountryId const &)countryId
{}
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

- (void)testUninstalledTerrainNeighbourDoesNotBlockAutoupdateCompletion
{
  auto const & storage = GetFramework().GetStorage();
  storage::CountryId missing;
  storage.ForEachInSubtree(storage.GetRootId(), [&](storage::CountryId const & id, bool /* group */)
  {
    storage::NodeStatuses statuses;
    storage.GetNodeStatuses(id, statuses);
    if (!statuses.m_groupNode && statuses.m_status == storage::NodeStatus::NotDownloaded)
      missing = id;
  });
  XCTAssertFalse(missing.empty());
  auto controller = [[AutoupdateProgressSpy alloc] init];
  [controller setValue:@YES forKey:@"progressFinished"];
  [controller processCountryEvent:@(missing.c_str())];
  XCTAssertTrue(controller.dismissed);
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
