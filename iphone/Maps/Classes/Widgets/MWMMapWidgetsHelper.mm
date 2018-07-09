#import "MWMMapWidgetsHelper.h"
#import "MWMMapWidgets.h"

@implementation MWMMapWidgetsHelper

+ (void)updateAvailableArea:(CGRect)frame
{
  [[MWMMapWidgets widgetsManager] updateAvailableArea:frame];
}

@end
