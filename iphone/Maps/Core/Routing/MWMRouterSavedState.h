@class MWMRoutePoint;

enum class MWMRouterForceStateChange
{
  None,
  Rebuild,
  Start
};

@interface MWMRouterSavedState : NSObject

@property(nonatomic, readonly) MWMRoutePoint * restorePoint;
@property(nonatomic) MWMRouterForceStateChange forceStateChange;

+ (instancetype)state;

+ (void)store;
+ (void)remove;
+ (void)restore;

@end
