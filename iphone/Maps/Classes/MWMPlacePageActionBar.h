#import <UIKit/UIKit.h>

@class MWMPlacePage;

@interface MWMPlacePageActionBar : UIView

@property (nonatomic) BOOL isBookmark;

+ (MWMPlacePageActionBar *)actionBarForPlacePage:(MWMPlacePage *)placePage;
- (void)configureWithPlacePage:(MWMPlacePage *)placePage;

- (instancetype)init __attribute__((unavailable("call actionBarForPlacePage: instead")));
- (instancetype)initWithCoder:(NSCoder *)aDecoder __attribute__((unavailable("call actionBarForPlacePage: instead")));
- (instancetype)initWithFrame:(CGRect)frame __attribute__((unavailable("call actionBarForPlacePage: instead")));

@end
