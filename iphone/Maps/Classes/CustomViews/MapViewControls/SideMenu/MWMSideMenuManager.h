#import <UIKit/UIKit.h>

@class MapViewController;
@protocol MWMSideMenuManagerProtocol;

typedef NS_ENUM(NSUInteger, MWMSideMenuState)
{
  MWMSideMenuStateHidden,
  MWMSideMenuStateInactive,
  MWMSideMenuStateActive
};

@interface MWMSideMenuManager : NSObject

@property (nonatomic) MWMSideMenuState state;
@property (nonatomic, readonly) CGRect menuButtonFrameWithSpacing;

- (instancetype)init __attribute__((unavailable("init is not available")));
- (instancetype)initWithParentController:(MapViewController *)controller delegate:(id<MWMSideMenuManagerProtocol>)delegate;

@end
