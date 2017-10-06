#import "MWMPlacePageManagerHelper.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageManager.h"

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMPlacePageManager * placePageManager;

@end

@interface MWMPlacePageManager ()

- (void)updateAvailableArea:(CGRect)frame;
- (void)showUGCAddReview:(MWMRatingSummaryViewValueType)value;

@end

@implementation MWMPlacePageManagerHelper

+ (void)updateAvailableArea:(CGRect)frame
{
  [[MWMMapViewControlsManager manager].placePageManager updateAvailableArea:frame];
}

+ (void)showUGCAddReview:(MWMRatingSummaryViewValueType)value
{
  [[MWMMapViewControlsManager manager].placePageManager showUGCAddReview:value];
}

@end
