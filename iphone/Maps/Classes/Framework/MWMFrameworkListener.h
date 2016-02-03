#import "MWMFrameworkObservers.h"

@interface MWMFrameworkListener : NSObject

+ (MWMFrameworkListener *)listener;

@property (nonatomic, readonly) location::EMyPositionMode myPositionMode;
@property (nonatomic) UserMark const * userMark;

- (void)addObserver:(id<MWMFrameworkObserver>)observer;

@end
