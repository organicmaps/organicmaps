#import <objc/runtime.h>
#import "MWMSearch.h"
#import "MWMSearchManager+Filter.h"
#import "Statistics.h"

@interface MWMSearchManager ()

@property(weak, nonatomic, readonly) UIViewController *ownerController;
@property(weak, nonatomic) IBOutlet UIButton *actionBarView;

@property(nonatomic, copy) MWMVoidBlock onFinishCallback;

@end

@implementation MWMSearchManager (Filter)

- (void)showHotelFilterWithParams:(MWMHotelParams *)params onFinishCallback:(MWMVoidBlock)callback {
  MWMSearchHotelsFilterViewController *filterVC =
    (MWMSearchHotelsFilterViewController *)[MWMSearchHotelsFilterViewController controller];
  filterVC.delegate = self;

  UINavigationController *navController = [[UINavigationController alloc] initWithRootViewController:filterVC];
  UIViewController *ownerController = self.ownerController;

  if (IPAD) {
    navController.modalPresentationStyle = UIModalPresentationPopover;
    UIPopoverPresentationController *popover = navController.popoverPresentationController;
    popover.sourceView = self.actionBarView;
    popover.sourceRect = self.actionBarView.bounds;
    popover.permittedArrowDirections = UIPopoverArrowDirectionLeft;
  }

  self.onFinishCallback = callback;
  [ownerController presentViewController:navController
                                animated:YES
                              completion:^{
                                if (params)
                                  [filterVC applyParams:params];
                              }];
}

- (IBAction)updateTap {
  [self showHotelFilterWithParams:[MWMSearch getFilter] onFinishCallback:nil];
  [Statistics logEvent:kStatSearchContextAreaClick withParameters:@{kStatValue: kStatFilter}];
}

- (IBAction)clearFilter {
  [MWMSearch clearFilter];
}

- (NSString *)onFinishCallback {
  return objc_getAssociatedObject(self, @selector(onFinishCallback));
}

- (void)setOnFinishCallback:(NSString *)onFinishCallback {
  objc_setAssociatedObject(self, @selector(onFinishCallback), onFinishCallback, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

#pragma mark - MWMSearchHotelsFilterViewControllerDelegate

- (void)hotelsFilterViewController:(MWMSearchHotelsFilterViewController *)viewController
                   didSelectParams:(MWMHotelParams *)params {
  [MWMSearch updateHotelFilterWithParams:params];
  [self.ownerController dismissViewControllerAnimated:YES completion:self.onFinishCallback];
}

- (void)hotelsFilterViewControllerDidCancel:(MWMSearchHotelsFilterViewController *)viewController {
  [self.ownerController dismissViewControllerAnimated:YES completion:nil];
}

@end
