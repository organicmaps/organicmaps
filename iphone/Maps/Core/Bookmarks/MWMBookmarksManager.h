#import "MWMBookmarksObserver.h"

@interface MWMBookmarksManager : NSObject

+ (instancetype)manager;

+ (void)addObserver:(id<MWMBookmarksObserver>)observer;
+ (void)removeObserver:(id<MWMBookmarksObserver>)observer;

+ (BOOL)areBookmarksLoaded;
+ (void)loadBookmarks;

- (instancetype)init __attribute__((unavailable("call +manager instead")));
- (instancetype)copy __attribute__((unavailable("call +manager instead")));
- (instancetype)copyWithZone:(NSZone *)zone __attribute__((unavailable("call +manager instead")));
+ (instancetype)alloc __attribute__((unavailable("call +manager instead")));
+ (instancetype)allocWithZone:(struct _NSZone *)zone
    __attribute__((unavailable("call +manager instead")));
+ (instancetype) new __attribute__((unavailable("call +manager instead")));

@end
