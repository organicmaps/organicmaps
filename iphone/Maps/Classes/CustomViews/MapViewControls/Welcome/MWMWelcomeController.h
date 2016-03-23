#import "MWMViewController.h"

@class MWMPageController;

@protocol MWMWelcomeControllerProtocol <NSObject>

+ (UIStoryboard *)welcomeStoryboard;
+ (instancetype)welcomeController;
+ (NSInteger)pagesCount;
+ (NSString *)udWelcomeWasShownKey;

@end

@interface MWMWelcomeController : MWMViewController <MWMWelcomeControllerProtocol>

@property (nonatomic) NSInteger pageIndex;
@property (weak, nonatomic) MWMPageController * pageController;

@property (nonatomic) CGSize size;

@end

using TMWMWelcomeConfigBlock = void (^)(MWMWelcomeController * controller);