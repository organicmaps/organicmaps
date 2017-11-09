#import "MWMPlacePageManagerHelper.h"
#import "MWMMapViewControlsManager.h"
#import "MWMPlacePageManager.h"

@interface MWMMapViewControlsManager ()

@property(nonatomic) MWMPlacePageManager * placePageManager;

@end

@interface MWMPlacePageManager ()

- (void)updateAvailableArea:(CGRect)frame;
- (void)showUGCAddReview:(MWMRatingSummaryViewValueType)value fromPreview:(BOOL)fromPreview;

@end

@implementation MWMPlacePageManagerHelper

+ (void)updateAvailableArea:(CGRect)frame
{
  [[MWMMapViewControlsManager manager].placePageManager updateAvailableArea:frame];
}

+ (void)showUGCAddReview:(MWMRatingSummaryViewValueType)value fromPreview:(BOOL)fromPreview
{
  [[MWMMapViewControlsManager manager].placePageManager showUGCAddReview:value
                                                             fromPreview:fromPreview];
}

@end
