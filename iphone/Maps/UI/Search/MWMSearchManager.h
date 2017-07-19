#import "MWMAlertViewController.h"
#import "MWMMapDownloaderTypes.h"
#import "MWMSearchTextField.h"
#import "MWMSearchManagerState.h"

@interface MWMSearchManager : NSObject

@property(nullable, weak, nonatomic) IBOutlet MWMSearchTextField * searchTextField;

@property(nonatomic) MWMSearchManagerState state;

@property(nonnull, nonatomic) IBOutletCollection(UIView) NSArray * topViews;

- (void)mwm_refreshUI;

- (void)searchText:(nonnull NSString *)text forInputLocale:(nullable NSString *)locale;

#pragma mark - Layout

- (void)viewWillTransitionToSize:(CGSize)size
       withTransitionCoordinator:(nonnull id<UIViewControllerTransitionCoordinator>)coordinator;

@end
