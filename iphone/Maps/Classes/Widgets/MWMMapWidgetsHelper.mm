#import "MWMMapWidgetsHelper.h"
#import "MWMMapWidgets.h"

@implementation MWMMapWidgetsHelper

+ (void)updateAvailableArea:(CGRect)frame
{
  [[MWMMapWidgets widgetsManager] updateAvailableArea:frame];
}

+ (void)updateLayout:(CGRect)frame
{
  [[MWMMapWidgets widgetsManager] updateLayout:frame];
}

+ (void)updateLayoutForAvailableArea
{
  [[MWMMapWidgets widgetsManager] updateLayoutForAvailableArea];
}

@end
