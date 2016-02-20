#import "MWMViewController.h"

@interface MWMMapDownloadDialog : UIView

+ (instancetype)dialogForController:(MWMViewController *)controller;

- (void)processViewportCountryEvent:(TCountryId const &)countryId;

@end
