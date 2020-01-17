#import "MWMSearchFilterViewController_Protected.h"
#import "SwiftBridge.h"

@implementation MWMSearchFilterViewController

+ (MWMSearchFilterViewController *)controller
{
  // Must be implemented in subclasses.
  [self doesNotRecognizeSelector:_cmd];
  return nil;
}

+ (MWMSearchFilterViewController *)controllerWithIdentifier:(NSString *)identifier
{
  auto storyboard = [UIStoryboard instance:MWMStoryboardSearchFilters];
  return [storyboard instantiateViewControllerWithIdentifier:identifier];
}
@end
