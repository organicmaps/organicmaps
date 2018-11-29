#import "MWMPlacePageManagerHelper.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageManager.h"
#import "MWMUGCReviewSource.h"

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMPlacePageManager * placePageManager;

@end

@interface MWMPlacePageManager ()

- (void)updateAvailableArea:(CGRect)frame;
- (void)showUGCAddReview:(MWMRatingSummaryViewValueType)value fromPreview:(BOOL)fromPreview;
- (void)searchSimilar;

@end

@implementation MWMPlacePageManagerHelper

+ (void)updateAvailableArea:(CGRect)frame
{
  [[MWMMapViewControlsManager manager].placePageManager updateAvailableArea:frame];
}

+ (void)showUGCAddReview:(MWMRatingSummaryViewValueType)value fromSource:(MWMUGCReviewSource)source
{
  [[MWMMapViewControlsManager manager].placePageManager showUGCAddReview:value
                                                             fromSource:source];
}

+ (void)searchSimilar
{
  [[MWMMapViewControlsManager manager].placePageManager searchSimilar];
}

@end
