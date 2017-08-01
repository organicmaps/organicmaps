#import "MWMAlertViewController.h"
#import "MWMMapDownloaderTypes.h"
#import "MWMSearchManagerObserver.h"
#import "MWMSearchManagerState.h"
#import "MWMSearchTextField.h"

@interface MWMSearchManager : NSObject

+ (nonnull MWMSearchManager *)manager;
+ (void)addObserver:(nonnull id<MWMSearchManagerObserver>)observer;
+ (void)removeObserver:(nonnull id<MWMSearchManagerObserver>)observer;

@property(nullable, weak, nonatomic) IBOutlet MWMSearchTextField * searchTextField;

@property(nonatomic) MWMSearchManagerState state;

@property(nonnull, nonatomic) IBOutletCollection(UIView) NSArray * topViews;

- (void)mwm_refreshUI;

- (void)searchText:(nonnull NSString *)text forInputLocale:(nullable NSString *)locale;

#pragma mark - Layout

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(nonnull id<UIViewControllerTransitionCoordinator>)coordinator;

@end
